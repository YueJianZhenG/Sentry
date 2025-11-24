
#define ENABLE_UNSAFE_CODE
#if ENABLE_UNSAFE_CODE
#pragma warning disable 414
#pragma warning disable 649
#endif

using System;
using System.Collections.Generic;
using c2s;
using rpc;
using s2c;
using UnityEngine;



public class Main : MonoBehaviour
{
    // Start is called before the first frame update
    [SerializeField] private GameObject entity;

    private GameObject player;
    private Vector3 MoveDir;
    private NetComponent _netComponent;
    private Dictionary<long, Entity> _gameObjects = new Dictionary<long, Entity>();

    private void Start()
    {
        //string address = "127.0.0.1:1122";
        string address = "43.143.239.75:7788";
        //this._netComponent = new NetComponent(new WsClient($"ws://{address}"));
        this._netComponent = new NetComponent(new kcp.Client(address));
        this._netComponent.Register("Scene", this);

        Application.targetFrameRate = 60;
    }

    private double fps = 0;
    private long count = 0;
    private long sumTime = 0;
    private long lastReceiveTime = 0;

    private void FixedUpdate()
    {
        if (this._netComponent != null)
        {
            this._netComponent.OnUpdate();
        }
    }

    public void OnChange(List<Sync> messages)
    {
        foreach (Sync syncInfo in messages)
        {
            if (syncInfo.player_id == _netComponent.playerId)
            {
                this.count++;
                long nowTime = TimeHelper.ClientNow();
                long costTime = nowTime - this.lastReceiveTime;
                sumTime += costTime;
                this.lastReceiveTime = nowTime;
            }

            if (this._gameObjects.TryGetValue(syncInfo.player_id, out Entity entityObject))
            {
                entityObject.OnChange(syncInfo);
            }
            // Debug.LogError($"receive message => {JsonConvert.SerializeObject(syncInfo)}");
        }
    }

    public void Create(CreateMessage message)
    {
        GameObject entityObject = GameObject.Instantiate(this.entity, this.transform);

        entityObject.name = $"[player:{message.player_id}]";
        Entity entityCom = entityObject.GetComponent<Entity>();
        entityObject.GetComponent<Rigidbody>().freezeRotation = true;
        entityObject.transform.position = new Vector3()
        {
            x = message.pos.x,
            y = message.pos.y,
            z = message.pos.z
        };
        Vector3 rotate = new Vector3()
        {
            x = message.rot.x,
            y = message.rot.y,
            z = message.rot.z
        };
        Quaternion targetRotation = Quaternion.LookRotation(-rotate);
        this.transform.rotation = Quaternion.Lerp(this.transform.rotation, targetRotation, 1f);

        if (message.player_id == _netComponent.playerId)
        {
            this.player = entityObject;
        }

        this._gameObjects.Add(message.player_id, entityCom);
    }

    public void Remove(LoginResponse response)
    {
        if (this._gameObjects.TryGetValue(response.player_id, out Entity _entity))
        {
            GameObject.Destroy(_entity.gameObject);
            this._gameObjects.Remove(response.player_id);
        }
    }

    private int frame = 0;
    private int maxFrame = 0;
    private long lastUpdateTime = 0;
    private Sync inputMessage = new Sync();

    private long lastFrameUpdateTime = 0;

    private long t1 = 0;
    private void Update()
    {
        if (this.player == null)
        {
            return;
        }

        this.frame++;
        float y = Input.GetAxis("Vertical");
        float x = Input.GetAxis("Horizontal");

        if (x != 0.0f || y != 0.0f)
        {
            inputMessage.inputs.Add(new Vec2()
            {
                x = x,
                y = y
            });
        }
        
        long nowMs = TimeHelper.ClientNow();
        if (nowMs - this.lastFrameUpdateTime >= 66)
        {
            if (inputMessage.inputs.Count > 0)
            {
                Debug.Log($"操作数量 => {inputMessage.inputs.Count}");
                _netComponent.Send("SceneSystem.Broadcast", inputMessage).Coroutine();
                this.inputMessage.inputs.Clear();
            }
            this.lastFrameUpdateTime = nowMs;
        }

        if (DateTime.Now.Second - lastUpdateTime >= 1)
        {
            this.fps = this.sumTime / (double)this.count;
            lastUpdateTime = DateTime.Now.Second;
            this.sumTime = 0;
            this.count = 0;
        }
    }

    private void OnGUI()
    {
        GUI.Label(new Rect(30, 0, 200, 60), $"{Application.targetFrameRate}ms", new GUIStyle()
        {
            fontSize = 40
        });

        if (this._netComponent.playerId == 0)
        {
            if (GUI.Button(new Rect(30, 70, 200, 60), "登录"))
            {
                this._netComponent.Login().Coroutine();
            }
        }
       

        if (GUI.Button(new Rect(30, 130, 200, 60), "退出"))
        {
            this._netComponent.Close().Coroutine();
        }

        if (this._netComponent.playerId > 0)
        {
            if (GUI.Button(new Rect(30, 190, 200, 60), "开始"))
            {
                this._netComponent.Start().Coroutine();
                this.lastReceiveTime = Environment.TickCount;
            }
        }
        
    }

    private void OnApplicationQuit()
    {
        this._netComponent.Close().Coroutine();
    }
}