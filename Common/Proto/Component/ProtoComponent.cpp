//
// Created by zmhy0073 on 2022/6/2.
//

#include"ProtoComponent.h"
#include"Entity/Actor/App.h"
#include"Util/File/FileHelper.h"
#include"Util/File/DirectoryHelper.h"
#include"google/protobuf/util/json_util.h"
#include"google/protobuf/dynamic_message.h"
#include"Proto/Lua/Message.h"
#include "google/protobuf/struct.pb.h"
#include"Yyjson/Lua/ljson.h"
#include "Lua/Lib/Lib.h"
namespace acs
{
    ImportError::ImportError()
        : mHasError(false), mHasWarning(false)
    {

    }

    void ImportError::AddError(const std::string &filename, int line, int column, const std::string &message)
    {
        this->mHasError = true;
		LOG_ERROR("{}:{} error:{}", filename, line, message);
    }

    void ImportError::AddWarning(const std::string &filename, int line, int column, const std::string &message)
    {
        this->mHasWarning = true;
		LOG_WARN("{}:{} error:{}", filename, line, message);
    }
}

namespace acs
{
	ProtoComponent::ProtoComponent()
	{
		REGISTER_JSON_CLASS_FIELD(proto::Config, path);
		REGISTER_JSON_CLASS_FIELD(proto::Config, imports);
	}

    bool ProtoComponent::Awake()
	{
		LuaCCModuleRegister::Add([](Lua::CCModule & ccModule) {
			ccModule.Open("util.pb", lua::lib::luaopen_lproto);
		});
		ServerConfig& config = this->mApp->GetConfig();
		LOG_CHECK_RET_FALSE(config.Get("pb", this->mConfig))
		LOG_CHECK_RET_FALSE(this->Load(this->mConfig.path.c_str()));
		if (!this->mConfig.imports.empty())
		{
			for (const std::string& path: this->mConfig.imports)
			{
				std::vector<std::string> types;
				LOG_CHECK_RET_FALSE(this->Import(path.c_str(), types));
			}
		}
		this->RegisterMessage<google::protobuf::Struct>();
		return true;
	}

    bool ProtoComponent::Load(const char * path)
    {
        if(!help::dir::DirectorIsExist(path))
        {
            LOG_ERROR("director {} not exist", path);
            return false;
        }
		this->mDynamicMessageMap.clear();
		this->mDynamicMessageFactorys.clear();
		this->mDynamicMessageFactory = nullptr;
		this->mImportError = std::make_unique<ImportError>();
		this->mSourceTree = std::make_unique<pb::compiler::DiskSourceTree>();
        this->mSourceTree->MapPath("", path);
        this->mImporter = std::make_unique<pb::compiler::Importer>(
                this->mSourceTree.get(), this->mImportError.get());
        if (this->mImportError->HasError())
        {
            LOG_ERROR("load proto message {}  fail", path);
            return false;
        }
        LOG_DEBUG("load proto dir => [{}]", path);
        return true;
    }

	bool ProtoComponent::Import(const char * fileName, std::vector<std::string> & types)
	{
        const std::string & path = this->mConfig.path;
        const std::string fullPath(fmt::format("{0}/{1}", path, fileName));
		if(!help::fs::FileIsExist(fullPath.c_str()))
		{
			LOG_ERROR("not proto file {}", fullPath);
			return false;
		}
		long long lastWriteTime = help::fs::GetLastWriteTime(fullPath.c_str());

		auto iter = this->mFiles.find(fileName);
        if(iter != this->mFiles.end() && iter->second == lastWriteTime)
		{
			return true;
		}
        this->mFiles[fileName] = lastWriteTime;
        const pb::FileDescriptor * fileDescriptor = this->mImporter->Import(fileName);
		if (fileDescriptor == nullptr)
		{
			LOG_ERROR("import {} fail", fileName);
			return false;
		}
		LOG_DEBUG("import [{}] ok", fileName);
		this->mDynamicMessageFactory = std::make_unique<pb::DynamicMessageFactory>(fileDescriptor->pool());
		for(int x = 0; x < fileDescriptor->message_type_count(); x++)
		{
			const pb::Descriptor * descriptor = fileDescriptor->message_type(x);
			this->LoopMessage(descriptor, types);
			if(descriptor->field_count() > 0)
			{
				const pb::Message * message = this->mDynamicMessageFactory->GetPrototype(descriptor);
				types.emplace_back(message->GetTypeName());
                this->mDynamicMessageMap.emplace(message->GetTypeName(), message);
			}
		}
		this->mDynamicMessageFactorys.emplace_back(std::move(mDynamicMessageFactory));
		return true;
	}

	void ProtoComponent::LoopMessage(const pb::Descriptor* descriptor, std::vector<std::string> & protos)
	{
		for(int y = 0; y < descriptor->nested_type_count(); y++)
		{
			const pb::Descriptor * descriptor1 = descriptor->nested_type(y);
			this->LoopMessage(descriptor1, protos);
			if(descriptor1->field_count() > 0)
			{
				const pb::Message * message = this->mDynamicMessageFactory->GetPrototype(descriptor1);
				//LOG_DEBUG("add new dynamic message {}",  message->GetTypeName());
				this->mDynamicMessageMap[message->GetTypeName()] = message;
				protos.emplace_back(message->GetTypeName());
			}
		}
	}

    bool ProtoComponent::New(const pb::Any &any, std::unique_ptr<pb::Message> & message)
    {
        std::string fullName;
        if(!pb::Any::ParseAnyTypeUrl(any.type_url(), &fullName))
        {
            return false;
        }
        if(!this->New(fullName, message))
		{
			return false;
		}
		return any.UnpackTo(message.get());
    }

	bool ProtoComponent::Has(const std::string& name)
	{
		return this->FindMessage(name) != nullptr;
	}

	pb::Message * ProtoComponent::Temp(const std::string& name)
	{
	    if(name.empty())
	    {
	        return nullptr;
	    }
		auto iter = this->mTempMessages.find(name);
		if(iter!= this->mTempMessages.end())
		{
			iter->second->Clear();
			return iter->second;
		}
		pb::Message * message = nullptr;
		const pb::Message * tmpMessage = this->FindMessage(name);
		if(tmpMessage != nullptr)
		{
			message = tmpMessage->New();
			this->mTempMessages.emplace(name, message);
		}
		return message;
	}

    bool ProtoComponent::New(const std::string & name, std::unique_ptr<pb::Message> & message)
    {
        const pb::Message * tmpMessage = this->FindMessage(name);
        if(tmpMessage == nullptr)
        {
            LOG_ERROR("find protobuf message {} fail", name);
            return false;
        }
        message.reset(tmpMessage->New());
		return true;
    }

    bool ProtoComponent::New(const std::string &name, const std::string &json, std::unique_ptr<pb::Message> & message)
    {
		if(!this->New(name, message))
		{
			return false;
		}
		return pb::util::JsonStringToMessage(json, message.get()).ok();
    }

	bool ProtoComponent::New(const std::string& name, const char* json, size_t size, std::unique_ptr<pb::Message> & message)
	{
		if(!this->New(name, message))
		{
			return false;
		}
		pb::StringPiece stringPiece(json, (int)size);
		return pb::util::JsonStringToMessage(stringPiece, message.get()).ok();
	}

    const pb::Message * ProtoComponent::FindMessage(const std::string &name)
	{
		auto iter1 = this->mDynamicMessageMap.find(name);
		if (iter1 != this->mDynamicMessageMap.end())
		{
			return iter1->second;
		}

		auto iter = this->mStaticMessageMap.find(name);
		if(iter != this->mStaticMessageMap.end())
		{
			return iter->second;
		}

		//静态查找
		const pb::DescriptorPool* pDescriptorPool = pb::DescriptorPool::generated_pool();
		const pb::Descriptor* pDescriptor = pDescriptorPool->FindMessageTypeByName(name);
		if (pDescriptor == nullptr)
		{
			return nullptr;
		}
		pb::MessageFactory* factory = pb::MessageFactory::generated_factory();
		const pb::Message* pMessage = factory->GetPrototype(pDescriptor);
		if (pMessage == nullptr)
		{
			return nullptr;
		}
		this->mStaticMessageMap.emplace(name, pMessage);
		return pMessage;
	}


	bool ProtoComponent::Write(lua_State * lua, const pb::Message& message)
	{
		if(message.GetTypeName() == "google.protobuf.Struct")
		{
			std::string json;
			if(!pb_json::MessageToJsonString(message, &json).ok())
			{
				return false;
			}
			return lua::yyjson::write(lua, json.c_str(), json.size());
		}
		MessageDecoder messageDecoder(lua, this);
		return messageDecoder.Decode(message);
	}

	pb::Message * ProtoComponent::Read(lua_State * lua, const std::string& name, int index)
	{
		pb::Message* message = this->Temp(name);
		if (message == nullptr)
		{
			return nullptr;
		}
		if(message->GetTypeName() == "google.protobuf.Struct")
		{
			std::string json;
			if(!lua::yyjson::read(lua, index, json))
			{
				return nullptr;
			}
			if(!pb_json::JsonStringToMessage(json, message).ok())
			{
				return nullptr;
			}
			return message;
		}
		MessageEncoder messageEncoder(lua);
		if (!messageEncoder.Encode(*message, index))
		{
			return nullptr;
		}
		return message;
	}

	bool ProtoComponent::OnRefresh()
	{
		std::queue<std::string> files;
		for(auto iter = this->mFiles.begin(); iter != this->mFiles.end(); ++iter)
		{
			files.push(iter->first);
		}
		while(!files.empty())
		{
			std::vector<std::string> types;
			const std::string & file = files.front();
			if(!this->Import(file.c_str(), types))
			{
				return false;
			}
			files.pop();
		}
		for(auto iter1 = this->mTempMessages.begin(); iter1 != this->mTempMessages.end(); iter1++)
		{
			iter1->second->Clear();
		}
		return true;
	}

	void ProtoComponent::OnDestroy()
	{
		this->mStaticMessageMap.clear();
		this->mDynamicMessageMap.clear();
		google::protobuf::ShutdownProtobufLibrary();
	}
}