#pragma once

#include"../Client/MysqlClient.h"
#include"../Client/MysqlHelper.h"
#include"Component/RpcService/LocalService.h"

namespace Sentry
{
	class MysqlService : public LocalService
	{
	 public:
		MysqlService() = default;
		~MysqlService() final = default;
	private:

        XCode Add(const db::mysql::add& request);

		XCode Save(const db::mysql::save& request);

		XCode Update(const db::mysql::update& request);

		XCode Delete(const db::mysql::remove& request);

        XCode Create(const db::mysql::create& request);

        XCode Query(const db::mysql::query& request, db::mysql::response& response);

	 private:
		bool OnStartService(ServiceMethodRegister & methodRegister);
	 private:
        MysqlHelper mMysqlHelper;
        class ProtoComponent * mProtoComponent;
        class MysqlDBComponent * mMysqlComponent;
	};
}// namespace Sentry