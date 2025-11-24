
using System;
using System.Collections.Generic;
using System.Reflection;
using System.Text;
using ETModel;
using Newtonsoft.Json;
using s2c;
using UnityEngine;

namespace rpc
{

    public interface IClient
    {
        public ETTask Close();

        public ETTask<bool> Connect();
        
        public ETTask<bool> Send(List<byte> message);
        
        public ETTask<int> ReceiveMessage(byte[] receiveBuffer);
    }

    public interface IUpdate
    {
        public void OnUpdate();
    }
    
    public class NetComponent
    {
        
        private IClient rpcClient;
        private IUpdate updateClient;
        private int _rpcId = 1;
        private int mRpcId => _rpcId++;
        public long playerId = 0;
        private Dictionary<string, object> BindMethods = new();
        
        private Dictionary<int, ETTaskCompletionSource<rpc.Message>> mTaskCompletionSources = new();

        public NetComponent(IClient client)
        {
            this.rpcClient = client;
            this.updateClient = client as IUpdate;
        }
        
        public bool Register<T>(T obj)
        {
            string name = typeof(T).Name;
            return this.BindMethods.TryAdd(name, obj);
        }

        public bool Register<T>(string name, T obj)
        {
            return this.BindMethods.TryAdd(name, obj);
        }
        
        private rpc.Message CreateMessage<T>(string func, T request)
        {
            rpc.Message rpcMessage = new Message();
            {
                rpcMessage.mHead.header.Add("$func", func);
                rpcMessage.mProtoHead.proto = rpc.Proto.none;
                rpcMessage.mProtoHead.type = rpc.Type.request;
            }
            if (request != null)
            {
                rpcMessage.mProtoHead.proto = rpc.Proto.json;
                string json = JsonConvert.SerializeObject(request);
                rpcMessage.mBody.AddRange(Encoding.UTF8.GetBytes(json));
            }
          
            return rpcMessage;
        }
        
        private rpc.Message CreateMessage(string func)
        {
            rpc.Message rpcMessage = new Message();
            {
                rpcMessage.mHead.header.Add("$func", func);
                rpcMessage.mProtoHead.proto = rpc.Proto.none;
                rpcMessage.mProtoHead.type = rpc.Type.request;
            }
            return rpcMessage;
        }

        public async ETTask<bool> Send(rpc.Message rpcMessage)
        {
            List<byte> buffer = new List<byte>();
            rpcMessage.OnSendMessage(buffer);
            return await this.rpcClient.Send(buffer);
        }

        public async ETTask<bool> Send<T>(string func, T message)
        {
            rpc.Message rpcMessage = this.CreateMessage(func, message);
            return await this.Send(rpcMessage);
        }
        
        public async ETTask<bool> Send(string func)
        {
            rpc.Message rpcMessage = this.CreateMessage(func);
            return await this.Send(rpcMessage);
        }

        public void OnUpdate()
        {
            if (this.updateClient != null)
            {
                this.updateClient.OnUpdate();
            }
        }

        private async ETTask<rpc.Message> CallMessage(rpc.Message rpcMessage)
        {
            try
            {
                rpcMessage.mProtoHead.rpcId = this.mRpcId;
                if (!await this.Send(rpcMessage))
                {
                    rpcMessage.mProtoHead.type = rpc.Type.response;
                    rpcMessage.mHead.header.Add("$code", "-2");
                    return rpcMessage;
                }

                ETTaskCompletionSource<rpc.Message> taskCompletionSource = new ETTaskCompletionSource<Message>();
                this.mTaskCompletionSources.Add(rpcMessage.mProtoHead.rpcId, taskCompletionSource);
                return await taskCompletionSource.Task;
            }
            catch (Exception e)
            {
                Debug.LogError(e);
                return null;
            }
        }

        public async ETTask<int> Call(string func)
        {
            rpc.Message rpcMessage = this.CreateMessage(func);
            rpc.Message rpcResponse = await this.CallMessage(rpcMessage);
            return rpcResponse.mHead.Code;
        }
        
        public async ETTask<(int, R)> Call<T, R>(string func, T request) where R : new()
        {
            rpc.Message rpcMessage = this.CreateMessage(func, request);
            rpc.Message rpcResponse = await this.CallMessage(rpcMessage);
            if (rpcResponse.mHead.Code != 0)
            {
                return (rpcResponse.mHead.Code, default);
            }

            string json = Encoding.UTF8.GetString(rpcResponse.mBody.ToArray());
            return (0,  JsonConvert.DeserializeObject<R>(json));
        }

        private async ETVoid StartReceive()
        {
            byte[] receiveBuffer = new byte[1024 * 8];
            while (true)
            {
                try
                {
                    int count = await rpcClient.ReceiveMessage(receiveBuffer);
                    if (count == 0)
                    {
                        continue;
                    }
                    int offset = 0;
                    rpc.Message rpcMessage = new Message();
                    {
                        rpcMessage.mProtoHead.type = receiveBuffer[offset++];
                        rpcMessage.mProtoHead.proto = receiveBuffer[offset++];
                        rpcMessage.mProtoHead.source = receiveBuffer[offset++];
                        rpcMessage.mProtoHead.rpcId = BitConverter.ToInt32(receiveBuffer, offset);
                        offset += 4;
                    }
                    while (true)
                    {
                        string line = rpc.tool.ReadLineFromBytes(receiveBuffer, ref offset);
                        if (string.IsNullOrEmpty(line))
                        {
                            break;
                        }

                        string[] resultList = line.Split(':');
                        if (resultList.Length != 2)
                        {
                            Debug.LogError("解析数据失败");
                            return;
                        }

                        rpcMessage.mHead.header.Add(resultList[0], resultList[1]);
                    }

                    for (int i = offset; i < count; i++)
                    {
                        rpcMessage.mBody.Add(receiveBuffer[i]);
                    }
                    this.OnMessage(rpcMessage);
                }
                catch (Exception e)
                {
                    Debug.LogError(e);
                    break;
                }
            }
            Debug.LogError("读取数据失败");
        }
        
        private void OnRequest(string func, List<byte> message)
        {
            string[] results = func.Split('.');
            if (!this.BindMethods.TryGetValue(results[0], out object service))
            {
                Debug.LogError($"not find class {results[0]}");
                return;
            }

            string method = results[1];
            MethodInfo methodInfo = service.GetType().GetMethod(method,
                BindingFlags.Public | BindingFlags.NonPublic | BindingFlags.Instance);
            if (methodInfo == null)
            {
                Debug.LogError($"not find method => {method}");
                return;
            }

            ParameterInfo[] parameterInfos = methodInfo.GetParameters();
            if (parameterInfos.Length == 0)
            {
                return;
            }

            System.Type type = parameterInfos[0].ParameterType;
            string json = Encoding.UTF8.GetString(message.ToArray());
            try
            {
                object parameter = JsonConvert.DeserializeObject(json, type);
                methodInfo.Invoke(service, new object[] { parameter });
            }
            catch (Exception e)
            {
                Debug.LogError(e);
            }
        }
        
        private void OnMessage(rpc.Message rpcMessage)
        {
            switch (rpcMessage.mProtoHead.type)
            {
                case rpc.Type.request:
                case rpc.Type.broadcast:
                {
                    string func = rpcMessage.mHead.Func;
                    Debug.Log("call function => " + func);
                    this.OnRequest(func, rpcMessage.mBody);
                    break;
                }
                case rpc.Type.response:
                {
                    int rpcId = rpcMessage.mProtoHead.rpcId;
                    if (!this.mTaskCompletionSources.TryGetValue(rpcId, out var taskCompletionSource))
                    {
                        Debug.LogError($"not find rpc id => {rpcId}");
                        return;
                    }

                    this.mTaskCompletionSources.Remove(rpcId);
                    taskCompletionSource.SetResult(rpcMessage);
                    break;
                }
                default:
                    break;
            }
        }


        public async ETVoid Login()
        {
            if (!await this.rpcClient.Connect())
            {
                Debug.LogError("连接失败");
                return;
            }
            this.StartReceive().Coroutine();
            string func = "SceneSystem.Login";
            (int code, LoginResponse response) = await this.Call<LoginRequest, LoginResponse>(func, new LoginRequest());
            if (code != 0)
            {
                Debug.LogError($"登录失败");
                return;
            }
            Debug.Log("登录成功");
            this.playerId = response.player_id;
        }

        public async ETVoid Start()
        {
            string func = "SceneSystem.Start";
            int code = await this.Call(func);
            Debug.LogWarning($"call {func} : {code}");
        }

        public async ETVoid Close()
        {
            int code = await this.Call("SceneSystem.Logout");
            Debug.LogWarning($"SceneSystem.Logout => {code}");
            await this.rpcClient.Close();
        }
    }
}

