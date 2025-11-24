local HttpService = require("HttpService")

local DBHttpService = HttpService()

function DBHttpService:Redis(request)
    local data = request.data
    local redis = require("RedisComponent")
    return XCode.Ok, redis:Run(table.unpack(data))
end

function DBHttpService:Mysql(request)
    local message = request.data
    local mysql = require("MysqlProxyComponent")
    local func = mysql[message.func]
    if func == nil then
        return XCode.Failure
    end
    return XCode.Ok, func(mysql, table.unpack(message.args))
end

function DBHttpService:Pgsql(request)
    local message = request.data
    local pgsql = require("PgsqlProxyComponent")
    local func = pgsql[message.func]
    if func == nil then
        return XCode.Failure
    end
    return XCode.Ok, func(pgsql, table.unpack(message.args))
end

function DBHttpService:Mongo(request)
    local data = request.data
    local mongo = require("MongoProxyComponent")
    local func = mongo[data.func]
    if func == nil then
        return XCode.Failure
    end
    return XCode.Ok, func(mongo, table.unpack(data.args))
end

function DBHttpService:Sqlite(request)
    local message = request.data
    local sqlite = require("SqliteComponent")
    local func = sqlite[message.func]
    if func == nil then
        return XCode.Failure
    end
    print(message.args)
    return XCode.Ok, func(sqlite, table.unpack(message.args))
end

return DBHttpService