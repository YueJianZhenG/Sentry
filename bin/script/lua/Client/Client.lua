
Client = {}

function Client.Test(tab)
    print(Json.Encode(tab))
end

local account = "yjz0995"
local password = "123456"
local phoneNum = 13716061995
local clientComponent = App.GetComponent("ClientComponent")
local loginComponent = require("component.LoginComponent")

local callCount = 0
local waitCount = 0
local CallMongo = function()
    
    while true do
        waitCount = waitCount + 1
        clientComponent:Call("MongoService.Query", {
            tab = "user.account",
            json = Json.Encode({
                _id = "646585122@qq.com"
            }),
            limit = 1
        })
        waitCount = waitCount - 1
        callCount = callCount + 1
    end
end

local CallChat = function()
    
    while true do
        waitCount = waitCount + 1
        clientComponent:Call("ChatService.Ping", {
            user_id = 1122, msg_type = 1, message = "hello"
        })
        waitCount = waitCount - 1
        callCount = callCount + 1
    end
end

local TestHttp = function()
    local count = 5000
    local phoneNum = 100
    local passwd = "yjz199595"
    local account = "%d@qq.com"
    while true do
        waitCount = waitCount + 1
        local data1 = string.format(account, count)
        local data2 = passwd .. tostring(count)
        local data3 = phoneNum + count
        local res = loginComponent.Register(data1,data2, data3)
        --table.print(res)
        waitCount = waitCount - 1
        callCount = callCount + 1
        --loginComponent.Login(data1, data2)
    end
end

function Client.Start()

    loginComponent.Awake()
    loginComponent.Register(account, password, phoneNum)

    local loginInfo = loginComponent.Login(account, password)
    if loginInfo == nil or loginInfo.code ~= XCode.Successful then
        Log.Error("使用http登陆失败")
        return false
    end

    table.print(loginInfo)
    local address = loginInfo.data.address
    if not clientComponent:StartConnectAsync(address) then
        Log.Error("连接网关服务器 [" , address, "] 失败")
        return false
    end
    Log.Debug("连接网关服务器[" , address, "]成功")

    local code, _ = clientComponent:Auth(loginInfo.data.token)
    if code ~= XCode.Successful then
        Log.Error("user auth failure")
        return false
    end
    coroutine.start(CallChat)
    coroutine.start(CallChat)
    coroutine.start(CallChat)

    coroutine.start(CallMongo)
    coroutine.start(CallMongo)
    coroutine.start(CallMongo)
    coroutine.start(TestHttp)
    coroutine.start(TestHttp)
    return true
end

Client.Update = function(tick)
    Log.Warning(string.format("count = %d   wait = %d", callCount, waitCount))
end

return Client