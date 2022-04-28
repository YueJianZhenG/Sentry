local lock = {}
lock.lock = function(tab)
    print(tab.key, tab.time)
    if redis.call("SETNX", tab.key, 1) == 0 then
        print("redis lock " .. tab.key .. "set faulure")
        return 0
    end
    return redis.call("SETEX", key, tab.time, 1)
end

lock.unlock = function(key)
    return redis.call("DEL", key)
end

local tab = {}
for i, v in ipairs(KEYS) do
    tab[v] = ARGV[i]
end

return lock[tab.func](tab)