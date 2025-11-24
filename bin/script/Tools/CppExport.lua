local CppExport = { }

function CppExport.Run(documents, types, fields, name, descs)
    local content = "#pragma once\n"

    local config = CppExport.config
    local includes = { }
    local members = { }
    for i, field in pairs(fields) do
        local type = types[i]
        local member = {}
        member.type = type
        member.name = field
        if type == "int" then
            member.value = "0"
        elseif type == "long" or type == "time" then
            member.value = "0"
            member.type = "long long"
        elseif type == "string" then
            member.type = "std::string"
            includes["string"] = true
        elseif type == "int[]" then
            includes["vector"] = true
            member.type = "std::vector<int>"
        elseif type == "string[]" then
            includes["string"] = true
            includes["vector"] = true
            member.type = "std::vector<std::string>"
        elseif type == "map" then
            includes["string"] = true
            includes["unordered_map"] = true
            member.type = "std::unordered_map<string,string>"
        elseif type == "map<int,int>" then
            includes["string"] = true
            includes["unordered_map"] = true
            member.type = "std::unordered_map<int,int>"
        end
        table.insert(members, member)
    end

    for include, _ in pairs(includes) do
        content = content .. string.format("#include<%s>\n", include)
    end

    content = content .. "namespace " .. config.namespace .. "\n{\n";
    content = content .. string.format("%sstruct %s\n", string.rep("    ", 1), name);
    content = content .. string.rep("    ", 1) .. "{\n"

    for _, member in ipairs(members) do
        local desc = descs[member.name]
        if not member.value then
            content = content .. string.rep("  ", 2) .. string.format("    %s %s;", member.type, member.name)
        else
            content = content .. string.rep("  ", 2) .. string.format("    %s %s = %s;", member.type, member.name, member.value)
        end
        if desc and #desc > 0 then
            content = content .. " //" .. desc
        end
        content = content .. "\n"
    end
    content = content .. string.rep("    ", 1) .. "};\n"
    return content .. "}";
end

return CppExport