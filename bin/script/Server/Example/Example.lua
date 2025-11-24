
local log = require("Log")
local RpcService = require("RpcService")

local Example = RpcService()

function Example:Ping(request)
    print("==========", request)
    return XCode.Ok, request
end

return Example