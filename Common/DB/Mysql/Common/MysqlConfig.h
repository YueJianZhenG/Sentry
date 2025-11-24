//
// Created by zmhy0073 on 2022/10/26.
//

#ifndef APP_MYSQLCONFIG_H
#define APP_MYSQLCONFIG_H
#include "DB/Common/Url.h"
#include "DB/Common/Explain.h"

namespace mysql
{
	class Cluster : public json::Object<Cluster>
	{
	public:
		int ping = 15;
		int count = 1;
		int retry = 5; //重试时间
		int conn_count = 3; //重试次数
		bool debug = false;
		std::string table;
		std::string binlog;
		db::Explain explain;
		unsigned int timeout = 15;
		std::vector<std::string> address;
	public:
		inline static void RegisterAllFields()
		{
			REGISTER_JSON_CLASS_FIELD(mysql::Cluster, ping);
			REGISTER_JSON_CLASS_FIELD(mysql::Cluster, count);
			REGISTER_JSON_CLASS_FIELD(mysql::Cluster, retry);
			REGISTER_JSON_CLASS_FIELD(mysql::Cluster, table);
			REGISTER_JSON_CLASS_FIELD(mysql::Cluster, debug);
			REGISTER_JSON_CLASS_FIELD(mysql::Cluster, explain);
			REGISTER_JSON_CLASS_FIELD(mysql::Cluster, binlog);
			REGISTER_JSON_CLASS_FIELD(mysql::Cluster, conn_count);
			REGISTER_JSON_CLASS_MUST_FIELD(mysql::Cluster, address);
		}
	};

	class Config : public db::Url
	{
	public:
		Config() : db::Url("mysql") { }
	public:
		std::string db;
		std::string user;
		int conn_count = 3;
		std::string script;
		std::string address;
		std::string password;
	};

	class Explain : public json::Object<Explain>
	{
	public:
		int id; // 查询块的唯一标识符，数值越小优先级越高（执行顺序越靠前）。
		// 示例：id=1 表示主查询，id=2 可能为子查询或 UNION 操作中的子部分。

		std::string ref; // 在连接操作中，用于关联其他表的列或常量。
		// 典型值：`const`（常量）、`NULL`（无关联）、`db.table.column`（关联其他表的列）。

		std::string table; // 当前操作涉及的表名，可能包含别名或派生表标识（如 `<derivedN>`）。
		// 示例：`user_order_list` 或 `<derived2>`（表示第2个派生表）。

		std::string key; // 实际使用的索引名称。若为 `PRIMARY` 表示主键索引，空字符串表示未使用索引。
		// 示例：`idx_order_date` 表示使用了名为该名称的索引。

		std::string select_type; // 查询类型，反映查询的复杂性和执行方式。
		// 典型值：`SIMPLE`（简单查询）、`PRIMARY`（外层查询）、`SUBQUERY`（子查询）、`UNION`（UNION 操作中的第二个或后续查询）。

		std::string Extra; // 额外执行信息，揭示查询优化的关键细节。
		// 典型值：`Using index`（索引覆盖，无需回表）、`Using where`（使用 WHERE 过滤）、`Using temporary`（使用临时表）。

		std::string type; // 数据访问类型，反映查询效率的关键指标。
		// 典型值：
		// - `system`/`const`：单行匹配（最高效）；
		// - `index`：全索引扫描；
		// - `range`：索引范围扫描；
		// - `ALL`：全表扫描（需警惕性能问题）。

		int rows; // 优化器预估需要扫描的行数（基于统计信息，可能与实际有偏差）。
		// 示例：rows=60 表示预计扫描 60 行。

		double filtered; // 存储引擎返回数据后，经服务层过滤后的剩余百分比（范围 0~100）。
		// 示例：filtered=10.0 表示 90% 的数据被 WHERE 过滤掉。

		std::string key_len; // 索引使用的字节长度，反映实际使用的索引列数量。
		// 示例：若索引为 `INT(8)`，则 key_len=4（4 字节）；复合索引可能显示总和，如 `key_len=8`。

		std::string possible_keys; // 可能适用的索引列表（逗号分隔），为空表示无合适索引。
		// 示例：`idx_a,idx_b` 表示优化器在 `idx_a` 和 `idx_b` 之间选择。

	public:
		inline static void RegisterAllFields()
		{
			REGISTER_JSON_CLASS_FIELD(mysql::Explain, id);
			REGISTER_JSON_CLASS_FIELD(mysql::Explain, ref);
			REGISTER_JSON_CLASS_FIELD(mysql::Explain, table);
			REGISTER_JSON_CLASS_FIELD(mysql::Explain, key);
			REGISTER_JSON_CLASS_FIELD(mysql::Explain, select_type);
			REGISTER_JSON_CLASS_FIELD(mysql::Explain, Extra);
			REGISTER_JSON_CLASS_FIELD(mysql::Explain, type);
			REGISTER_JSON_CLASS_FIELD(mysql::Explain, rows);
			REGISTER_JSON_CLASS_FIELD(mysql::Explain, filtered);
			REGISTER_JSON_CLASS_FIELD(mysql::Explain, key_len);
			REGISTER_JSON_CLASS_FIELD(mysql::Explain, possible_keys);
		}
	};
}


#endif //APP_MYSQLCONFIG_H