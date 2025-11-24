local TypeDecode = { }
local string_split = function(str, reps)
    local resultStrList = {}
    string.gsub(str,'[^'..reps..']+',function ( w )
        table.insert(resultStrList,w)
    end)
    return resultStrList
end

local json = require("util.json")

local split_array = function(value)

    local result = { }
    local separators = ";|" --根据;或者|分割
    local pattern = "([^" .. separators .. "]+)"
    for match in value:gmatch(pattern) do
        table.insert(result, match)
    end
    return result
end

TypeDecode["bool"] = function(value)
    local str = string.lower(value)
    return str == "true" or str == "1"
end

TypeDecode["json"] = function(value)
    return json.decode(value)
end

TypeDecode["int"] = function(value)
    if value == nil or #value == 0 then
        return 0
    end
    return tonumber(value)
end

TypeDecode["long"] = function(value)
    if value == nil or #value == 0 then
        return 0
    end
    return tonumber(value)
end

TypeDecode["float"] = function(value)
    if value == nil or #value == 0 then
        return 0
    end
    return tonumber(value)
end

TypeDecode["double"] = function(value)
    if value == nil or #value == 0 then
        return 0
    end
    return tonumber(value)
end

TypeDecode["int[]"] = function(value)
    local result = { }
    local source = split_array(value)
    for _, val in ipairs(source) do
        table.insert(result, math.tointeger(val))
    end
    return result
end

TypeDecode["string"] = function(value)
    return value or ""
end

TypeDecode["time"] = function(value)
    if value == nil or #value <= 0 then
        return 0
    end
    return time.from(value)
end

TypeDecode["string[]"] = function(value)
    return split_array(value)
end

TypeDecode["map"] = function(value)
    local result = { }
    local split = split_array(value)
    for _, val in ipairs(split) do
        local items = string_split(val, ":")
        if items == nil or #items ~= 2 then
            error("解析map结构失败" .. val)
        end
        local key = items[1]
        local content = items[2]
        result[key] = content
    end
    return result
end

TypeDecode["pos2"] = function(value)
    local result = split_array(value)
    if #result ~= 2 then
        error(string.format("[%s] pos2", value))
    end
    return {
        x = tonumber(result[1]),
        y = tonumber(result[2]),
    }
end

TypeDecode["pos3"] = function(value)
    local result = split_array(value)
    if #result ~= 3 then
        error(string.format("[%s] pos3", value))
    end
    return {
        x = tonumber(result[1]),
        y = tonumber(result[2]),
        z = tonumber(result[3])
    }
end

TypeDecode["map<string,int>"] = function(value)
    local result = { }
    local split = split_array(value)
    for _, val in ipairs(split) do
        local items = string_split(val, ":")
        if items == nil or #items ~= 2 then
            error("解析map结构失败" .. val)
        end
        local key = items[1]
        result[key] = math.tointeger(items[2])
    end
    return result
end

--模糊匹配
TypeDecode.match = function(type, value)
    local match_type, num = type:match("^(%a+)%[(%d+)%]$") -- 匹配 int[] string[] 定长数组
    if match_type and num then
        local count = tonumber(num)
        local result = split_array(value)
        if #result ~= count then
            error(string.format("(%s:%s) count:%s=>%s", type, value, count, #result))
        end
        local response = { }
        for _, str in ipairs(result) do
            local func = TypeDecode[match_type]
            table.insert(response, func(str))
        end
        return response
    end
end

return TypeDecode