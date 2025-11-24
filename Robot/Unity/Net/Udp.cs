using System;
using System.Collections.Generic;
using System.Net;
using System.Net.Sockets;
using ETModel;
using rpc;
using UnityEngine;

namespace udp
{
    public class Client : IClient
    {
        private UdpClient udpClient;
        private IPEndPoint _endPoint;

        public Client(string address)
        {
            this.udpClient = new UdpClient();
            string[] result = address.Split(':');
            IPAddress addr = IPAddress.Parse(result[0]);
            ushort port = ushort.Parse(result[1]);
            this._endPoint = new IPEndPoint(addr, port);
            this.udpClient = new UdpClient();
        }

        public ETTask Close()
        {
            this.udpClient.Close();
            return ETTask.CompletedTask;
        }

        public async ETTask<bool> Connect()
        {
            this.udpClient.Connect(this._endPoint);
            return true;
        }

        public async ETTask<bool> Send(List<byte> message)
        {
            try
            {
                await this.udpClient.SendAsync(message.ToArray(), message.Count);
            }
            catch (Exception e)
            {
                Debug.LogError(e);
                return false;
            }

            return true;
        }

        public async ETTask<int> ReceiveMessage(byte[] receiveBuffer)
        {
            UdpReceiveResult udpReceiveResult = await this.udpClient.ReceiveAsync();
            for (int index = 0; index < udpReceiveResult.Buffer.Length; index++)
            {
                receiveBuffer[index] = udpReceiveResult.Buffer[index];
            }

            return udpReceiveResult.Buffer.Length;
        }
    }
}