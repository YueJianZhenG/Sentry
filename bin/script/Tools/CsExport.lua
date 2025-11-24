local CppExport = { }

function CppExport.Run(documents, types, fields, name, descs)
    local content = "using System;\n"

    local config = CppExport.config
    local includes = { }
    local members = { }
    for i, field in pairs(fields) do
        local type = types[i]:gsub("%s+", "")
        local member = {}
        member.type = type
        member.name = field
        if type == "int[]" then
            includes["System.Collections.Generic"] = true
            member.type = "List<int>"
        elseif type == "string[]" then
            includes["System.Collections.Generic"] = true
            member.type = "List<string>"
        elseif type == "map" then
            includes["System.Collections.Generic"] = true
            member.type = "Dictionary<string,string>"
        elseif type == "map<int,int>" then
            includes["System.Collections.Generic"] = true
            member.type = "Dictionary<int,int>"
        elseif type == "time" then
            member.type = "long"
        end
        table.insert(members, member)
    end

    for include, _ in pairs(includes) do
        content = content .. string.format("using %s;\n", include)
    end

    content = content .. "namespace " .. config.namespace .. "\n{\n";
    content = content .. string.format("%spublic class %s\n", string.rep("    ", 1), name);
    content = content .. string.rep("    ", 1) .. "{\n"

    for _, member in ipairs(members) do
        if #member.type > 0 and #member.name > 0 then
            local desc = descs[member.name]
            if not member.value then
                content = content .. string.rep("  ", 2) .. string.format("    public %s %s;", member.type, member.name)
            else
                content = content .. string.rep("  ", 2) .. string.format("    public %s %s = %s;", member.type, member.name, member.value)
            end
            if desc and #desc > 0 then
                content = content .. " //" .. desc
            end
            content = content .. "\n"
        end
    end
    content = content .. string.rep("    ", 1) .. "};\n"
    return content .. "}";
end

return CppExport