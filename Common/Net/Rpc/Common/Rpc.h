//
// Created by zmhy0073 on 2021/10/14.
//

#pragma once

#include"Log/Common/CommonLogDef.h"
namespace rpc
{

	//消息类型
    namespace type
    {
        constexpr char none = 0;
		constexpr char auth = 1;       //验证身份消息
		constexpr char ping = 2;       //
		constexpr char pong = 3;       //
		constexpr char request = 4;    //请求消息
		constexpr char response = 5;   //请求的返回
		constexpr char forward = 6;    //内网通过网关转发到客户端的消息
		constexpr char broadcast = 7;  //内网广播到网关的消息
		constexpr char subPublish = 8; //发布订阅消息
		constexpr char logout = 9;     //退出消息
		constexpr char client = 10; 	  //服务器发到客户端的消息

		constexpr char close = 30; //关闭连接
		constexpr char parameter = 31; //设置socket局部参数
		// 自定义扩展
        constexpr char Max = 32;
    };

	namespace msg //协议格式
	{
		constexpr char none = 0;
		constexpr char bin = 1;  //带长度
		constexpr char json = 2;
		constexpr char text = 3; //不带长度
		constexpr char ws = 4; //websocket格式
		constexpr char opcode = 5; //长度 协议id 二进制 客户端专用
	}


	// rpc消息头
	namespace Header
	{
		const std::string func = "$func";
		const std::string code = "$code";
		const std::string sock_id = "$sock";
		const std::string client_sock_id = "$cli";
		const std::string forward_tar = "$tar";
		const std::string app_id = "$aid";
		const std::string from_addr = "$from_addr";
		const std::string id = "$id";
		const std::string channel = "$channel";
	}

	//协议类型
    namespace proto
    {
		constexpr char none = 0;
		constexpr char json = 1;
		constexpr char string = 2;
		constexpr char pb = 3;
		constexpr char lua = 4;
		constexpr char bson = 5;
		constexpr char number = 6;

		constexpr char error = 126; //错误

		// 自定义扩展
		constexpr char Max = std::numeric_limits<char>::max();
    };

	//网络类型
	namespace net
	{
		constexpr char tcp = 1; //tcp
		constexpr char http = 2; //http
		constexpr char redis = 3; //redis队列
		constexpr char client = 4; //客户端专用
		constexpr char udp = 5; //upd
		constexpr char kcp = 6; //upd
		constexpr char forward = 7;
		constexpr char ws = 8;
		constexpr char broker = 9; //中转
		// 自定义扩展
		constexpr char max = std::numeric_limits<char>::max();
	}

	//网关转发方式
	namespace forward
	{
		constexpr char fixed = 0; //固定转发
		constexpr char random = 1; //随机分配
		constexpr char hash = 2;  //id哈希
		constexpr char next = 3; //轮询
		// 自定义扩展
		constexpr char max = std::numeric_limits<char>::max();
	}

	//消息源
	namespace source
	{
		constexpr char none = 0;
		constexpr char client = 1;
		constexpr char server = 2;
	}

	struct ProtoHead
	{
	public:
		unsigned int Len = 0; //协议包长度
		char type = rpc::type::none; //协议类型
		char porto = rpc::proto::none; //使用 的通信协议
		char source = rpc::source::none; //消息源
		int rpcId = 0; // rpcId
	};

	constexpr int RPC_PACKET_LEN_BYTES = 4; //包长度占用字节
	constexpr int RPC_PACK_HEAD_LEN = RPC_PACKET_LEN_BYTES + sizeof(char) * 3 + sizeof(int);
	constexpr size_t INNER_RPC_BODY_MAX_LENGTH = 1024 * 1024 - 128; //内网消息体最大字节
	constexpr size_t OUTER_RPC_BODY_MAX_LENGTH = 1024 * 4; //外网消息体最大字节
//#pragma pack()
}

// 消息长度 类型 协议 rpcid

