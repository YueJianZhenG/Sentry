local RpcService = require("RpcService")

local SceneSystem = RpcService()

require("TableUtil")
local log = require("Log")
local math = require("util.math")
local router = require("core.router")
local console = require("Console")
--local player = {
--    status = 0,
--    sock_id = 0,
--    player_id = 0,
--    position = { x = 0, y = 0, z = 0 },
--    orientation = { x = 0, y = 0, z = 0 }
--}

local ROOM_STATUS_NONE = 0
local ROOM_STATUS_GAME = 1

function SceneSystem:OnAwake()
    self.index = 0
    self.frame = 0
    self.players = { }
    self.status = ROOM_STATUS_NONE
    self.position = { }
    local pos = { x = -3.5, y = 0.5, z = 3.5}
    for x = 1, 3 do
        local pos1 = {
            x = pos.x,
            y = pos.y,
            z = pos.z - x * 2
        }
        for y = 1, 3 do
            table.insert(self.position, {
                x = pos1.x + y * 2,
                y = pos1.y,
                z = pos1.z
            })
        end
    end
end

function SceneSystem:RandVector3(min, max)
    local num1 = math.random(min, max)
    local num2 = math.random(min, max)
    return {
        x = num1,
        y = 0.5,
        z = num2
    }
end

function SceneSystem:OnFrameUpdate()

    if self.status ~= ROOM_STATUS_GAME then
        return
    end
    local list = { }
    self.frame = self.frame + 1
    for _, player in pairs(self.players) do
        if #player.inputs > 0 then
            local message = {
                inputs = player.inputs,
                player_id = player.player_id
            }
            table.insert(list, message)
        end
    end
    if #list > 0 then
        for sock_id, player in pairs(self.players) do
            local code = router.send(sock_id, "Scene.OnChange", list)
            --log.Debug("[{}] ({}) => {}", self.frame, player.player_id, #list)
            player.inputs = { }
        end
    end
end

function SceneSystem:Login(request)
    local socketId = request.socketId
    if self.players[socketId] ~= nil then
        return XCode.PlayerOnLine
    end
    self.index = self.index + 1
    local player = {
        status = 0,
        inputs = { }, --当前输入
        sock_id = socketId,
        player_id = self.index,
        frame_buffer = { }, --帧数据
        pos = self.position[self.index],
        rot = { x = 0, y = 0, z = 0 }
    }
    self.players[socketId] = player
    log.Debug("player:{} login ok", player.player_id)
    return XCode.Ok, { player_id = player.player_id }
end

function SceneSystem:PlayerCount()
    local count = 0
    for _, _ in pairs(self.players) do
        count = count + 1
    end
    return count
end

function SceneSystem:Start(request)
    --if self:PlayerCount() < 3 then
    --    return XCode.Failure
    --end
    local func = "Scene.Create"
    for _, player in pairs(self.players) do
        local message = {
            pos = player.pos,
            rot = player.rot,
            player_id = player.player_id
        }
        for sock_id, _ in pairs(self.players) do
            router.send(sock_id, func, message)
        end
    end
    self.frame = 0
    self.status = ROOM_STATUS_GAME
    return XCode.Ok
end

function SceneSystem:Broadcast(request)
    local frameInfo = request.data
    local socketId = request.socketId
    local player = self.players[socketId]
    if player == nil then
        return XCode.NotFindUser
    end
    for _, input in ipairs(frameInfo.inputs) do
        table.insert(player.inputs, input)
    end
    return XCode.Ok
end

function SceneSystem:Logout(request)
    local socketId = request.socketId
    local player = self.players[socketId]
    if player == nil then
        return XCode.NotFindUser
    end
    local message = {
        player_id = player.player_id
    }
    for _, user in pairs(self.players) do
        local sockId = user.sock_id
        local code = router.send(sockId, "Scene.Remove", message)
        if code == XCode.Ok then
            log.Warning("在客户端{}移除entity", user.player_id)
        end
    end
    self.players[socketId] = nil
    log.Info("player:{} 退出", player.player_id)
    return XCode.Ok
end

return SceneSystem