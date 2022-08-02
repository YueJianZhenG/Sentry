
Client = {}

function Client.Test(tab)
    print(Json.Encode(tab))
end

local account = "yjz1995"
local password = "123456"
local phoneNum = 13716061995

function Test()
    for i = 1, 10 do
        coroutine.start(LoopLogin)
    end
    return false
end

function Client.StartLogic()

    LoginComponent.Awake()

    if Test() == false then
        return
    end


    local clientComponent = App.GetComponent("ClientComponent")
    LoginComponent.Register(account, password, phoneNum)

    local loginInfo = LoginComponent.Login(account, password)
    if loginInfo.code ~= XCode.Successful then
        Log.Error("使用http登陆失败")
        return
    end

    local address = loginInfo.data.address
    if not clientComponent:StartConnectAsync(address) then
        Log.Error("连接网关服务器 [" , address, "] 失败")
        return
    end
    Log.Debug("连接网关服务器[" , address, "]成功")

    local code, _ = clientComponent:Call("GateService.Auth", "c2s.auth.request", {
        token = loginInfo.data.token
    })
    if code ~= XCode.Successful then
        Log.Error("user auth failure")
        return
    end
    --LoginComponent.Register(account, password, phoneNum)
    --LoginComponent.Login(account, password)

    local res, response = clientComponent:Call("ChatService.Chat", "c2s.chat.request", {
        user_id = 1122, msg_type = 1, message = "hello"
    })
    Log.Error("code = ", res, Json.Encode(response))

end

local loginCount = 0
local registerCount = 0

function LoopRegister()
    while true do
        local t1 = Time.GetNowMilTime()
        LoginComponent.Register(account, password, phoneNum)
        registerCount = registerCount + 1
        print(string.format("register use time = [%dms] count = %d", Time.GetNowMilTime() - t1, registerCount))
    end
end

function LoopLogin()
    while true do
        local t1 = Time.GetNowMilTime()
        LoginComponent.Login(account, password)
        loginCount = loginCount + 1
        print(string.format("login use time = [%dms] count = %d", Time.GetNowMilTime() - t1, loginCount))
    end
end

function LoopCall()
    local clientComponent = App.GetComponent("ClientComponent")
    while true do
        local t1 = Time.GetNowMilTime()
        local res, response = clientComponent:Call("ChatService.Chat", "c2s.chat.request", {
            user_id = 1122, msg_type = 1, message = "hello"
        })
        Log.Error("code = ", res, Json.Encode(response), " time = ", Time.GetNowMilTime() - t1)
    end
end