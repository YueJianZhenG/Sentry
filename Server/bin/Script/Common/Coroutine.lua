function coroutine.wakeup(cor, ...)
    local res, ret = coroutine.resume(cor, ...)
    assert(res, ret)
    return ret
end

function coroutine.start(func, ...)
    local cor = coroutine.create(func)
    return coroutine.wakeup(cor, ...)
end

function coroutine.call(func)
    local luaTaskSource = TaskSource.New()
    coroutine.start(function(taskSource)
        local state, error = pcall(func)
        if not state then
            Log.Error(error)
        end
        taskSource:SetResult()
    end, luaTaskSource)
    return luaTaskSource
end

-- function coroutine.sleep(ms)
--     if coroutine.running() == nil then
--         assert(false, "sleep not in coroutine")
--     end
--     return SoEasy.Sleep(ms)
-- end
