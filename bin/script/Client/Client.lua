
Client = {}
local messageComponent
function Client.Awake()
    messageComponent = App.GetComponent("MessageComponent")
    local msg = messageComponent:New("c2s.GateAuth.Request", {
        token = "112233"
    })
    print(msg, type(msg))

    LoginComponent.Awake()
end

function Client.Test(tab)
    print(Json.Encode(tab))
end

local account = "yjz1995"
local password = "123456"
local phoneNum = 13716061995

function Client.Start()


    local httpComponent = App.GetComponent("HttpComponent")
    local response = httpComponent:Get("http://127.0.0.1:80/source/file/files")
    print(response.data)
    local code = httpComponent:Download("http://127.0.0.1:80/source/file/download?2022-06-04/all.log", "./log/hello.log")

    local clientComponent = App.GetComponent("ClientComponent")

    LoginComponent.Register(account, password, phoneNum)

    local loginInfo = LoginComponent.Login(account, password)

    if not clientComponent:StartConnect(loginInfo.address) then
        Log.Error("connect [" , loginInfo, "] failure")
        return
    end
    Log.Debug("connect [" , loginInfo.address, "] successful")

    local authMessage = messageComponent:New("c2s.GateAuth.Request", {
        token = loginInfo.token
    })

    clientComponent:Call("GateService.Auth", authMessage)


    while true do

        local testtMessage = messageComponent:New("lua.Test",{
            address = "127.0.0.1:7799",  service =
            {
                "LoginComponent",
                "RegisterComponent"
            }
        })

        local code, res = clientComponent:Call("ChatService.Test", testtMessage)
        Log.Error("code = ", code, Json.Encode(res))
    end
end