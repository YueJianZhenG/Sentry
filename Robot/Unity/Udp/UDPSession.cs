using System;
using System.Collections.Generic;
using System.Net;
using System.Net.Sockets;
using System.Text;
using System.Threading.Tasks;
using ETModel;
using KcpProject;
using rpc;
using UnityEngine;
using Random = System.Random;

namespace kcp
{
    class Client : IClient, IUpdate
    {
        private KcpSharp.Kcp mKCP = null;
        private UdpClient udpClient;
        private IPEndPoint _endPoint;
        private ByteBuffer mRecvBuffer = ByteBuffer.Allocate(1024 * 32);
        private UInt32 mNextUpdateTime = 0;
        private bool isDisposed = false;

        public string Address { get; private set; }
        private int receiveBufferSize = 1024 * 32;

        private DateTime startDt = DateTime.Now;

        public Client(string address)
        {
            if (string.IsNullOrEmpty(address))
                throw new ArgumentNullException(nameof(address));

            this.Address = address;
            string[] result = address.Split(':');
            if (result.Length != 2)
                throw new ArgumentException("地址格式不正确，应为 ip:port", nameof(address));

            IPAddress addr = IPAddress.Parse(result[0]);
            if (!ushort.TryParse(result[1], out ushort port))
                throw new ArgumentException("端口号无效", nameof(address));

            this.udpClient = new UdpClient();
            this._endPoint = new IPEndPoint(addr, port);
        }

        public async ETTask<bool> Connect()
        {
            try
            {
                // 连接到服务器
                udpClient.Connect(_endPoint);
                this.mKCP = new KcpSharp.Kcp(1, this.RawSend, null);
                this.mKCP.WndSize(128, 128);
                this.mKCP.Nodelay(1, 20, 2, 1);
                mRecvBuffer.Clear();
                mNextUpdateTime = 0;

                Log("KCP客户端已连接");
                return true;
            }
            catch (Exception ex)
            {
                LogError($"连接失败: {ex.Message}");
                return false;
            }
        }

        public async ETTask<bool> Send(List<byte> message)
        {
            if (message == null || message.Count == 0)
                return true; // 空消息无需发送

            try
            {
                this.mKCP.Send(message.ToArray());
                this.mKCP.Flush();
                return true;
            }
            catch (Exception ex)
            {
                LogError($"发送消息失败: {ex.Message}");
                return false;
            }
        }

        public async ETTask<int> ReceiveMessage(byte[] receiveBuffer)
        {
            UdpReceiveResult result = await udpClient.ReceiveAsync();
            if (result.Buffer == null || result.Buffer.Length == 0)
                return 0;

            // 处理接收到的UDP数据
            byte[] recvBuffer = ProcessReceivedData(result.Buffer);
            if (recvBuffer == null)
            {
                return 0;
            }
            Debug.LogWarning($"kcp count => {recvBuffer.Length}");

            for (int i = 0; i < recvBuffer.Length; i++)
            {
                receiveBuffer[i] = recvBuffer[i];
            }

            return recvBuffer.Length;
        }

        private int RawSend(ReadOnlySpan<byte> message, object user)
        {
            try
            {
                udpClient.Send(message.ToArray(), message.Length);
            }
            catch (Exception ex)
            {
                LogError($"原始发送失败: {ex.Message}");
            }

            return 0;
        }

        private byte[] ProcessReceivedData(byte[] data)
        {
            this.mKCP.Input(data);
            Span<byte> buffer = stackalloc byte[1024];
            int count = this.mKCP.Recv(buffer);
            if (count <= 0)
            {
                return null;
            }
            return buffer.Slice(0, count).ToArray();
        }

        public void OnUpdate()
        {
            // 定期更新KCP状态
            if (this.mKCP == null)
            {
                return;
            }

            mKCP.Update((uint)TimeHelper.ClientNow());
        }

        private void Log(string str)
        {
            DateTime now = DateTime.Now;
            int t = (int)(now - startDt).TotalMilliseconds;
            Debug.Log($"[{t.ToString(),10}] {str}");
        }

        public void LogError(string str)
        {
            DateTime now = DateTime.Now;
            int t = (int)(now - startDt).TotalMilliseconds;
            Debug.LogError($"[{t.ToString(),10}] {str}");
        }

        public async ETTask Close()
        {
            try
            {
                udpClient?.Close();
            }
            catch (Exception ex)
            {
                LogError($"关闭连接错误: {ex.Message}");
            }

            Log("连接已关闭");
            await ETTask.CompletedTask;
        }

        public void Dispose()
        {
            if (isDisposed)
                return;

            isDisposed = true;
            this.Close().Coroutine();
            udpClient?.Dispose();
            mRecvBuffer?.Dispose();
        }
    }
}