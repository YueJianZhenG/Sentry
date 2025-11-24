//
// Created by 64658 on 2025/2/11.
//

#ifndef APP_MYSQLCOMMON_H
#define APP_MYSQLCOMMON_H

#include <string>
#include <vector>

namespace mysql
{
	namespace cmd
	{
		constexpr char SLEEP = 0x00;             // 使 MySQL 服务器进入休眠状态
		constexpr char QUIT = 0x01;              // 关闭 MySQL 连接
		constexpr char INIT_DB = 0x02;           // 选择数据库 (USE database)
		constexpr char QUERY = 0x03;             // 执行 SQL 语句
		constexpr char FIELD_LIST = 0x04;        // 获取表的字段列表
		constexpr char CREATE_DB = 0x05;         // 创建数据库
		constexpr char DROP_DB = 0x06;           // 删除数据库
		constexpr char REFRESH = 0x07;           // 刷新数据库或表缓存
		constexpr char SHUTDOWN = 0x08;          // 关闭 MySQL 服务器
		constexpr char STATISTICS = 0x09;        // 获取 MySQL 服务器统计信息
		constexpr char PROCESS_INFO = 0x0A;      // 获取正在执行的进程列表
		constexpr char CONNECT = 0x0B;           // 连接 MySQL 服务器
		constexpr char PROCESS_KILL = 0x0C;      // 终止查询进程
		constexpr char DEBUG = 0x0D;             // 调试 MySQL 服务器
		constexpr char PING = 0x0E;              // 检测 MySQL 服务器是否存活
		constexpr char TIME = 0x0F;              // 获取服务器当前时间
		constexpr char DELAYED_INSERT = 0x10;    // 已弃用
		constexpr char CHANGE_USER = 0x11;       // 切换用户
		constexpr char BINLOG_DUMP = 0x12;       // 复制 Binlog
		constexpr char TABLE_DUMP = 0x13;        // 复制表数据（已废弃）
		constexpr char CONNECT_OUT = 0x14;       // 外部连接
		constexpr char REGISTER_SLAVE = 0x15;    // 作为从服务器注册
		constexpr char STMT_PREPARE = 0x16;      // 预处理 SQL 语句（用于 `PREPARE` 语句）
		constexpr char STMT_EXECUTE = 0x17;      // 执行预处理的 SQL 语句
		constexpr char STMT_SEND_LONG_DATA = 0x18; // 发送长数据（如 BLOB）
		constexpr char STMT_CLOSE = 0x19;        // 关闭预处理语句
		constexpr char STMT_RESET = 0x1A;        // 重置预处理语句
		constexpr char SET_OPTION = 0x1B;        // 设置会话选项
		constexpr char STMT_FETCH = 0x1C;        // 获取预处理查询结果
		constexpr char DAEMON = 0x1D;            // MySQL 服务器后台任务
		constexpr char BINLOG_DUMP_GTID = 0x1E;  // Binlog GTID 复制
		constexpr char RESET_CONNECTION = 0x1F;  // 重置连接
	}

	constexpr uint32_t CLIENT_FLAG = 17037263; //1 0000 0011 1111 0111 1100 1111
	constexpr unsigned int SERVER_MORE_RESULTS_EXISTS = 8;
}

namespace mysql
{
	namespace client_flag
	{
		// ======================== 基础功能 ========================
		/// @brief 支持改进版旧密码认证（更长的密码哈希）
		/// @note 用于兼容旧版认证协议
		constexpr unsigned int CLIENT_LONG_PASSWORD = 1;

		/// @brief 返回匹配行数而非受影响行数
		/// @note 对于UPDATE和DELETE语句，返回找到的行数而不是实际修改或删除的行数
		constexpr unsigned int CLIENT_FOUND_ROWS = 2;

		/// @brief 启用长查询和结果集
		/// @note 允许发送和接收更大的数据包
		constexpr unsigned int CLIENT_LONG_FLAG = 4;

		/// @brief 连接时指定数据库名
		/// @note 在连接请求中包含要使用的数据库名称
		constexpr unsigned int CLIENT_CONNECT_WITH_DB = 8;

		/// @brief 禁止使用数据库名前缀表名
		/// @note 不允许在表名前加上数据库名作为前缀
		constexpr unsigned int CLIENT_NO_SCHEMA = 16;

		/// @brief 请求压缩通信
		/// @note 如果服务器支持，将启用压缩以减少网络传输的数据量
		constexpr unsigned int CLIENT_COMPRESS = 32;

		/// @brief 标识为ODBC客户端
		/// @note 表示这是一个ODBC驱动程序
		constexpr unsigned int CLIENT_ODBC = 64;

		/// @brief 允许使用本地文件加载数据
		/// @note 客户端可以执行LOAD DATA LOCAL INFILE命令
		constexpr unsigned int CLIENT_LOCAL_FILES = 128;

		/// @brief 忽略函数名后的空格
		/// @note 解析SQL时不考虑函数名后面的空白字符
		constexpr unsigned int CLIENT_IGNORE_SPACE = 256;

		/// @brief 使用MySQL 4.1版本及以后的协议
		/// @note 包含了对新特性如预处理语句的支持
		constexpr unsigned int CLIENT_PROTOCOL_41 = 512;

		/// @brief 标记此连接为交互式
		/// @note 可能会影响超时设置等行为
		constexpr unsigned int CLIENT_INTERACTIVE = 1024;

		/// @brief 请求SSL加密连接
		/// @note 如果服务器配置了SSL，则会建立安全连接
		constexpr unsigned int CLIENT_SSL = 2048;

		/// @brief 忽略SIGPIPE信号
		/// @note 防止因网络问题导致进程终止
		constexpr unsigned int CLIENT_IGNORE_SIGPIPE = 4096;

		/// @brief 支持事务操作
		/// @note 客户端能够发起和管理事务
		constexpr unsigned int CLIENT_TRANSACTIONS = 8192;

		/// @brief 保留位，目前未使用
		constexpr unsigned int CLIENT_RESERVED = 16384;

		constexpr unsigned int CLIENT_SECURE_CONNECTION =  32768;

		/// @brief 允许多条语句在一个查询中执行
		/// @note 单个query可以包含多个SQL语句
		constexpr unsigned int CLIENT_MULTI_STATEMENTS = (1UL << 16);

		/// @brief 允许多个结果集从一个查询返回
		/// @note 如存储过程调用可能产生多结果集
		constexpr unsigned int CLIENT_MULTI_RESULTS = (1UL << 17);

		/// @brief 预处理语句可返回多个结果集
		/// @note 类似CLIENT_MULTI_RESULTS，针对预处理语句
		constexpr unsigned int CLIENT_PS_MULTI_RESULTS = (1UL << 18);

		/// @brief 插件认证机制
		/// @note 允许使用插件进行身份验证
		constexpr unsigned int CLIENT_PLUGIN_AUTH = (1UL << 19);

		/// @brief 连接属性
		/// @note 发送额外信息给服务器，例如应用程序名称
		constexpr unsigned int CLIENT_CONNECT_ATTRS = (1UL << 20);

		/// @brief 插件认证数据长度编码
		/// @note 认证数据的长度使用变长整数编码
		constexpr unsigned int CLIENT_PLUGIN_AUTH_LENENC_CLIENT_DATA = (1UL << 21);

		/// @brief 能够处理过期密码的情况
		/// @note 当用户密码过期时，允许进行特定操作
		constexpr unsigned int CLIENT_CAN_HANDLE_EXPIRED_PASSWORDS = (1UL << 22);

		/// @brief 会话跟踪
		/// @note 服务器向客户端报告状态变化
		constexpr unsigned int CLIENT_SESSION_TRACK = (1UL << 23);

		/// @brief 弃用EOF包
		/// @note 使用OK包代替EOF包来结束结果集
		constexpr unsigned int CLIENT_DEPRECATE_EOF = (1UL << 24);

		constexpr unsigned int CLIENT_OPTIONAL_RESULTSET_METADATA = (1UL << 25);
	}

	namespace server_flag
	{
		// ======================== 服务器状态标志 ========================
		/// @brief 当前连接处于事务中
		constexpr unsigned int SERVER_STATUS_IN_TRANS = 0x00000001;

		/// @brief 自动提交模式开启
		constexpr unsigned int SERVER_STATUS_AUTOCOMMIT = 0x00000002;

		/// @brief 存在更多结果集待读取
		constexpr unsigned int SERVER_MORE_RESULTS_EXISTS = 0x00000008;

		/// @brief 查询没有使用到好的索引
		constexpr unsigned int SERVER_STATUS_NO_GOOD_INDEX_USED = 0x00000010;

		/// @brief 查询完全没有使用索引
		constexpr unsigned int SERVER_STATUS_NO_INDEX_USED = 0x00000020;

		/// @brief 游标存在并可用
		constexpr unsigned int SERVER_STATUS_CURSOR_EXISTS = 0x00000040;

		/// @brief 最后一行已发送给客户端
		constexpr unsigned int SERVER_STATUS_LAST_ROW_SENT = 0x00000080;

		/// @brief 数据库已被删除
		constexpr unsigned int SERVER_STATUS_DB_DROPPED = 0x00000100;

		/// @brief 关闭反斜杠转义
		constexpr unsigned int SERVER_STATUS_NO_BACKSLASH_ESCAPES = 0x00000200;

		/// @brief 元数据发生改变
		constexpr unsigned int SERVER_STATUS_METADATA_CHANGED = 0x00000400;

		/// @brief 查询执行时间较长
		constexpr unsigned int SERVER_QUERY_WAS_SLOW = 0x00000800;

		/// @brief 预处理语句输出参数
		constexpr unsigned int SERVER_PS_OUT_PARAMS = 0x00001000;

		/// @brief 处于只读事务中
		constexpr unsigned int SERVER_STATUS_IN_TRANS_READONLY = 0x00002000;

		/// @brief 会话状态发生变化
		constexpr unsigned int SERVER_SESSION_STATE_CHANGED = 0x00004000;
	}
}

namespace mysql
{
	namespace charset
	{
		constexpr unsigned char latin1 = 8;  // Latin1字符集，常用于西欧语言，编号为8
		constexpr unsigned char utf8 = 33; // UTF-8字符集，广泛支持多种语言字符，编号为33
		constexpr unsigned char utf8mb4 = 255; // UTF-8mb4字符集，支持完整的Unicode字符集，包括表情符号等，编号为255
		constexpr unsigned char ascii = 1; // ASCII字符集，包含128个字符，主要用于英文字符，编号为1
		constexpr unsigned char gbk = 16; // GBK字符集，常用于简体中文，编号为16
		constexpr unsigned char big5 = 18; // Big5字符集，常用于繁体中文，编号为18
		constexpr unsigned char latin2 = 4; // Latin2字符集，支持中欧语言，编号为4
		constexpr unsigned char sjis = 32; // Shift-JIS字符集，用于日本语言，编号为32
		constexpr unsigned char euckr = 22; // EUC-KR字符集，用于韩国语言，编号为22
		constexpr unsigned char utf32 = 101; // UTF-32字符集，支持所有Unicode字符，编号为101
	}
}

namespace mysql
{
	namespace config
	{
		constexpr unsigned char charset = charset::utf8mb4; //默认字符集
		constexpr uint32_t PACKAGE_MAX_SIZE = 1024 * 1024 * 2; // 默认2M
	}
}

namespace mysql
{
	namespace field
	{
		// 整数类型
		constexpr unsigned char MYSQL_TYPE_TINY = 1;    // TINYINT
		constexpr unsigned char MYSQL_TYPE_SHORT = 2;    // SMALLINT
		constexpr unsigned char MYSQL_TYPE_LONG = 3;    // INT
		constexpr unsigned char MYSQL_TYPE_FLOAT = 4;    // FLOAT
		constexpr unsigned char MYSQL_TYPE_DOUBLE = 5;    // DOUBLE
		constexpr unsigned char MYSQL_TYPE_NULL = 6;    // DOUBLE
		constexpr unsigned char MYSQL_TYPE_TIMESTAMP = 7;    // DOUBLE

		constexpr unsigned char MYSQL_TYPE_LONGLONG = 8;
		constexpr unsigned char MYSQL_TYPE_INT24 = 9;
		constexpr unsigned char MYSQL_TYPE_DATE = 10;
		constexpr unsigned char MYSQL_TYPE_TIME = 11;
		constexpr unsigned char MYSQL_TYPE_DATETIME = 12;
		constexpr unsigned char MYSQL_TYPE_YEAR = 13;
		constexpr unsigned char MYSQL_TYPE_NEWDATE = 14;
		constexpr unsigned char MYSQL_TYPE_VARCHAR = 15;
		constexpr unsigned char MYSQL_TYPE_BIT = 16;
		constexpr unsigned char MYSQL_TYPE_TIMESTAMP2 = 17;
		constexpr unsigned char MYSQL_TYPE_DATETIME2 = 18;
		constexpr unsigned char MYSQL_TYPE_TIME2 = 19;
		constexpr unsigned char MYSQL_TYPE_JSON = 245;
		constexpr unsigned char MYSQL_TYPE_NEWDECIMAL = 246;
		constexpr unsigned char MYSQL_TYPE_ENUM = 247;
		constexpr unsigned char MYSQL_TYPE_SET = 248;
		constexpr unsigned char MYSQL_TYPE_TINY_BLOB = 249;
		constexpr unsigned char MYSQL_TYPE_MEDIUM_BLOB = 250;
		constexpr unsigned char MYSQL_TYPE_LONG_BLOB = 251;
		constexpr unsigned char MYSQL_TYPE_BLOB = 252;
		constexpr unsigned char MYSQL_TYPE_VAR_STRING = 253;
		constexpr unsigned char MYSQL_TYPE_STRING = 254;
		constexpr unsigned char MYSQL_TYPE_GEOMETRY = 255;
	}
}


namespace mysql
{
	constexpr unsigned char PACKAGE_OK = 0x00;
	constexpr unsigned char PACKAGE_ERR = 0xFF;
	constexpr unsigned char PACKAGE_EOF = 0xFE;
	constexpr unsigned char PACKAGE_MORE = 0x01;
	constexpr unsigned char PACKAGE_USE_SSL = 0x02;
}

namespace mysql
{
	struct FieldInfo
	{
		std::string name;
		unsigned char type;
	};
}

namespace mysql
{
	struct OKResponse
	{
	public:
		std::string mInfo;
		unsigned int mAffectedRows = 0;
		unsigned int mLastInsertId = 0;
		unsigned int mServerStatus = 0;
		unsigned int mWarningCount = 0;
	};

//	struct StmtResponse
//	{
//	public:
//		unsigned int stmtId = 0;
//		unsigned short columnCount = 0;
//		unsigned short paramCount = 0;
//		unsigned short warning = 0;
//		unsigned int nullBitMapLen = 0;
//	};

}

#endif //APP_MYSQLCOMMON_H
