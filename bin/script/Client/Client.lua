require("XCode")
require("Coroutine")
local random = math.random
local log = require("Log")
local str_format = string.format
local timer = require("core.timer")
local Module = require("Module")
local Session = require("Common.Session")
local http = require("HttpComponent")

local tab_insert = table.insert
local coroutine_sleep = coroutine.sleep
local coroutine_create = coroutine.create
local coroutine_resume = coroutine.resume

local COUNT = os.getenv("APP_COUNT") or 1
local Main = Module()

local rpc_count = 0
local http_count = 0

function Main:Awake()

    self.count = 0
    self.login_count = 0
    self.accounts = { }
    self.sessions = { }
    for i = 1, COUNT do
        local account = string.format("yjz1995%s", i)
        tab_insert(self.accounts, { account = account, password = "123456", count = 0 })
    end
end

function Main:OnComplete()
    --local client = require("net.client")
    --local fd = client.Connect("127.0.0.1:7789")
    --client.Call(fd, "GateSystem.Login", "token")
    local t1 = time.ms()
    for i = 1, 10000 do
        timer.timeout(i * 1000, function()
            --local t2 = time.ms() - t1
            --log.Debug("index:{} ms:{:.2f}", i, t2 / 1000)
        end)
    end
end

local info = {
    user_id = 4,
    create_time = 8,
    player_name = "",
    level = 4,
    login_time = 8
}

local time_ms = time.ms
local host = "http://127.0.0.1:8088"
--local host = "http://43.143.239.75:8080"

local redis_time = 0
local redis_count = 0

local start_redis = function()

    redis_time = time_ms()
    local url = str_format("%s/db/redis", host)
    while true do
        http:Post(url, { "SET", "now_time", os.time() })
        http:Post(url, { "GET", "now_time" })
        http:Post(url, { "INFO"})
        redis_count = redis_count + 3
        if time_ms() - redis_time >= 5000 then
            print("[redis count] =>", redis_count)
            redis_time = time_ms()
        end
    end
end

local ping_time = 0
local ping_count = 0

local start_ping = function()

    ping_time = time_ms()
    while true do
        ping_count = ping_count + 1
        http:Get(str_format("%s/admin/ping", host))
        if time_ms() - ping_time >= 5000 then
            print("[ping count] =>", ping_count)
            ping_time = time_ms()
        end
    end
end

local hello_time = 0
local hello_count = 0

local start_hello = function()
    hello_time = time_ms()
    while true do
        hello_count = hello_count + 1
        http:Get(str_format("%s/hello", host))
        if time_ms() - hello_time >= 5000 then
            print("[hello count] =>", hello_count)
            hello_time = time_ms()
        end
    end
end

local mysql_time = 0
local mysql_count = 0

local start_mysql = function()
    mysql_time = time_ms()
    local tab = "yy.user_info_list"
    local url = str_format("%s/db/mysql", host)
    while true do

        local filter1 = { user_id = 10000 }
        local filter2 = { user_id = 10004 }
        local filter3 = { user_id = 10005 }

        http:Post(url, { func = "Update", args = { tab, filter1, { amount = 1000}}})
        http:Post(url, { func = "Update", args = { tab, filter2, { amount = 1000}}})
        http:Post(url, { func = "Update", args = { tab, filter3, { amount = 1000}}})

        http:Post(url, { func = "FindOne", args = { tab, filter1 }})
        http:Post(url, { func = "FindOne", args = { tab, filter2 }})
        http:Post(url, { func = "FindOne", args = { tab, filter3 }})


        mysql_count = mysql_count + 6
        if time_ms() - mysql_time >= 5000 then
            print("[mysql count] =>", mysql_count)
            mysql_time = time_ms()
        end
    end
end

local pgsql_time = 0
local pgsql_count = 0

local start_pgsql = function()
    local tab = "yy.user_info_list"
    local count = 0
    pgsql_time = time_ms()
    local url = str_format("%s/db/pgsql", host)
    while true do

        local filter1 = { user_id = 10000 }
        local filter2 = { user_id = 10004 }
        local filter3 = { user_id = 10005 }

        http:Post(url, { func = "Update", args = { tab, filter1, { amount = 1000}}})
        http:Post(url, { func = "Update", args = { tab, filter2, { amount = 1000}}})
        http:Post(url, { func = "Update", args = { tab, filter3, { amount = 1000}}})

        http:Post(url, { func = "FindOne", args = { tab, filter1 }})
        http:Post(url, { func = "FindOne", args = { tab, filter2 }})
        http:Post(url, { func = "FindOne", args = { tab, filter3 }})


        pgsql_count = pgsql_count + 6
        if time_ms() - pgsql_time >= 5000 then
            print("[pgsql count] =>", pgsql_count)
            pgsql_time = time_ms()
        end
    end
end

local sqlite_time = 0
local sqlite_count = 0
local start_sqlite = function()
    local tab = "user_info_list"
    sqlite_time = time_ms()
    local url = str_format("%s/db/sqlite", host)
    while true do

        local filter1 = { user_id = 10000 }
        local filter2 = { user_id = 10004 }
        local filter3 = { user_id = 10005 }

        http:Post(url, { func = "UpdateOne", args = { tab, filter1, { amount = 1000}}})
        http:Post(url, { func = "UpdateOne", args = { tab, filter2, { amount = 1000}}})
        http:Post(url, { func = "UpdateOne", args = { tab, filter3, { amount = 1000}}})

        http:Post(url, { func = "FindOne", args = { tab, filter1 }})
        http:Post(url, { func = "FindOne", args = { tab, filter2 }})
        http:Post(url, { func = "FindOne", args = { tab, filter3 }})


        sqlite_count = sqlite_count + 6
        if time_ms() - sqlite_time >= 5000 then
            print("[sqlite count] =>", sqlite_count)
            sqlite_time = time_ms()
        end
    end
end

local mongo_time = 0
local mongo_count = 0

local start_mongo = function()

    local tab = "yjz.user_info_list"
    mongo_time = time_ms()
    local url = str_format("%s/db/mongo", host)
    while true do

        local filter1 = { user_id = 10000 }
        local filter2 = { user_id = 10004 }
        local filter3 = { user_id = 10005 }

        http:Post(url, { func = "UpdateOne", args = { tab, filter1, { amount = 1000}}})
        http:Post(url, { func = "UpdateOne", args = { tab, filter2, { amount = 1000}}})
        http:Post(url, { func = "UpdateOne", args = { tab, filter3, { amount = 1000}}})

        http:Post(url, { func = "FindOne", args = { tab, filter1 }})
        http:Post(url, { func = "FindOne", args = { tab, filter2 }})
        http:Post(url, { func = "FindOne", args = { tab, filter3 }})

        mongo_count = mongo_count + 3
        if time_ms() - mongo_time >= 5000 then
            print("[mongo count] =>", mongo_count)
            mongo_time = time_ms()
        end
    end
end

local info_time = 0
local info_count = 0

local start_run_info = function()

    local url = str_format("%s/admin/info", host)
    while true do
        info_count = info_count + 1
        local response = http:Get(url).data
        if time_ms() - info_time >= 5000 then
            print("[run info count] => ", info_count)
            info_time = time_ms()
        end
    end
end

--function Main:OnComplete()
--    --for i = 1, 5 do
--    --    coroutine.start(start_ping)
--    --    coroutine.start(start_hello)
--    --
--    --    coroutine.start(start_redis)
--    --    coroutine.start(start_mysql)
--    --    coroutine.start(start_mongo)
--    --    coroutine.start(start_pgsql)
--    --    --coroutine.start(start_sqlite)
--    --    coroutine.start(start_run_info)
--    --end
--end




return Main