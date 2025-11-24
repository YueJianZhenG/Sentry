local Module = require("Module")

local Demo = Module()

require("TableUtil")
function Demo:OnAwake()

end

function Demo:OnComplete()

    --local sms = require("ali.sms")
    --printf("{}", sms.send("13028338223", { code = math.random(100000, 999999)}))
    ----
    ----local t1 = time.from("2025-11-20 00:00:00")
    --
    ---- printf("{} {}", t1, time.date(t1))
    --local mysql = require("MysqlProxyComponent")
    --local response = mysql:ExecuteInRead("query_order_page", 0)
    --table.print(response)

    --local tcp = require("net.tcp")
    --local json = require("util.json")
    --local client = tcp.connect("127.0.0.1", 2010, true)
    --
    --printf("{}", client:send(19020, json.encode({
    --    role_id = 10020
    --})))

end

return Demo