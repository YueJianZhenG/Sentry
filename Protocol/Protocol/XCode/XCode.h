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
    CallLuaFunctionFail = 10,//调用lua方法错误
    NotResponseMessage = 11,//不返回数据
    JsonCastProtocbufFail = 12,//json转protobuf失败
    ProtocbufCastJsonFail = 13,//protobuf转json失败
    CreatePorotbufFail = 14,//创建protobuf失败
    SerializationFailure = 15,//序列化失败
    CommandArgsError = 16,//GM命令参数错误
    SessionIsNull = 17,//Session是空
    SendMessageFail = 18,//发送消息错误
    NoCoroutineContext = 19,//没有协程上下文
    ParseJsonFailure = 20,//解析json失败
    CacheMessageNextSend = 21,//缓存消息下次发送
    MysqlNotInCoroutine = 22,//Mysql没有协程上下文
    MysqlStartTaskFail = 23,//Mysql任务启动失败
    MysqlInitTaskFail = 24,//Mysql初始化失败
    MysqlSocketIsNull = 25,//Mysql连接是空
    MysqlSelectDbFailure = 26,//Mysql选择数据库失败
    MysqlInvokeFailure = 27,//Mysql执行sql语句失败
    RedisNotInCoroutine = 28,//Redis没有协程上下文
    RedisStartTaskFail = 29,//Redis任务启动失败
    RedisSocketIsNull = 30,//Redis连接是空
    RedisReplyIsNull = 31,//Redis回复是空
    RedisInvokeFailure = 32,//Rsedis命令执行失败
    AccountAlreadyExists = 33,//账号已经存在
    AccountNotExists = 34//账号不存在
};