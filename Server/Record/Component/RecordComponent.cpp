//
// Created by leyi on 2024/3/8.
//

#include "RecordComponent.h"
#include "Util/Tools/Guid.h"
#include "Http/Common/HttpRequest.h"
#include "Http/Common/HttpResponse.h"
#include "Rpc/Config/MethodConfig.h"
#include "Util/Tools/TimeHelper.h"
#include "Sqlite/Component/SqliteComponent.h"
constexpr const char * HTTP_RECORD_LIST = "http_record_list";

namespace acs
{
	RecordComponent::RecordComponent()
	{
		this->mSqlite = nullptr;
	}

	bool RecordComponent::LateAwake()
	{
		this->mSqlite = this->GetComponent<SqliteComponent>();
		return this->mSqlite != nullptr;
	}

	void RecordComponent::OnRequestDone(const HttpMethodConfig & config, const http::Request & request, const http::Response & response)
	{
		if(!config.record)
		{
			return;
		}
		json::w::Document document;
		long long id = help::ID::Make();
		long long nowTime = help::Time::NowSec();
		{
			document.Add("path", config.path);
			document.Add("desc", config.desc);
			document.Add("method", config.type);
			document.Add("_id", std::to_string(id));
			document.Add("tcreate_timeime", nowTime);
		}
		if(request.GetBody() != nullptr)
		{
			std::string req = request.GetBody()->ToStr();
			document.Add("request", req);
		}
		else if(request.GetUrl().GetQuery().Size() > 0)
		{
			std::string req = request.GetUrl().GetQuery().ToStr();
			document.Add("request", req);
		}
		if(response.GetBody() != nullptr)
		{
			std::string res = response.GetBody()->ToStr();
			document.Add("response", res);
		}
		int userId = 0;
		if(request.GetUrl().GetQuery().Get(http::query::UserId, userId))
		{
			document.Add("user_id", userId);
		}
		this->mSqlite->Insert(HTTP_RECORD_LIST, document);
	}
}