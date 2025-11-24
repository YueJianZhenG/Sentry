local proto = require("Proto")
local Module = require("Module")
local json = require("util.json")
local http = require("HttpComponent")
local redis = require("RedisComponent")
local mongo = require("MongoProxyComponent")

local Main = Module()

SetMember(Main, "count", 1)

local timer = require("core.timer")

local timerId
local count = 0
function Main:OnAwake()

end

function Main:OnStart()

end

function Main:InsertData()
    local tab = "user_info_list"

    local mysql = require("MysqlProxyComponent")
    local pgsql = require("PgsqlProxyComponent")
    local sqlite = require("SqliteComponent")
    local path = string.format("%s/mongo/20250606/%s.json", os.dir, tab)
   -- local path = "C:/Users/64658/Desktop/yy/gitee/aes/bin/mongo/20250606/user_info_list.json"

    if pgsql:Count("yy." .. tab) > 0 then
        return
    end

    local doneCount = 0
    local t1 = time.ms()
    local fs = io.open(path)
    while true do
        local documents = { }
        for i = 1, 500 do
            local line = fs:read("L")
            if line == nil then
                goto LOOP_END
            end
            local document = json.decode(line)
            if document == nil then
                goto LOOP_END
            end
            local info = {
                _id = document._id,
                sex = document.sex,
                icon = document.icon,
                user_id = document.user_id,
                public_id = document.public_id or "",
                city = document.city,
                amount = document.amount or 0,
                club_id = document.club_id or 0,
                card_id = document.card_id or 0,
                unionid = document.unionid or "",
                city_name = document.city_name or "",
                permission = document.permission or 0,
                vip_time = document.vip_time or 0,
                create_time = document.create_time or 0,
                activity_list = document.activity_list or {},
                user_desc = document.desc or "",
                nick = document.nick or ""
            }
            if #info.nick > 0 then
                table.insert(documents, info)
            end
        end
        local rows = pgsql:InsertBatch(tab, documents)
        if rows > 0 then
            doneCount = doneCount + rows
            print("insert count => ", doneCount)
        end

    end
    :: LOOP_END ::
    print(string.format("[%sms] 插入完成", time.ms() - t1))
end

function Main:OnComplete()

    local oss = require("ali.oss")

    --local res = oss.sign({
    --    file_name = "kabi/bg",
    --    file_type = "image/png",
    --    max_length = 1024 * 1024 * 5,
    --    expiration = os.time() + 10,
    --    limit_type = { "image/png"},
    --    upload_dir = ""
    --})
    --table.print(res)
    local path = "C:/Users/64658/Desktop/project/美术切图/主界面/worldmap.jpg";
    table.print(oss.upload("game-huanhu", "C:/Users/64658/Documents/huanhu/bg.png", "kabi/bg.png"))

    --self:InsertData()
    --local tab = "user_list"
    --local pipeline = {
    --    { ["$match"] = { city = 3201 } },
    --    { ["$sample"] = { size = 10 } },
    --    { ["$project"] = { _id = 0, user_id = 1, nick = 1 } }
    --}
    --
    --local pipeline1 = {
    --    { ["$match"] = { amount = { ["$gt"] = 0 } }},
    --    { ["$group"] = { _id = "$city", total = { ["$sum"] = "$amount"} }}
    --}
    --
    --local response1 = mongo:Aggregate(tab, pipeline1)
    --local response2 = mongo:RandomDocument(tab, 10, { city = 3201}, { "user_id", "nick"})
    --print(response1)
    --print(response2)
    --
    --local response = mongo:Run(tab, "aggregate", {
    --    cursor = { batchSize = 1000 },
    --    pipeline = pipeline })
    --print(response.cursor.firstBatch)


    --local ms = require("MeiliSearchComponent")
    --local path = string.format("C:/Users/64658/Desktop/yy/gitee/aes/bin/mongo/20250606/%s.json", tab)
    --
    ----print(ms:Setting(tab, {
    ----    search = { "desc", "nick", "city_name" }
    ----}))
    --
    --local fs = io.open(path)
    --while true do
    --    local documents = { }
    --    for i = 1, 100 do
    --        local line = fs:read("L")
    --        if line == nil then
    --            goto LOOP_END
    --        end
    --        table.insert(documents, json.decode(line))
    --    end
    --    ms:Set(tab, documents)
    --end
    --:: LOOP_END ::
    --print("insert document ok")
    --
    --local response2 = ms:Search(tab, {
    --    q = "10086",
    --    limit = 5,
    --    fields = { "user_id", "nick", "city_name", "desc" }
    --})
    --table.print(response2)

    --local mongo1 = require("MongoProxyComponent")
    --print(mongo1:Aggregate(tab, pipeline))
    --mongo1:Deletes(tab, { { user_id = 33183 }, { user_id = 33184 }, { user_id = 33185 } })
    --
    --local res1 = mongo:Run("admin", "isMaster")

    --local response = mongo1:ListDatabases()
    --for _, info in ipairs(response) do
    --    local res1 = mongo1:ListCollections(info.name)
    --    table.print(res1)
    --end

    --local pb = require("Proto")
    --
    --pb.Import("mysql/comment.proto")
    local mysql = require("MysqlProxyComponent")
    local pgsql = require("PgsqlProxyComponent")
    local sqlite = require("SqliteComponent")

    --sqlite:Set("player:10001", { id = 10001, name = "小明", level = 10, attr = { hp = 100, atk = 200}})

    --print(sqlite:Get("player:10001"))

    --sqlite:Build("admin_list_query", "SELECT * FROM admin_list")
    --sqlite:Build("admin_list_insert", "INSERT INTO admin_list (name,user_id,account,password,permission,create_time)VALUES(?,?,?,?,?,?)")

    --sqlite:Invoke("admin_list_insert", "管理员", 1, "root", "123456", 100, os.time())
    --print(sqlite:Invoke("admin_list_query"))
    ----mysql:Drop("yy.user_info_list")
    ----mysql:Create("yy.user_info_list", { "_id" })
    ----
    --local path = "C:/Users/64658/Desktop/yy/gitee/aes/bin/mongo/20250606/user_info_list.json"
    --
    --local doneCount = 0
    --local fs = io.open(path)
    --while true do
    --    local documents = { }
    --    for i = 1, 100 do
    --        local line = fs:read("L")
    --        if line == nil then
    --            goto LOOP_END
    --        end
    --        local document = json.decode(line)
    --        if document == nil then
    --            goto LOOP_END
    --        end
    --        local info = {
    --            _id = document._id,
    --            sex = document.sex,
    --            icon = document.icon,
    --            user_id = document.user_id,
    --            public_id = document.public_id or "",
    --            city = document.city,
    --            amount = document.amount or 0,
    --            club_id = document.club_id or 0,
    --            card_id = document.card_id or 0,
    --            unionid = document.unionid or "",
    --            city_name = document.city_name or "",
    --            permission = document.permission or 0,
    --            vip_time = document.vip_time or 0,
    --            create_time = document.create_time or 0,
    --            activity_list = document.activity_list or {},
    --            user_desc = document.desc or "",
    --            nick = document.nick or ""
    --        }
    --        table.insert(documents, info)
    --    end
    --    local rows = mysql:InsertBatch("user_info_list", documents)
    --    if rows > 0 then
    --        doneCount = doneCount + rows
    --        print("insert count => ", doneCount)
    --    end
    --
    --end
    --:: LOOP_END ::

    --print(mysql:RunInRead("SELECT COUNT(*) FROM yy.user_info_list"))

    --mysql:SetIndex("yy.user_info_list", "user_id", true)

    --
    --print(mysql:Count("yy.user_info_list"))
    --print(mysql:Func("yy.user_info_list", "SUM", "amount", "amount>0"))
    --print(mysql:Func("yy.user_info_list", "AVG", "amount", "amount>0"))
    --print(mysql:Func("yy.user_info_list", "MIN", "amount", "amount>0"))
    --print(mysql:Func("yy.user_info_list", "MAX", "amount", "amount>0"))
    --
    --mysql:Inc("yy.user_info_list", "amount", 50, { user_id = 10000 }, 1)
    --
    --table.print(mysql:RunInRead("DESCRIBE yy.user_info_list"))
    --table.print(mysql:FindPage("yy.user_info_list", nil, { user_id = "ASC"}, { "user_id", "nick", "amount"}, 1, 10))

    --local fields = { "column_name", "data_type" }
    --table.print(pgsql:Find("information_schema.columns", {
    --    table_name = "order_list",
    --}, { ordinal_position = "ASC" }, fields, 0))

    --print(mysql:Find("yy.user_info_list", nil, nil, nil, 10))

    --print(mysql:RunInRead("DESCRIBE user_info_list;"))

    --table.print(mysql:Find("user_info_list", { permission = 10 }, nil, nil, 100))
    --for i = 1, 1000 do
    --    coroutine.start(function()
    --        for i = 1, 100 do
    --            mysql:InsertOne("player_list", {
    --                level = math.random(1, 50),
    --                new_player = true,
    --                nick = string.range(32),
    --                create_time = os.time(),
    --                sex = math.random(0, 2),
    --                attr = { akt = math.random(100, 999), hp = math.random(1000, 3000)},
    --                icon = string.format("http://%s.png", string.range(64))
    --            })
    --        end
    --    end)
    --end
    --table.print(sqlite:TableInfo("local_data"))

    --mysql:RunInRead("PREPARE query_user_info FROM 'SELECT * FROM user_info_list WHERE user_id=?'")

    --print(mysql:Inc("user_info_list", { user_id = 43769}, "amount", 1))
    --
    --print(mysql:ExecuteInRead("query_simple_info", {
    --    user_id = 10000
    --}))

    --print(mysql:FindOne("user_info_list", { user_id = 10000 }, {"user_id", "amount"}))
    --print(mysql:FindOne("user_info_list", { user_id = 10004 }, {"user_id", "amount"}))
    --
    --table.print(mysql:Commit({
    --    "SET @cur_time=NOW()",
    --    "SET @source_last_amount = (SELECT amount FROM user_info_list WHERE user_id=10004)",
    --    "SET @target_last_amount = (SELECT amount FROM user_info_list WHERE user_id=10000)",
    --    "UPDATE user_info_list SET amount=amount+100 WHERE user_id=10000",
    --    "UPDATE user_info_list SET amount=amount-100 WHERE user_id=10004",
    --    "SET @source_name = (SELECT nick FROM user_info_list WHERE user_id=10004)",
    --    "SET @target_name = (SELECT nick FROM user_info_list WHERE user_id=10000)",
    --    "SET @source_next_amount = (SELECT amount FROM user_info_list WHERE user_id=10004)",
    --    "SET @target_next_amount = (SELECT amount FROM user_info_list WHERE user_id=10000)",
    --    "SET @source_reason=CONCAT('[收款]来自',@source_name)",
    --    "SET @target_reason=CONCAT('[转账]转给',@target_name)",
    --    "INSERT INTO amount_record_list(user_id,amount,create_at,reason,last_amount,next_amount)VALUES(10000,100,@cur_time,@source_reason,@source_last_amount,@source_next_amount)",
    --    "INSERT INTO amount_record_list(user_id,amount,create_at,reason,last_amount,next_amount)VALUES(10004,-100,@cur_time,@target_reason,@target_last_amount,@target_next_amount)"
    --}))
    --
    --print(mysql:FindOne("user_info_list", { user_id = 10000 }, {"user_id", "amount"}))
    --print(mysql:FindOne("user_info_list", { user_id = 10004 }, {"user_id", "amount"}))

    --local filter = { user_id = 10004 }
    --table.print(mysql:Func("user_info_list", "COUNT", "*", "amount>0"))
    --table.print(mysql:Func("user_info_list", "SUM", "amount", nil, "city"))
    --table.print(msyql:RunInRead("SELECT SUM(amount) AS amount,COUNT(*) AS count,city FROM user_info_list GROUP BY city"))
    --table.print(pgsql:RunInRead(string.format("SELECT city,SUM(amount) AS amount FROM user_info_list WHERE amount>0 GROUP BY city ORDER BY amount DESC")))
    --table.print(pgsql:Find("user_info_list", { user_id = { 10000, 10004, 10005}}, { "nick", "user_id", "amount"}))
    --for i = 1, 10 do
    --    print(pgsql:Inc("user_info_list", filter, "amount", 10))
    --    print(pgsql:FindOne("user_info_list", filter, {"amount", "user_id"}))
    --end
    --local documents = { }
    --for i = 1, 10 do
    --    table.insert(documents, {
    --        player_id = 1000 + 5 + i,
    --        level = math.random(1, 100),
    --        new_player = false,
    --        nick = string.range(32),
    --        attr = {
    --            hp = math.random(100, 1000)
    --        }
    --    })
    --end
    --pgsql:InsertBatch("player_list", documents)
    --
    --table.print(pgsql:Find("player_list"))

    --table.print(mysql:Find("user_info_list", nil, "amount", { "amount", "user_id", "nick"}, 10))
    --
    --pgsql:UpdateBatch({
    --    { tab = "user_info_list", filter = { user_id = 10004 }, document = "amount = amount - 10"},
    --    { tab = "user_info_list", filter = { user_id = 10000 }, document = "amount = amount + 10"}
    --})
    --print(pgsql:Find("user_info_list", { user_id = { 10000, 10004 }}, nil, { "amount", "user_id"}, 2))
    --print(mysql:Like("user_info_list", "nick", "慢%", {"user_id", "nick", "amount"}, "amount", 10))
    --pgsql:Update("user_info_list", filter, { amount = 100 * 100})
    --print(pgsql:FindOne("user_info_list", filter, { "user_id", "amount", "nick"}))
    --pgsql:Delete("user_info_list", filter)
    --mysql:RunInRead("SET GLOBAL long_query_time=2")
    --print(mysql:RunInRead("SHOW VARIABLES LIKE 'long_query_time'"))

    --print(mongo:DeleteOne("user_info_list", { user_id = 10005}))

    --print(redis:HashSet("player_list", playerInfo.player_id, playerInfo))

    --print(redis:Run("HGETALL", "player_list:10000"))

    --sqlite:Set("player_list:10000", playerInfo)

    --sqlite:SetTimeout("player_list:10000", 10)
    --print(sqlite:Get("player_list:10000"))

    --local mongo_client = require("db.mongo")
    --print(mongo_client.run("user_info_list", "find", "filter", { user_id = 10004 }))
    --print(mongo_client.run("user_info_list", "findAndModify",
    --        "query", { user_id = 10004 },
    --        "update",  { amount = 1000 }, "fields", { user_id = 1, amount = 1, nick = 1}, "options", { returnDocument = "after"} ))
end

function Main:OnUpdate()

end

return Main
