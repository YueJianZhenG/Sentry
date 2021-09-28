#pragma once
enum XCode
{
	Successful = 0,//成功
	Failure = 1,//失败
	ParseMessageError = 2,//解析protobuf失败
	CallServiceNotFound = 3,//调用服务未发现
	NotFoundRpcConfig = 4,//没有rpc配置
	RpcTypeError = 5,//rpc类型错误
	InitMessageFail = 6,//初始化消息失败
	CallFunctionNotExist = 7,//调用方法不存在
	CallArgsError = 8,//调用参数错误
	TimeoutAutoCall = 9,//超时自动调用
	LuaCoroutineWait = 10,//等待lua协程返回
	CallLuaFunctionFail = 11,//调用lua方法错误
	NotResponseMessage = 12,//不返回数据
	JsonCastProtocbufFail = 13,//json转protobuf失败
	ProtocbufCastJsonFail = 14,//protobuf转json失败
	CreatePorotbufFail = 15,//创建protobuf失败
	SerializationFailure = 16,//序列化失败
	CommandArgsError = 17,//GM命令参数错误
	SessionIsNull = 18,//Session是空
	SendMessageFail = 19,//发送消息错误
	NetWorkError = 20,//网络错误
	NoCoroutineContext = 21,//没有协程上下文
	ParseJsonFailure = 22,//解析json失败
	CacheMessageNextSend = 23,//缓存消息下次发送
	MysqlNotInCoroutine = 24,//Mysql没有协程上下文
	MysqlStartTaskFail = 25,//Mysql任务启动失败
	MysqlInitTaskFail = 26,//Mysql初始化失败
	MysqlSocketIsNull = 27,//Mysql连接是空
	MysqlSelectDbFailure = 28,//Mysql选择数据库失败
	MysqlInvokeFailure = 29,//Mysql执行sql语句失败
	RedisNotInCoroutine = 30,//Redis没有协程上下文
	RedisStartTaskFail = 31,//Redis任务启动失败
	RedisSocketIsNull = 32,//Redis连接是空
	RedisReplyIsNull = 33,//Redis回复是空
	RedisInvokeFailure = 34,//Rsedis命令执行失败
	AccountAlreadyExists = 35,//账号已经存在
	AccountNotExists = 36//账号不存在
};