local MysqlProxyComponent = { }

local type = type
local node = require("Node")
local json = require("util.json")
local table_insert = table.insert
local table_concat = table.concat
local string_format = string.format

local MYSQL_READ_PROXY = "MysqlReadProxy"
local MYSQL_WRITE_PROXY = "MysqlWriteProxy"

---@param tab string
---@param keys table
function MysqlProxyComponent:Create(tab, keys)
    local id = node:Allot(MYSQL_WRITE_PROXY)
    local code = node:Call(id, "MysqlWriteProxy.Create", {
        tab = tab,
        keys = keys
    })
    return code == XCode.Ok
end

---@param tab string
---@param field string
---@param value number
---@param filter table
function MysqlProxyComponent:Inc(tab, filter, field, value)
    local id = node:Allot(MYSQL_WRITE_PROXY)
    local code, response = node:Call(id, "MysqlWriteProxy.Inc", {
        tab = tab,
        field = field,
        value = value or 1,
        filter = filter
    })
    return code == XCode.Ok and response.value or 0
end

---@param list table
function MysqlProxyComponent:Commit(list)
    local id = node:Allot(MYSQL_WRITE_PROXY)
    local code, response = node:Call(id, "MysqlWriteProxy.Commit", list)
    return code == XCode.Ok and response or nil
end

---@param tab string
---@param document table
---@return number
function MysqlProxyComponent:InsertOne(tab, document)
    local id = node:Allot(MYSQL_WRITE_PROXY)
    local code, response = node:Call(id, "MysqlWriteProxy.InsertOne", {
        tab = tab,
        document = document
    })
    if code ~= XCode.Ok then
        return 0
    end
    return response.count
end

---@param tab string
---@param documents table
---@return number
function MysqlProxyComponent:InsertBatch(tab, documents)
    assert(#documents > 0)
    local id = node:Allot(MYSQL_WRITE_PROXY)
    local code, response = node:Call(id, "MysqlWriteProxy.InsertBatch", {
        tab = tab,
        documents = documents
    })
    if code ~= XCode.Ok then
        return 0
    end
    return response.count
end

---@param tab string
---@param document table
---@return boolean
function MysqlProxyComponent:ReplaceOne(tab, document)
    local id = node:Allot(MYSQL_WRITE_PROXY)
    local code = node:Call(id, "MysqlWriteProxy.Replace", {
        tab = tab,
        document = document
    })
    return code == XCode.Ok
end

---@param tab string
---@param filter table | string
---@return boolean
function MysqlProxyComponent:Delete(tab, filter, limit)
    local id = node:Allot(MYSQL_WRITE_PROXY)
    local code = node:Call(id, "MysqlWriteProxy.Delete", {
        tab = tab,
        limit = limit or 1,
        filter = filter
    })
    return code == XCode.Ok
end

---@param tab string
---@param filter table | string
---@param document table
---@param limit number
---@return boolean
function MysqlProxyComponent:Update(tab, filter, document, limit)
    local id = node:Allot(MYSQL_WRITE_PROXY)
    local code, response = node:Call(id, "MysqlWriteProxy.Update", {
        tab = tab,
        limit = limit or 1,
        filter = filter,
        document = document
    })
    return code == XCode.Ok and response.count, 0
end

---@param tab string
function MysqlProxyComponent:Drop(tab)
    return self:RunInWrite(string_format("DROP TABLE %s;", tab))
end

---@param tab string
---@param field string
---@param unique boolean
---@return boolean
function MysqlProxyComponent:SetIndex(tab, field, unique)
    local id = node:Allot(MYSQL_WRITE_PROXY)
    local code = node:Call(id, "MysqlWriteProxy.SetIndex", {
        tab = tab,
        field = field,
        unique = unique or false
    })
    return code == XCode.Ok
end

---@param tab string
---@param filter table | string
---@param fields table
---@return table | nil
function MysqlProxyComponent:FindOne(tab, filter, fields)
    local id = node:Allot(MYSQL_READ_PROXY)
    local code, response = node:Call(id, "MysqlReadProxy.FindOne", {
        tab = tab,
        limit = 1,
        filter = filter,
        fields = fields,
    })
    return code ~= XCode.Ok and nil or response
end

---@param tab string
---@param filter table | string
---@param fields table
---@return table | nil
function MysqlProxyComponent:Find(tab, filter, fields, sort, limit)
    local id = node:Allot(MYSQL_READ_PROXY)
    local code, response = node:Call(id, "MysqlReadProxy.Find", {
        tab = tab,
        sort = sort,
        limit = limit,
        filter = filter,
        fields = fields,
    })
    if code ~= XCode.Ok then
        return nil
    end
    return response
end

---@param tab string
---@param field string
---@param value string
---@param fields table
---@param limit number
---@return table | nil
function MysqlProxyComponent:Like(tab, field, value, fields, sort, limit)
    local id = node:Allot(MYSQL_READ_PROXY)
    local code, response = node:Call(id, "MysqlReadProxy.Like", {
        tab = tab,
        sort = sort,
        field = field,
        value = value,
        fields = fields,
        limit = limit or 1
    })
    if code ~= XCode.Ok then
        return nil
    end
    return response
end

---@param stmt string
function MysqlProxyComponent:ExecuteInRead(stmt, ...)
    local id = node:Allot(MYSQL_READ_PROXY)
    local code, response = node:Call(id, "MysqlReadProxy.Execute", {
        stmt = stmt,
        args = { ... }
    })
    if code ~= XCode.Ok then
        return nil
    end
    return response
end

---@param tab string
---@param func string
---@param field string
---@param filter table
---@return table
function MysqlProxyComponent:Func(tab, func, field, filter, group)
    local id = node:Allot(MYSQL_READ_PROXY)
    local code, response = node:Call(id, "MysqlReadProxy.Func", {
        tab = tab,
        func = func,
        field = field,
        filter = filter,
        group =group,
    })
    return code ~= XCode.Ok and nil or response
end

---@param tab string
---@param filter table
---@return number
function MysqlProxyComponent:Count(tab, filter)
    local id = node:Allot(MYSQL_READ_PROXY)
    local code, response = node:Call(id, "MysqlReadProxy.Count", {
        tab = tab,
        filter = filter
    })
    if code ~= XCode.Ok then
        return 0
    end
    return response.count
end

---@param tab string
---@param filter table | string
---@param fields table
---@param page number
---@param count number
---@return table | nil
function MysqlProxyComponent:FindPage(tab, filter, sort, fields, page, count)
    local id = node:Allot(MYSQL_READ_PROXY)
    local code, response = node:Call(id, "MysqlReadProxy.FindPage", {
        tab = tab,
        sort = sort,
        filter = filter,
        fields = fields,
        page = page or 1,
        count = count or 10
    })
    if code ~= XCode.Ok then
        return nil
    end
    return response
end

---@param sql string
function MysqlProxyComponent:RunInRead(sql)
    local id = node:Allot(MYSQL_READ_PROXY)
    local code, response = node:Call(id, "MysqlReadProxy.Run", sql)
    if code ~= XCode.Ok then
        return nil
    end
    return response.list, response.count
end

---@param sql string
function MysqlProxyComponent:RunInWrite(sql)
    local id = node:Allot(MYSQL_WRITE_PROXY)
    local code, response = node:Call(id, "MysqlWriteProxy.Run", sql)
    if code ~= XCode.Ok then
        return nil
    end
    return response.list, response.count
end

---@param tab string
---@param field string
---@param value number
---@param filter any
function MysqlProxyComponent:Deduction(tab, field, value, filter)
    local id = node:Allot(MYSQL_WRITE_PROXY)
    local code, response = node:Call(id, "MysqlWriteProxy.Deduction", {
        tab = tab,
        field = field,
        value = value,
        filter = filter
    })
    if code ~= XCode.Ok then
        return 0
    end
    return response.count
end

return MysqlProxyComponent