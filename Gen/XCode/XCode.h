#pragma once
namespace XCode 
{
	 constexpr int Successful = 0;//成功
	 constexpr int Failure = 1;//失败
	 constexpr int NetTimeout = 2;//网络超时
	 constexpr int NetReceiveFailure = 3;//接收网络消息失败
	 constexpr int NetSendFailure = 4;//发送网络消息失败
	 constexpr int NetConnectFailure = 5;//连接服务器失败
	 constexpr int NetActiveShutdown = 6;//主动关闭连接
	 constexpr int NetBigDataShutdown = 7;//数据包过大
	 constexpr int ParseMessageError = 8;//解析protobuf失败
	 constexpr int UnKnowPacket = 9;//恶意数据包
	 constexpr int ParseRequestDataError = 10;//解析请求数据包失败
	 constexpr int ParseResponseDataError = 11;//解析响应数据包失败
	 constexpr int CallServiceNotFound = 12;//调用服务未发现
	 constexpr int NotFoundRpcConfig = 13;//没有rpc配置
	 constexpr int RpcTypeError = 14;//rpc类型错误
	 constexpr int InitMessageFail = 15;//初始化消息失败
	 constexpr int CallFunctionNotExist = 16;//调用方法不存在
	 constexpr int CallArgsError = 17;//调用参数错误
	 constexpr int NotFindUser = 18;//没有找到玩家对象
	 constexpr int CallTypeError = 19;//调用参数类型错误
	 constexpr int CallTimeout = 20;//超时自动调用
	 constexpr int ThrowError = 21;//抛出一个错误
	 constexpr int CallLuaFunctionFail = 22;//调用lua方法错误
	 constexpr int JsonCastProtoFailure = 23;//json转protobuf失败
	 constexpr int ProtoCastJsonFailure = 24;//protobuf转json失败
	 constexpr int CreateProtoFailure = 25;//创建protobuf失败
	 constexpr int SerializationFailure = 26;//序列化失败
	 constexpr int SendMessageFail = 27;//发送消息错误
	 constexpr int NetWorkError = 28;//网络错误
	 constexpr int ParseJsonFailure = 29;//解析json失败
	 constexpr int MysqlResultIsNull = 30;//Mysql结果返回空
	 constexpr int MysqlTableNameError = 31;//Mysql表名不符合规则
	 constexpr int MysqlInitTaskFail = 32;//Mysql初始化失败
	 constexpr int MysqlSocketIsNull = 33;//Mysql连接是空
	 constexpr int MysqlInvokeFailure = 34;//Mysql执行sql语句失败
	 constexpr int SaveToMysqlFailure = 35;//保存数据到Mysql失败
	 constexpr int SaveToRedisFailure = 36;//保存数据到Redis失败
	 constexpr int AccountAlreadyExists = 37;//账号已经存在
	 constexpr int AccountNotExists = 38;//账号不存在
	 constexpr int AddressAllotFailure = 39;//服务分配失败
	 constexpr int LuaCoroutineWait = 40;//等待lua协程完成
};