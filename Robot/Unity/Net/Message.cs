using System;
using System.Collections.Generic;
using System.Text;

namespace rpc
{
    public class Type
    {
        public const byte request = 4;
        public const byte response = 5;
        public const byte broadcast = 7;
    }

    public class Net
    {
        public const byte tcp = 1;
        public const byte kcp = 6;
    }

    public class Proto
    {
        public const byte none = 0;
        public const byte json = 1;
        public const byte str = 2;
        public const byte pb = 3;
        public const byte lua = 4;
    }

    public class Source
    {
        public const byte none = 0;
        public const byte client = 1;
        public const byte server = 2;
    }

    public class ProtoHead
    {
        public uint len = 0;
        public byte type = rpc.Type.request;
        public byte proto = rpc.Proto.json;
        public byte source = rpc.Source.none;
        public int rpcId = 0;
    }

    public static class tool
    {
        public static string ReadLineFromBytes(byte[] buffer, ref int offset)
        {
            // 检查参数有效性
            if (buffer == null)
                throw new ArgumentNullException(nameof(buffer));
            if (offset < 0 || offset >= buffer.Length)
                return null; // 已经到达末尾

            int startIndex = offset;
            bool foundCR = false;

            // 查找换行符
            while (offset < buffer.Length)
            {
                byte currentByte = buffer[offset];

                // 检查是否是回车符
                if (currentByte == '\r')
                {
                    foundCR = true;
                    offset++;
                    continue;
                }

                // 检查是否是换行符
                if (currentByte == '\n')
                {
                    offset++;
                    // 计算行字节长度
                    int length = foundCR
                        ? offset - startIndex - 2
                        : // 减去\r\n
                        offset - startIndex - 1; // 只减去\n

                    // 将字节转换为字符串
                    return Encoding.UTF8.GetString(buffer, startIndex, length);
                }

                // 如果之前找到过回车但没有跟着换行符，那回车就是普通字符
                if (foundCR)
                {
                    offset--; // 回退到回车符位置
                    break;
                }

                offset++;
            }

            // 如果没有找到换行符，但还有剩余字节
            if (offset > startIndex)
            {
                int length = offset - startIndex;
                return Encoding.UTF8.GetString(buffer, startIndex, length);
            }

            return null; // 没有更多数据
        }
    }

    public class Head
    {
        public int OnSendMessage(StringBuilder stringBuilder)
        {
            foreach (var item in this.header)
            {
                stringBuilder.Append(item.Key).Append(':');
                stringBuilder.Append(item.Value).Append('\n');
            }

            stringBuilder.Append('\n');
            return 0;
        }

        public int OnRecvMessage(List<byte> buffer)
        {
            return 0;
        }

        public string Func => header["$func"];
        public int Code => int.Parse(header["$code"]);

        public Dictionary<string, string> header = new Dictionary<string, string>();
    }

    public class Message
    {
        public Head mHead = new Head();
        private byte mNet = rpc.Net.tcp;
        public List<byte> mBody = new List<byte>();
        public ProtoHead mProtoHead = new ProtoHead();

        public void OnSendMessage(List<byte> buffer)
        {
            buffer.Add(this.mProtoHead.type);
            buffer.Add(this.mProtoHead.proto);
            buffer.Add(this.mProtoHead.source);
            buffer.AddRange(BitConverter.GetBytes(this.mProtoHead.rpcId));
            StringBuilder stringBuilder = new StringBuilder();
            {
                this.mHead.OnSendMessage(stringBuilder);
                buffer.AddRange(Encoding.UTF8.GetBytes(stringBuilder.ToString()));
            }
            buffer.AddRange(this.mBody);
        }
    }
}