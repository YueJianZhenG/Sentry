
DataServer = { }
local messageComponent = App.GetComponent("ProtoComponent")
local function CreateTable(tabName, keys)
    local code = App.Call("MysqlService.Create", {
        keys = keys,
        data = messageComponent:New(tabName)
    })
    return code == XCode.Successful
end

function DataServer.OnLocalComplete()
    CreateTable("user.account_info", {"account"})
end