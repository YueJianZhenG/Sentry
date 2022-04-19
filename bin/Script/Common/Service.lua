
Service = {}

function Service.Call(func, id, json)

    Log.Info(func, id, json)
    local tab = Json.ToObject(json)
    local state, code, response = pcall(func, id, tab)

    if not state then
        Log.Error(code)
        return XCode.CallLuaFunctionFail
    end
    print(state, code, response)
    assert(type(code) == 'number')
    assert(type(response) == 'table' or type(response) == 'nil')

    if code ~= XCode.Successful then
        return code
    end
    return code, response ~= nil and Json.ToObject(response) or nil
end

function Service.CallAsync(func, id, json)

    Log.Info(id, json)
    local context = function(luaTaskSource)
        local tab = Json.ToObject(json)
        local state, error, response = pcall(func, id, tab)
        if not state then
            Log.Error(error)
            return luaTaskSource:SetResult(XCode.CallLuaFunctionFail, error)
        end
        assert(type(error) == 'number')
        assert(type(response) == 'table' or type(response) == 'nil')

        if error == XCode.Successful then
            if type(response) == 'table' then
                local str = Json.ToString(response)
                print("response = ", str)
                return luaTaskSource:SetResult(error, str)
            end
        end
        return luaTaskSource:SetResult(error, StringUtil.Empty)
    end
    local taskSource = LuaServiceTaskSource.New();
    coroutine.start(context, taskSource)
    return taskSource;
end