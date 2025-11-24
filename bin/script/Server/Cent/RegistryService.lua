

local router = require("core.router")
local RpcService = require("RpcService")
local RegistryService = assert(RpcService())

function RegistryService:OnAwake()
    self.nodes = { }
end

function RegistryService:Add(request)
    local info = request.data
    info.last_time = os.time()
    info.sockId = request.socketId
    self.nodes[info.id] = info
    for _, item in pairs(self.nodes) do
        if item.sockId and item.sockId > 0 then
            router.send(item.sockId, "NodeSystem.Add", info)
        end
        router.send(info.sockId, "NodeSystem.Add", item)
    end

    return XCode.Ok
end

function RegistryService:Del(request)
    local id = request.data
    if id == nil then
        return XCode.Failure
    end
    self.nodes[id] = nil
    return XCode.Ok
end

function RegistryService:Ping(request)
    local id = request.data
    local info = self.nodes[id]
    if info == nil then
        return XCode.Failure
    end
    info.last_time = os.time()
    return XCode.Ok
end

function RegistryService:Find(request)
    local name = request.data

    return XCode.Ok
end

return RegistryService