Main = {}
function Main.Awake()
   return true
end

Person = {}
Person.age = 10
Person.name = "xiaoming"
Person.arr = {}

for i = 1, 10 do
    table.insert(Person.arr, i)
end

function Main.Start()

    Log.Error("star main lua")
    local t1 = Time.GetNowSecTime();
    coroutine.sleep(1000)
    --Timer.AddTimer(5000, function()
    --    local t2 = Time.GetNowSecTime();
    --    print("&&&&&&&&&&&&&&&&&&&&&", t2 - t1)
    --end)

    --Sentry.Sleep(1500)
    return true
end
