
#include"ServerConfig.h"
#include"Core/System/System.h"
#include"Util/File/FileHelper.h"
#include"Util/File/DirectoryHelper.h"

namespace acs
{
	ServerConfig::ServerConfig()
		: JsonConfig("ServerConfig")
	{

	}

	bool ServerConfig::OnReLoadJson()
	{
		return true;
	}

	bool ServerConfig::OnLoadText(const char* str, size_t length)
	{
		json::r::Document mainDocument;
		json::w::Document jsonDocument;
		if(!mainDocument.Decode(str, length))
		{
			return false;
		}
		std::string directory;
		json::r::Value jsonArray;
		const std::string & path = this->Path();
		help::dir::GetDirByPath(path, directory);
		if(mainDocument.Get("include", jsonArray) && jsonArray.IsArray())
		{
			size_t index = 0;
			std::string file;
			while (jsonArray.Get(index, file))
			{
				std::string content;
				json::r::Document document1;
				std::string p = directory + file;
				if (!os::System::ReadFile(p, content))
				{
					return false;
				}
				if (!document1.Decode(content, YYJSON_READ_INSITU))
				{
					return false;
				}
				json::Merge(jsonDocument, document1);
				index++;
			}
		}
		std::string json;
		json::Merge(jsonDocument, mainDocument);
		if(jsonDocument.Serialize(&json, true) && this->Decode(json))
		{
			//printf("%s\n", json.c_str());
			help::fs::WriterFile("./gen.json", json);
			return this->OnLoadJson();
		}
		return false;
	}

	bool ServerConfig::OnReloadText(const char* str, size_t length)
	{
		return true;
	}

	bool ServerConfig::OnLoadJson()
	{
		json::r::Value jsonObject;
		if(!this->Get("core", jsonObject))
		{
			return false;
		}
		std::string secret;
		if(jsonObject.Get("secret", secret))
		{
			if(help::fs::FileIsExist(secret.c_str()))
			{
				help::fs::ReadTxtFile(secret, this->mSecret);
			}
			else
			{
				this->mSecret = secret;
			}
		}
		os::System::GetAppEnv("name", this->mName);
		if(this->Get("path", jsonObject) && jsonObject.IsObject())
		{
			std::vector<const char *> keys;
			if(jsonObject.GetKeys(keys) > 0)
			{
				std::unique_ptr<json::r::Value> value;
				for (const char* k: keys)
				{
					std::string path;
					if (jsonObject.Get(k, path))
					{
						this->mPaths.emplace(k, path);
					}
				}
			}
		}
		os::System::SetAppEnv("name", this->mName);
		return true;
	}

	bool ServerConfig::GetPath(const std::string& name, std::string& path) const
	{
		auto iter = this->mPaths.find(name);
		if (iter == this->mPaths.end())
		{
			return false;
		}
		path = iter->second;
		return true;
	}

	std::unique_ptr<json::r::Document> ServerConfig::Read(const std::string& name) const
	{
		std::string path;
		if(!this->GetPath(name, path))
		{
			return nullptr;
		}
		std::unique_ptr<json::r::Document>
		        document = std::make_unique<json::r::Document>();
		return document->FromFile(path) ? std::move(document) : nullptr;
	}
}
