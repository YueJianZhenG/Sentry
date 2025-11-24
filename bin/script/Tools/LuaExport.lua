local LuaExport = { }
local string_rep = string.rep
local table_insert = table.insert
local table_concat = table.concat
local string_format = string.format

function is_array(t)
    local len = #t
    if len == 0 then
        return false
    end
    local count = 0
    for _, v in pairs(t) do
        count = count + 1
    end
    return count == len;
end

local rep = "   "
local function serialize(t, indent)
    indent = indent or 0
    local padding = string_rep(rep, indent)  -- 每一层缩进两个空格

    local items = { "{" }
    if is_array(t) then
        for _, v in ipairs(t) do
            local values = { padding, rep }
            if type(v) == "table" then
                table_insert(values, serialize(v, indent + 1))
            elseif type(t) == "string" then
                table_insert(values, string_format("%q", v))
            else
                table_insert(values, tostring(v))
            end
            table_insert(values, ",")
            table_insert(items, table.concat(values))
        end
    else
        local keys = { }
        for k, v in pairs(t) do
            table_insert(keys, k)
        end
        table.sort(keys, function(a, b)
            return #a < #b
        end)
        for _, k in ipairs(keys) do
            local v = t[k]
            local key = k
            local values = { padding, rep, key, " = " }
            if type(v) == "table" then
                table_insert(values, serialize(v, indent + 1))
            elseif type(v) == "string" then
                table_insert(values, string_format("%q", v))
            else
                table_insert(values, tostring(v))
            end
            table_insert(values, ",")
            table_insert(items, table_concat(values))
        end
    end
    table_insert(items, padding .. "}")
    return table_concat(items, "\n")
end

function LuaExport.Run(documents)
    local result = "return \n"
    return result .. serialize(documents, 0) .. "\n"
end

return LuaExport