local JsonExport = {}

local table_is_array = function(tab)
    local len = #tab
    if len == 0 then
        return false;
    end
    local count = 0
    for i, v in pairs(tab) do
        if type(i) ~= "number" then
            return false
        end
        count = count + 1
    end
    return count == len
end

format_member = function(json, key, value)
    if type(value) == "table" then
        if table_is_array(value) then
            local jsonArray = json:add_array(key)
            for _, val in ipairs(value) do
                if type(val) == "table" then
                    format_member(jsonArray, nil, val)
                else
                    jsonArray:add_member(val)
                end
            end
        else
            local keys = { }
            for k, _ in pairs(value) do
                table.insert(keys, k)
            end
            table.sort(keys)
            local jsonObject = json:add_object(key)
            for _, key1 in ipairs(keys) do
                local item = value[key1]
                if type(item) == "table" then
                    format_member(jsonObject, key1, item)
                else
                    jsonObject:add_member(key1, item)
                end
            end
        end
    else
        json:add_member(key, value)
    end
end

function JsonExport.Run(documents)
    local count = 0
    local json = require("util.json")
    local jsonArray = json.new(true)
    for _, document in ipairs(documents) do
        local keys = { }
        for k, _ in pairs(document) do
            table.insert(keys, k)
        end
        table.sort(keys, function(a, b)
            return #a < #b
        end)
        local jsonObject = jsonArray:add_object(v)
        for _, key in ipairs(keys) do
            local value = document[key]
            format_member(jsonObject, key, value)
        end
        count = count + 1
    end
    if count <= 0 then
        return nil
    end
    return jsonArray:encode(true)
end

return JsonExport
