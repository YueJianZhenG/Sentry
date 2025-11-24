#pragma once

#include<vector>
#include<unordered_map>
#include"MethodConfig.h"
#include<unordered_set>
#include"Config/Base/JsonConfig.h"
#include"Core/Singleton/Singleton.h"
namespace acs
{
	class InterfaceConfig : public JsonConfig
	{
	public:
		using JsonConfig::JsonConfig;
	private:
		void OnReload() final;
		void OnLoad(const std::string &path) final;
	private:
		std::unordered_map<std::string, long long> mFileTime;
	};


    class RpcConfig final : public InterfaceConfig, public ConstSingleton<RpcConfig>
    {
    public:
        RpcConfig();
	public:
		void GetRpcMethods(std::vector<std::string> & methods) const;
    	const RpcMethodConfig * GetMethodConfig(int opcode) const;
		const RpcMethodConfig * GetMethodConfig(const std::string & fullName) const;
		bool GetMethodConfigs(std::vector<const RpcMethodConfig *> & configs, int type) const;
		bool GetMethodConfigs(const std::string & name, std::vector<const RpcMethodConfig *> & configs) const;
		inline bool HasService(const std::string & name) const { return this->mAllServices.find(name) != this->mAllServices.end(); }
    private:
		bool OnLoadJson() final;
		bool OnReLoadJson() final;
        RpcMethodConfig* MakeConfig(const std::string& name);
    private:
		std::unordered_set<std::string> mAllServices;
    	std::unordered_map<int, RpcMethodConfig *> mRpcOpcodeMethodConfig;
        std::unordered_map<std::string, std::unique_ptr<RpcMethodConfig>> mRpcMethodConfig;
    };

    class HttpConfig final : public InterfaceConfig , public Singleton<HttpConfig>
    {
    public:
        HttpConfig() : InterfaceConfig("HttpConfig") { }
    public:
        const HttpMethodConfig * GetMethodConfig(const std::string & url) const;
		void GetMethodList(std::vector<const HttpMethodConfig *> & configs, const std::string & url) const;
		void GetMethodConfigs(std::vector<const HttpMethodConfig *> & configs, const std::string & method) const;
		inline bool HasService(const std::string & name) const { return this->mAllService.find(name) != this->mAllService.end(); }
    private:
        HttpMethodConfig* MakeConfig(const std::string& url);
    private:
		bool OnLoadJson() final;
		bool OnReLoadJson() final;
    private:
		std::unordered_set<std::string> mAllService;
		std::unordered_map<std::string, long long> mFileTime;
		std::vector<std::unique_ptr<HttpMethodConfig>> mRegexConfigs;
        std::unordered_map<std::string, std::unique_ptr<HttpMethodConfig>> mMethodConfigs;
    };
}// namespace Sentry