//
// Created by 64658 on 2025/2/24.
//

#ifndef APP_DB_EXPLAIN_H
#define APP_DB_EXPLAIN_H
#include <algorithm>
#include "Yyjson/Object/JsonObject.h"
namespace db
{
	class Explain : public json::Object<Explain>
	{
	public:
		bool open = false;
		std::vector<std::string> command;
	public:
		inline bool HasCommand(const std::string & cmd) const {
			return std::find(command.begin(), command.end(), cmd) != command.end();
		}
		inline static void RegisterAllFields()
		{
			REGISTER_JSON_CLASS_FIELD(Explain, open);
			REGISTER_JSON_CLASS_FIELD(Explain, command);
		}
	};
}

#endif //APP_DB_EXPLAIN_H
