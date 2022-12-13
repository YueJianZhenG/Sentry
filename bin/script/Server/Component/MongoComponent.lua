
local MongoComponent = {}
local this = MongoComponent
function MongoComponent.InsertOnce(tab, data, flag)
    if type(data) == "table" then
        data = rapidjson.encode(data)
    end
    if flag == nil then
        flag = type(tab._id) == "number" and tab._id or 0
    end
    local address = Service.AllotServer("MongoService")
    return Service.Call(address, "MongoService.Insert", {
        tab = tab,
        json = data,
        flag = flag
    })
end

function MongoComponent.Delete(tab, data, limit, flag)
    if type(data) == "table" then
        data = rapidjson.encode(data)
    end
    assert(type(data) == "string")
    if flag == nil then
        flag = type(tab._id) == "number" and tab._id or 0
    end
    local address = Service.AllotServer("MongoService")
    return Service.Call(address, "MongoService.Delete", {
        tab = tab,
        json = data,
        limit = limit or 1,
        flag = flag
    })
end

function MongoComponent.ClearTable(tab)
    assert(type(tab) == "string")
    return this.Delete(tab, {}, 0) == XCode.Successful
end

function MongoComponent.QueryOnce(tab, data)
    if type(data) == "table" then
        data = rapidjson.encode(data)
    end
    assert(type(data) == "string")
    local address = Service.AllotServer("MongoService")
    local code, response = Service.Call(address, "MongoService.Query", {
        tab = tab,
        json = data,
        limit = 1
    })
    if code ~= XCode.Successful or response == nil then
        return nil
    end
    if #response.jsons > 0 then
        return rapidjson.decode(response.jsons[1])
    end
    return nil
end

function MongoComponent.Query(tab, data, limit)
    if type(data) == "table" then
        data = rapidjson.encode(data)
    end
    print("query json ", data)
    assert(type(data) == "string")
    local address = Service.AllotServer("MongoService")
    local code, response = Service.Call(address, "MongoService.Query", {
        tab = tab,
        json = data,
        limit = limit or 0
    })
    if code ~= XCode.Successful or response == nil then
        return nil
    end
    local responses = {}
    if #response.jsons > 0 then
        for _, json in ipairs(response.jsons) do
            table.insert(responses, rapidjson.decode(json))
        end
    end
    return responses
end

function MongoComponent.QueryDatas(tab, list)
    assert(type(tab) == "string")
    assert(type(list) == "table" and #list > 0)

    local request = {
        _id = {
            ["$in"] = list
        }
    }
    local address = Service.AllotServer("MongoService")
    local code, response = Service.Call(address, "MongoService.Query", {
        tab = tab,
        limit = #list,
        json = rapidjson.encode(request)
    })
    if code ~= XCode.Successful or response == nil then
        return nil
    end
    local responses = {}
    if #response.jsons > 0 then
        for _, json in ipairs(response.jsons) do
            table.insert(responses, rapidjson.decode(json))
        end
    end
    return responses
end

function MongoComponent.SetIndex(tab, name)
    assert(type(tab) == "string")
    assert(type(name) == "string")
    local address = Service.AllotServer("MongoService")
    return Service.Call(address, "MongoService.SetIndex", {
        tab = tab,
        name = name
    })
end

function MongoComponent.Update(tab, select, update, tag, flag)
    if type(select) == "table" then
        select = rapidjson.encode(select)
    end
    if type(update) == "table" then
        update = rapidjson.encode(update)
    end
    assert(type(select) == "string")
    assert(type(update) == "string")
    if flag == nil then
        flag = type(tab._id) == "number" and tab._id or 0
    end
    local address = Service.AllotServer("MongoService")
    return Service.Call(address, "MongoService.Update", {
        tab = tab,
        select = select,
        update = update,
        tag = tag or "$set",
        flag = flag
    })
end

function MongoComponent.Push(tab, select, update)
    return this.Update(tab, select, update, "$push")
end

return MongoComponent