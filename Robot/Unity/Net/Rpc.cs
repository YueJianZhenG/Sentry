using System;
using System.Collections.Generic;
using System.Net.WebSockets;
using System.Threading;
using ETModel;
using UnityEngine;

namespace rpc
{
    public static class TimeHelper
    {
        private static readonly long epoch = new DateTime(1970, 1, 1, 0, 0, 0, DateTimeKind.Utc).Ticks;
        /// <summary>
        /// 客户端时间
        /// </summary>
        /// <returns></returns>
        public static long ClientNow()
        {
            return (DateTime.UtcNow.Ticks - epoch) / 10000;
        }

        public static long ClientNowSeconds()
        {
            return (DateTime.UtcNow.Ticks - epoch) / 10000000;
        }
    }
    
    public class WsClient : IClient
    {
        private Uri mServerUri;
       
        private ClientWebSocket WsbClient;

        private CancellationTokenSource reveiveTokenSource;
        
        public WsClient(string address)
        {
            this.mServerUri = new Uri(address);
        }
        
        
        public async ETTask<bool> Connect()
        {
            try
            {
                this.WsbClient = new ClientWebSocket();
                CancellationTokenSource cts = new CancellationTokenSource();
                await this.WsbClient.ConnectAsync(this.mServerUri, cts.Token);
                Debug.Log($"connect {this.mServerUri} ok");
            }
            catch (Exception e)
            {
                Debug.LogError(e);
                return false;
            }
            return this.WsbClient.State == WebSocketState.Open;
        }

        public async ETTask<bool> Send(List<byte> buffer)
        {
            if (this.WsbClient.State != WebSocketState.Open)
            {
                return false;
            }
            try
            {
                CancellationTokenSource cts = new CancellationTokenSource();
                ArraySegment<byte> arraySegment = new ArraySegment<byte>(buffer.ToArray());
                await this.WsbClient.SendAsync(arraySegment, WebSocketMessageType.Binary, true, cts.Token);
            }
            catch (Exception e)
            {
                Debug.LogError(e);
                this.reveiveTokenSource?.Cancel();
                return false;
            }

            return true;
        }


        public async ETTask<int> ReceiveMessage(byte[] receiveBuffer)
        {
            ArraySegment<byte> arraySegment = new ArraySegment<byte>(receiveBuffer);
            if (this.WsbClient.State != WebSocketState.Open)
            {
                return 0;
            }
            WebSocketReceiveResult result = await this.WsbClient.ReceiveAsync(arraySegment, this.reveiveTokenSource.Token);
            if (result.MessageType == WebSocketMessageType.Close)
            {
                CancellationTokenSource closeSource = new CancellationTokenSource();
                await this.WsbClient.CloseAsync(WebSocketCloseStatus.NormalClosure,
                    "Closing", closeSource.Token);
                return 0;
            }
            return result.Count;
        }
        
        public async ETTask Close()
        {
            CancellationTokenSource closeSource = new CancellationTokenSource();
            await this.WsbClient.CloseAsync(WebSocketCloseStatus.NormalClosure,
                "Closing", closeSource.Token);
            Application.Quit();
        }
    }
}