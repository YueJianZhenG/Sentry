XCode = 
{
	Successful = 0,--成功
	Failure = 1,--失败
	NetTimeout = 2,--网络超时
	NetReceiveFailure = 3,--接收网络消息失败
	NetSendFailure = 4,--发送网络消息失败
	NetConnectFailure = 5,--连接服务器失败
	NetActiveShutdown = 6,--主动关闭连接
	NetBigDataShutdown = 7,--数据包过大
	ParseMessageError = 8,--解析protobuf失败
	UnKnowPacket = 9,--恶意数据包
	ParseRequestDataError = 10,--解析请求数据包失败
	ParseResponseDataError = 11,--解析响应数据包失败
	CallServiceNotFound = 12,--调用服务未发现
	NotFoundRpcConfig = 13,--没有rpc配置
	RpcTypeError = 14,--rpc类型错误
	InitMessageFail = 15,--初始化消息失败
	CallFunctionNotExist = 16,--调用方法不存在
	CallArgsError = 17,--调用参数错误
	NotFindUser = 18,--没有找到玩家对象
	CallTypeError = 19,--调用参数类型错误
	CallTimeout = 20,--超时自动调用
	ThrowError = 21,--抛出一个错误
	CallLuaFunctionFail = 22,--调用lua方法错误
	JsonCastProtoFailure = 23,--json转protobuf失败
	ProtoCastJsonFailure = 24,--protobuf转json失败
	CreateProtoFailure = 25,--创建protobuf失败
	SerializationFailure = 26,--序列化失败
	SendMessageFail = 27,--发送消息错误
	NetWorkError = 28,--网络错误
	ParseJsonFailure = 29,--解析json失败
	MysqlResultIsNull = 30,--Mysql结果返回空
	MysqlTableNameError = 31,--Mysql表名不符合规则
	MysqlInitTaskFail = 32,--Mysql初始化失败
	MysqlSocketIsNull = 33,--Mysql连接是空
	MysqlInvokeFailure = 34,--Mysql执行sql语句失败
	SaveToMysqlFailure = 35,--保存数据到Mysql失败
	SaveToRedisFailure = 36,--保存数据到Redis失败
	AccountAlreadyExists = 37,--账号已经存在
	AccountNotExists = 38,--账号不存在
	AddressAllotFailure = 39,--服务分配失败
	LuaCoroutineWait = 40,--等待lua协程完成
	MakeTcpRequestFailure = 41,--创建tcp请求错误
	MakeHttpRequestFailure = 42,--创建http请求错误
	ParseHttpUrlFailure = 43,--解析http的url失败
	UnknownMessageNetType = 44,--未知的传输网络类型
	OnlyUseTcpProtocol = 45,--只能使用tcp协议
	NotFoundServerRpcAddress = 46,--找不到服务器rpc地址
	NotFoundPlayerRpcAddress = 47,--找不到玩家所在服务器地址
}