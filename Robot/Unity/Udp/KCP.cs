// Kcp.cs
// Full C# port of ikcp.c / ikcp.h (KCP) by skywind3000
// Translated and adapted to C# types and collections.
// Usage:
//   var kcp = new Kcp(conv, OutputCallback, user);
//   kcp.Send(...);
//   kcp.Input(...);
//   kcp.Update(currentMs);
//   kcp.Flush(); // usually called inside Update
//
// Output callback signature:
//   int Output(ReadOnlySpan<byte> buffer, object user)

using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Runtime.CompilerServices;
using System.Text;

namespace KcpSharp
{
    public delegate int KcpOutput(ReadOnlySpan<byte> buffer, object user);

    public class Kcp
    {
        #region constants

        public const uint IKCP_RTO_NDL = 30;       // no delay min rto
        public const uint IKCP_RTO_MIN = 100;     // normal min rto
        public const uint IKCP_RTO_DEF = 200;
        public const uint IKCP_RTO_MAX = 60000;
        public const byte IKCP_CMD_PUSH = 81;     // cmd: push data
        public const byte IKCP_CMD_ACK = 82;      // cmd: ack
        public const byte IKCP_CMD_WASK = 83;     // cmd: window probe (ask)
        public const byte IKCP_CMD_WINS = 84;     // cmd: window size (tell)
        public const byte IKCP_ASK_SEND = 1;      // need to send IKCP_CMD_WASK
        public const byte IKCP_ASK_TELL = 2;      // need to send IKCP_CMD_WINS
        public const uint IKCP_WND_SND = 32;
        public const uint IKCP_WND_RCV = 128;     // must >= max fragment size
        public const uint IKCP_MTU_DEF = 1400;
        public const uint IKCP_ACK_FAST = 3;
        public const uint IKCP_INTERVAL = 100;
        public const uint IKCP_OVERHEAD = 24;
        public const uint IKCP_DEADLINK = 20;
        public const uint IKCP_THRESH_INIT = 2;
        public const uint IKCP_THRESH_MIN = 2;
        public const uint IKCP_PROBE_INIT = 7000;     // 7 secs to probe window size
        public const uint IKCP_PROBE_LIMIT = 120000;  // up to 120 secs to probe window
        public const uint IKCP_FASTACK_LIMIT = 5;     // max times to trigger fastack

        // log flags kept for compatibility (no logger implemented by default)
        public const int IKCP_LOG_OUTPUT = 1;
        public const int IKCP_LOG_INPUT = 2;
        public const int IKCP_LOG_SEND = 4;
        public const int IKCP_LOG_RECV = 8;
        public const int IKCP_LOG_IN_DATA = 16;
        public const int IKCP_LOG_IN_ACK = 32;
        public const int IKCP_LOG_IN_PROBE = 64;
        public const int IKCP_LOG_IN_WINS = 128;
        public const int IKCP_LOG_OUT_DATA = 256;
        public const int IKCP_LOG_OUT_ACK = 512;
        public const int IKCP_LOG_OUT_PROBE = 1024;
        public const int IKCP_LOG_OUT_WINS = 2048;

        #endregion

        #region segment

        private class Segment
        {
            public uint conv;
            public uint cmd;
            public uint frg;
            public uint wnd;
            public uint ts;
            public uint sn;
            public uint una;
            public uint len;
            public uint resendts;
            public uint rto;
            public uint fastack;
            public uint xmit;
            public byte[] data;

            public Segment(int size)
            {
                if (size < 0) size = 0;
                data = new byte[size];
                len = (uint)size;
            }
        }

        #endregion

        #region fields (ikcpcb)

        public uint conv;
        public uint mtu;
        public uint mss;
        public uint state;

        public uint snd_una;
        public uint snd_nxt;
        public uint rcv_nxt;

        public uint ts_recent;
        public uint ts_lastack;
        public uint ssthresh;

        public int rx_rttval;
        public int rx_srtt;
        public int rx_rto;
        public int rx_minrto;

        public uint snd_wnd;
        public uint rcv_wnd;
        public uint rmt_wnd;
        public uint cwnd;
        public uint probe;

        public uint current;
        public uint interval;
        public uint ts_flush;
        public uint xmit;

        public uint nrcv_buf;
        public uint nsnd_buf;
        public uint nrcv_que;
        public uint nsnd_que;

        public uint nodelay;
        public uint updated;

        public uint ts_probe;
        public uint probe_wait;

        public uint dead_link;
        public uint incr;

        // queues
        private LinkedList<Segment> snd_queue = new LinkedList<Segment>();
        private LinkedList<Segment> rcv_queue = new LinkedList<Segment>();
        private LinkedList<Segment> snd_buf = new LinkedList<Segment>();
        private LinkedList<Segment> rcv_buf = new LinkedList<Segment>();

        // ack list: store pairs (sn, ts) flattened
        private List<uint> acklist = null;
        private uint ackcount = 0;
        private uint ackblock = 0;

        public object user;
        private byte[] buffer;

        public int fastresend;
        public int fastlimit;
        public int nocwnd;
        public int stream;
        public int logmask;

        public KcpOutput output;
        public Action<string, Kcp, object> writelog;

        #endregion

        #region ctor / release

        public Kcp(uint conv, KcpOutput output, object user)
        {
            this.conv = conv;
            this.user = user;
            this.snd_una = 0;
            this.snd_nxt = 0;
            this.rcv_nxt = 0;
            this.ts_recent = 0;
            this.ts_lastack = 0;
            this.ts_probe = 0;
            this.probe_wait = 0;
            this.snd_wnd = IKCP_WND_SND;
            this.rcv_wnd = IKCP_WND_RCV;
            this.rmt_wnd = IKCP_WND_RCV;
            this.cwnd = 0;
            this.incr = 0;
            this.probe = 0;
            this.mtu = IKCP_MTU_DEF;
            this.mss = this.mtu - IKCP_OVERHEAD;
            this.stream = 0;

            this.buffer = new byte[(this.mtu + IKCP_OVERHEAD) * 3];

            this.nrcv_buf = 0;
            this.nsnd_buf = 0;
            this.nrcv_que = 0;
            this.nsnd_que = 0;
            this.state = 0;
            this.acklist = null;
            this.ackblock = 0;
            this.ackcount = 0;
            this.rx_srtt = 0;
            this.rx_rttval = 0;
            this.rx_rto = (int)IKCP_RTO_DEF;
            this.rx_minrto = (int)IKCP_RTO_MIN;
            this.current = 0;
            this.interval = IKCP_INTERVAL;
            this.ts_flush = IKCP_INTERVAL;
            this.nodelay = 0;
            this.updated = 0;
            this.logmask = 0;
            this.ssthresh = IKCP_THRESH_INIT;
            this.fastresend = 0;
            this.fastlimit = (int)IKCP_FASTACK_LIMIT;
            this.nocwnd = 0;
            this.xmit = 0;
            this.dead_link = IKCP_DEADLINK;
            this.output = output;
            this.writelog = null;
        }

        public void Release()
        {
            // clear snd_buf
            foreach (var seg in snd_buf)
            {
                // nothing special
            }
            snd_buf.Clear();

            foreach (var seg in rcv_buf) { }
            rcv_buf.Clear();

            snd_queue.Clear();
            rcv_queue.Clear();

            buffer = null;
            acklist = null;
            ackcount = 0;
        }

        #endregion

        #region helpers: encode/decode + small utils

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        private static uint _imin(uint a, uint b) => a <= b ? a : b;

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        private static uint _imax(uint a, uint b) => a >= b ? a : b;

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        private static uint _ibound(uint lower, uint middle, uint upper)
        {
            return _imin(_imax(lower, middle), upper);
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        private static int _itimediff(uint later, uint earlier)
        {
            return (int)((uint)(later - earlier));
        }

        private static void encode8u(byte[] p, ref int offset, byte c)
        {
            p[offset++] = c;
        }

        private static void encode16u(byte[] p, ref int offset, ushort w)
        {
            p[offset++] = (byte)(w & 0xff);
            p[offset++] = (byte)((w >> 8) & 0xff);
        }

        private static void encode32u(byte[] p, ref int offset, uint l)
        {
            p[offset++] = (byte)(l & 0xff);
            p[offset++] = (byte)((l >> 8) & 0xff);
            p[offset++] = (byte)((l >> 16) & 0xff);
            p[offset++] = (byte)((l >> 24) & 0xff);
        }

        private static uint decode32u(ReadOnlySpan<byte> p, ref int offset)
        {
            uint r = (uint)(p[offset] | (p[offset + 1] << 8) | (p[offset + 2] << 16) | (p[offset + 3] << 24));
            offset += 4;
            return r;
        }

        private static ushort decode16u(ReadOnlySpan<byte> p, ref int offset)
        {
            ushort r = (ushort)(p[offset] | (p[offset + 1] << 8));
            offset += 2;
            return r;
        }

        private static byte decode8u(ReadOnlySpan<byte> p, ref int offset)
        {
            byte r = p[offset];
            offset++;
            return r;
        }

        #endregion

        #region allocator hooks (not used but present)

        // In original C implementation there is ikcp_allocator to override malloc/free.
        // In managed C# we don't need it. We keep placeholder methods if user wants to plug pooling.

        #endregion

        #region segment management

        private Segment segmentNew(int size)
        {
            return new Segment(size);
        }

        private void segmentDelete(Segment seg)
        {
            // managed language: nothing to explicitly free
        }

        #endregion

        #region logging (no-op by default)

        private void ikcp_log(int mask, string fmt, params object[] args)
        {
            if ((mask & this.logmask) == 0 || this.writelog == null) return;
            string s = string.Format(fmt, args);
            this.writelog(s, this, this.user);
        }

        private bool ikcp_canlog(int mask)
        {
            if ((mask & this.logmask) == 0 || this.writelog == null) return false;
            return true;
        }

        #endregion

        #region output helper

        private int ikcp_output(ReadOnlySpan<byte> data, int size)
        {
            if (ikcp_canlog(IKCP_LOG_OUTPUT))
            {
                ikcp_log(IKCP_LOG_OUTPUT, "[RO] {0} bytes", size);
            }
            if (size == 0) return 0;
            return this.output(data.Slice(0, size), this.user);
        }

        #endregion

        #region public API (set output, send, recv, etc.)

        public void SetOutput(KcpOutput output)
        {
            this.output = output; // note: stored via ctor; allow change
        }

        public int Recv(Span<byte> buffer)
        {
            bool ispeek = false; // original supports negative len for peek; we keep simple variant
            int peeksize;
            int recover = 0;
            Segment seg;

            if (rcv_queue.Count == 0) return -1; // EAGAIN

            peeksize = PeekSize();
            if (peeksize < 0) return -2;
            if (peeksize > buffer.Length) return -3;

            if (nrcv_que >= rcv_wnd) recover = 1;

            // merge fragments
            int len = 0;
            LinkedListNode<Segment> p = rcv_queue.First;
            while (p != null)
            {
                seg = p.Value;
                var next = p.Next;
                if (buffer != null)
                {
                    seg.data.AsSpan(0, (int)seg.len).CopyTo(buffer.Slice(len));
                }
                len += (int)seg.len;
                byte fragment = (byte)seg.frg;

                if (!ispeek)
                {
                    rcv_queue.Remove(p);
                    segmentDelete(seg);
                    nrcv_que--;
                }
                if (fragment == 0) break;
                p = next;
            }

            Debug.Assert(len == peeksize);

            // move available data from rcv_buf -> rcv_queue
            while (rcv_buf.Count > 0)
            {
                var first = rcv_buf.First.Value;
                if (first.sn == rcv_nxt && nrcv_que < rcv_wnd)
                {
                    rcv_buf.RemoveFirst();
                    nrcv_buf--;
                    rcv_queue.AddLast(first);
                    nrcv_que++;
                    rcv_nxt++;
                }
                else break;
            }

            // fast recover
            if (nrcv_que < rcv_wnd && recover != 0)
            {
                probe |= IKCP_ASK_TELL;
            }

            return len;
        }

        public int PeekSize()
        {
            if (rcv_queue.Count == 0) return -1;
            var seg = rcv_queue.First.Value;
            if (seg.frg == 0) return (int)seg.len;
            if (nrcv_que < seg.frg + 1) return -1;
            int length = 0;
            foreach (var s in rcv_queue)
            {
                length += (int)s.len;
                if (s.frg == 0) break;
            }
            return length;
        }

        public int Send(ReadOnlySpan<byte> data)
        {
            Segment seg;
            int count;
            int i;
            int sent = 0;

            if (mss <= 0) return -1;
            if (data.Length < 0) return -1;

            // streaming mode append: omitted here if stream != 0 behavior (we implement basic)
            if (stream != 0)
            {
                if (snd_queue.Count > 0)
                {
                    var old = snd_queue.Last.Value;
                    if (old.len < mss)
                    {
                        int capacity = (int)mss - (int)old.len;
                        int extend = data.Length < capacity ? data.Length : capacity;
                        seg = segmentNew((int)old.len + extend);
                        if (seg == null) return -2;
                        snd_queue.AddLast(seg);
                        Buffer.BlockCopy(old.data, 0, seg.data, 0, (int)old.len);
                        if (data.Length > 0)
                        {
                            data.Slice(0, extend).CopyTo(seg.data.AsSpan((int)old.len));
                        }
                        seg.len = (uint)((int)old.len + extend);
                        seg.frg = 0;
                        // delete old
                        snd_queue.RemoveLast();
                        segmentDelete(old);
                        sent = extend;
                        // move data pointer
                        if (extend >= data.Length)
                        {
                            return sent;
                        }
                        data = data.Slice(extend);
                    }
                }
                if (data.Length <= 0) return sent;
            }

            if (data.Length <= mss) count = 1;
            else count = (data.Length + (int)mss - 1) / (int)mss;

            if (count >= (int)IKCP_WND_RCV)
            {
                if (stream != 0 && sent > 0) return sent;
                return -2;
            }

            if (count == 0) count = 1;

            // fragment
            for (i = 0; i < count; i++)
            {
                int size = data.Length > (int)mss ? (int)mss : data.Length - i * (int)mss;
                if (size < 0) size = 0;
                seg = segmentNew(size);
                if (seg == null) return -2;
                if (data.Length > 0)
                {
                    var start = i * (int)mss;
                    int take = Math.Min(size, Math.Max(0, data.Length - start));
                    if (take > 0)
                        data.Slice(start, take).CopyTo(seg.data.AsSpan(0, take));
                }
                seg.len = (uint)size;
                seg.frg = (uint)(stream == 0 ? (count - i - 1) : 0);
                snd_queue.AddLast(seg);
                nsnd_que++;
                sent += size;
            }

            return sent;
        }

        #endregion

        #region ack / parse helpers

        private void updateAck(int rtt)
        {
            int rto = 0;
            if (rx_srtt == 0)
            {
                rx_srtt = rtt;
                rx_rttval = rtt / 2;
            }
            else
            {
                int delta = rtt - rx_srtt;
                if (delta < 0) delta = -delta;
                rx_rttval = (3 * rx_rttval + delta) / 4;
                rx_srtt = (7 * rx_srtt + rtt) / 8;
                if (rx_srtt < 1) rx_srtt = 1;
            }
            rto = rx_srtt + (int)_imax(interval, (uint)(4 * rx_rttval));
            rx_rto = (int)_ibound((uint)rx_minrto, (uint)rto, IKCP_RTO_MAX);
        }

        private void shrinkBuf()
        {
            if (snd_buf.Count > 0)
            {
                var first = snd_buf.First.Value;
                snd_una = first.sn;
            }
            else
            {
                snd_una = snd_nxt;
            }
        }

        private void parseAck(uint sn)
        {
            if (_itimediff(sn, snd_una) < 0 || _itimediff(sn, snd_nxt) >= 0) return;

            var node = snd_buf.First;
            while (node != null)
            {
                var next = node.Next;
                var seg = node.Value;
                if (sn == seg.sn)
                {
                    snd_buf.Remove(node);
                    segmentDelete(seg);
                    nsnd_buf--;
                    break;
                }
                if (_itimediff(sn, seg.sn) < 0) break;
                node = next;
            }
        }

        private void parseUna(uint una)
        {
            var node = snd_buf.First;
            while (node != null)
            {
                var next = node.Next;
                var seg = node.Value;
                if (_itimediff(una, seg.sn) > 0)
                {
                    snd_buf.Remove(node);
                    segmentDelete(seg);
                    nsnd_buf--;
                }
                else break;
                node = next;
            }
        }

        private void parseFastack(uint sn, uint ts)
        {
            if (_itimediff(sn, snd_una) < 0 || _itimediff(sn, snd_nxt) >= 0) return;

            var node = snd_buf.First;
            while (node != null)
            {
                var next = node.Next;
                var seg = node.Value;
                if (_itimediff(sn, seg.sn) < 0) break;
                else if (sn != seg.sn)
                {
#if false
                    seg.fastack++;
#else
                    if (_itimediff(ts, seg.ts) >= 0) seg.fastack++;
#endif
                }
                node = next;
            }
        }

        private void ackPush(uint sn, uint ts)
        {
            uint newsize = ackcount + 1;
            if (newsize > ackblock)
            {
                uint newblock = 8;
                while (newblock < newsize) newblock <<= 1;
                var newlist = new List<uint>( (int)newblock * 2 );
                if (acklist != null)
                {
                    for (int x = 0; x < ackcount; x++)
                    {
                        newlist.Add(acklist[x * 2 + 0]);
                        newlist.Add(acklist[x * 2 + 1]);
                    }
                }
                acklist = newlist;
                ackblock = newblock;
            }
            if (acklist == null) acklist = new List<uint>();
            if ((acklist.Count / 2) < ackcount) { /* should not happen */ }
            acklist.Add(sn);
            acklist.Add(ts);
            ackcount++;
        }

        private void ackGet(int p, out uint sn, out uint ts)
        {
            sn = 0; ts = 0;
            if (acklist == null) return;
            int idx = p * 2;
            if (idx + 1 < acklist.Count)
            {
                sn = acklist[idx];
                ts = acklist[idx + 1];
            }
        }

        private void parseData(Segment newseg)
        {
            uint sn = newseg.sn;
            int repeat = 0;

            if (_itimediff(sn, rcv_nxt + rcv_wnd) >= 0 || _itimediff(sn, rcv_nxt) < 0)
            {
                segmentDelete(newseg);
                return;
            }

            // insert ordered into rcv_buf (from tail backward)
            var node = rcv_buf.Last;
            while (node != null)
            {
                var seg = node.Value;
                if (seg.sn == sn)
                {
                    repeat = 1;
                    break;
                }
                if (_itimediff(sn, seg.sn) > 0)
                {
                    break;
                }
                node = node.Previous;
            }

            if (repeat == 0)
            {
                if (node == null)
                {
                    rcv_buf.AddFirst(newseg);
                }
                else
                {
                    // insert after node
                    rcv_buf.AddAfter(node, newseg);
                }
                nrcv_buf++;
            }
            else
            {
                segmentDelete(newseg);
            }

            // move available data from rcv_buf -> rcv_queue
            while (rcv_buf.Count > 0)
            {
                var first = rcv_buf.First.Value;
                if (first.sn == rcv_nxt && nrcv_que < rcv_wnd)
                {
                    rcv_buf.RemoveFirst();
                    nrcv_buf--;
                    rcv_queue.AddLast(first);
                    nrcv_que++;
                    rcv_nxt++;
                }
                else break;
            }
        }

        #endregion

        #region input / flush / update / check

        public int Input(ReadOnlySpan<byte> data)
        {
            // returns 0 on success, negative on error
            uint prev_una = snd_una;
            uint maxack = 0, latest_ts = 0;
            int flag = 0;

            if (ikcp_canlog(IKCP_LOG_INPUT)) ikcp_log(IKCP_LOG_INPUT, "[RI] {0} bytes", data.Length);

            if (data == null || data.Length < IKCP_OVERHEAD) return -1;

            int offset = 0;
            while (true)
            {
                if (data.Length - offset < IKCP_OVERHEAD) break;

                uint conv_ = decode32u(data, ref offset);
                if (conv_ != conv) return -1;
                byte cmd = decode8u(data, ref offset);
                byte frg = decode8u(data, ref offset);
                ushort wnd = decode16u(data, ref offset);
                uint ts = decode32u(data, ref offset);
                uint sn = decode32u(data, ref offset);
                uint una = decode32u(data, ref offset);
                uint len = decode32u(data, ref offset);

                if (data.Length - offset < (int)len) return -2;

                if (cmd != IKCP_CMD_PUSH && cmd != IKCP_CMD_ACK && cmd != IKCP_CMD_WASK && cmd != IKCP_CMD_WINS)
                    return -3;

                rmt_wnd = wnd;
                parseUna(una);
                shrinkBuf();

                if (cmd == IKCP_CMD_ACK)
                {
                    if (_itimediff(current, ts) >= 0)
                    {
                        updateAck(_itimediff(current, ts));
                    }
                    parseAck(sn);
                    shrinkBuf();
                    if (flag == 0)
                    {
                        flag = 1;
                        maxack = sn;
                        latest_ts = ts;
                    }
                    else
                    {
                        if (_itimediff(sn, maxack) > 0)
                        {
#if false
                            maxack = sn;
                            latest_ts = ts;
#else
                            if (_itimediff(ts, latest_ts) > 0)
                            {
                                maxack = sn;
                                latest_ts = ts;
                            }
#endif
                        }
                    }
                    if (ikcp_canlog(IKCP_LOG_IN_ACK))
                    {
                        ikcp_log(IKCP_LOG_IN_ACK, "input ack: sn={0} rtt={1} rto={2}", sn, _itimediff(current, ts), rx_rto);
                    }
                }
                else if (cmd == IKCP_CMD_PUSH)
                {
                    if (ikcp_canlog(IKCP_LOG_IN_DATA)) ikcp_log(IKCP_LOG_IN_DATA, "input psh: sn={0} ts={1}", sn, ts);
                    if (_itimediff(sn, rcv_nxt + rcv_wnd) < 0)
                    {
                        ackPush(sn, ts);
                        if (_itimediff(sn, rcv_nxt) >= 0)
                        {
                            var seg = segmentNew((int)len);
                            seg.conv = conv_;
                            seg.cmd = cmd;
                            seg.frg = frg;
                            seg.wnd = wnd;
                            seg.ts = ts;
                            seg.sn = sn;
                            seg.una = una;
                            seg.len = len;
                            if (len > 0)
                            {
                                if (len > seg.data.Length) seg.data = new byte[len];
                                data.Slice(offset, (int)len).CopyTo(seg.data);
                            }
                            parseData(seg);
                        }
                    }
                }
                else if (cmd == IKCP_CMD_WASK)
                {
                    // remote ask me to send IKCP_CMD_WINS
                    probe |= IKCP_ASK_TELL;
                    if (ikcp_canlog(IKCP_LOG_IN_PROBE)) ikcp_log(IKCP_LOG_IN_PROBE, "input probe");
                }
                else if (cmd == IKCP_CMD_WINS)
                {
                    // do nothing, remote tell its wnd
                    if (ikcp_canlog(IKCP_LOG_IN_WINS)) ikcp_log(IKCP_LOG_IN_WINS, "input wins: {0}", wnd);
                }
                else
                {
                    return -3;
                }

                offset += (int)len;
            }

            if (flag != 0)
            {
                parseFastack(maxack, latest_ts);
            }

            if (_itimediff(snd_una, prev_una) > 0)
            {
                if (cwnd < rmt_wnd)
                {
                    uint mss_ = mss;
                    if (cwnd < ssthresh)
                    {
                        cwnd++;
                        incr += mss_;
                    }
                    else
                    {
                        if (incr < mss_) incr = mss_;
                        incr += (mss_ * mss_) / incr + (mss_ / 16);
                        if ((cwnd + 1) * mss_ <= incr)
                        {
                            cwnd = (incr + mss_ - 1) / (mss_ > 0 ? mss_ : 1);
                        }
                    }
                    if (cwnd > rmt_wnd)
                    {
                        cwnd = rmt_wnd;
                        incr = rmt_wnd * mss_;
                    }
                }
            }

            return 0;
        }

        public void Flush()
        {
            // alias current local
            uint current = this.current;
            byte[] buffer = this.buffer;
            int ptr = 0;
            int count, size, i;
            uint resent, cwnd_;
            uint rtomin;
            int change = 0;
            int lost = 0;

            if (this.updated == 0) return;

            // ack segment template
            Segment segtmp = new Segment(0);
            segtmp.conv = conv;
            segtmp.cmd = IKCP_CMD_ACK;
            segtmp.frg = 0;
            segtmp.wnd = (uint)wndUnused();
            segtmp.una = rcv_nxt;
            segtmp.len = 0;
            segtmp.sn = 0;
            segtmp.ts = 0;

            // flush acknowledges
            count = (int)ackcount;
            for (i = 0; i < count; i++)
            {
                size = ptr;
                if (size + (int)IKCP_OVERHEAD > buffer.Length)
                {
                    ikcp_output(buffer, ptr);
                    ptr = 0;
                }
                ackGet(i, out uint sn, out uint ts);
                // encode segtmp with sn/ts
                encode32u(buffer, ref ptr, segtmp.conv);
                encode8u(buffer, ref ptr, (byte)segtmp.cmd);
                encode8u(buffer, ref ptr, (byte)segtmp.frg);
                encode16u(buffer, ref ptr, (ushort)segtmp.wnd);
                encode32u(buffer, ref ptr, ts); // seg.ts = ts
                encode32u(buffer, ref ptr, sn); // seg.sn = sn
                encode32u(buffer, ref ptr, segtmp.una);
                encode32u(buffer, ref ptr, 0);
            }
            ackcount = 0;
            acklist?.Clear();

            // probe window size (if remote window size equals zero)
            if (rmt_wnd == 0)
            {
                if (probe_wait == 0)
                {
                    probe_wait = IKCP_PROBE_INIT;
                    ts_probe = current + probe_wait;
                }
                else
                {
                    if (_itimediff(current, ts_probe) >= 0)
                    {
                        if (probe_wait < IKCP_PROBE_INIT) probe_wait = IKCP_PROBE_INIT;
                        probe_wait += probe_wait / 2;
                        if (probe_wait > IKCP_PROBE_LIMIT) probe_wait = IKCP_PROBE_LIMIT;
                        ts_probe = current + probe_wait;
                        probe |= IKCP_ASK_SEND;
                    }
                }
            }
            else
            {
                ts_probe = 0;
                probe_wait = 0;
            }

            // flush window probing commands
            if ((probe & IKCP_ASK_SEND) != 0)
            {
                segtmp.cmd = IKCP_CMD_WASK;
                size = ptr;
                if (size + (int)IKCP_OVERHEAD > buffer.Length)
                {
                    ikcp_output(buffer, ptr);
                    ptr = 0;
                }
                // encode segtmp
                encode32u(buffer, ref ptr, segtmp.conv);
                encode8u(buffer, ref ptr, (byte)segtmp.cmd);
                encode8u(buffer, ref ptr, (byte)segtmp.frg);
                encode16u(buffer, ref ptr, (ushort)segtmp.wnd);
                encode32u(buffer, ref ptr, 0);
                encode32u(buffer, ref ptr, 0);
                encode32u(buffer, ref ptr, segtmp.una);
                encode32u(buffer, ref ptr, 0);
            }

            if ((probe & IKCP_ASK_TELL) != 0)
            {
                segtmp.cmd = IKCP_CMD_WINS;
                size = ptr;
                if (size + (int)IKCP_OVERHEAD > buffer.Length)
                {
                    ikcp_output(buffer, ptr);
                    ptr = 0;
                }
                encode32u(buffer, ref ptr, segtmp.conv);
                encode8u(buffer, ref ptr, (byte)segtmp.cmd);
                encode8u(buffer, ref ptr, (byte)segtmp.frg);
                encode16u(buffer, ref ptr, (ushort)segtmp.wnd);
                encode32u(buffer, ref ptr, 0);
                encode32u(buffer, ref ptr, 0);
                encode32u(buffer, ref ptr, segtmp.una);
                encode32u(buffer, ref ptr, 0);
            }

            probe = 0;

            // calculate window size
            cwnd_ = _imin(snd_wnd, rmt_wnd);
            if (nocwnd == 0) cwnd_ = _imin(cwnd, cwnd_);

            // move data from snd_queue to snd_buf
            while (_itimediff(snd_nxt, snd_una + cwnd_) < 0)
            {
                if (snd_queue.Count == 0) break;
                var newseg = snd_queue.First.Value;
                snd_queue.RemoveFirst();
                snd_buf.AddLast(newseg);
                nsnd_que--;
                nsnd_buf++;

                newseg.conv = conv;
                newseg.cmd = IKCP_CMD_PUSH;
                newseg.wnd = segtmp.wnd;
                newseg.ts = current;
                newseg.sn = snd_nxt++;
                newseg.una = rcv_nxt;
                newseg.resendts = current;
                newseg.rto = (uint)rx_rto;
                newseg.fastack = 0;
                newseg.xmit = 0;
            }

            // calculate resent
            resent = (fastresend > 0) ? (uint)fastresend : 0xffffffff;
            rtomin = (nodelay == 0) ? (uint)(rx_rto >> 3) : 0;

            // flush data segments
            var p = snd_buf.First;
            while (p != null)
            {
                var segment = p.Value;
                var next = p.Next;
                int needsend = 0;
                if (segment.xmit == 0)
                {
                    needsend = 1;
                    segment.xmit++;
                    segment.rto = (uint)rx_rto;
                    segment.resendts = current + segment.rto + rtomin;
                }
                else if (_itimediff(current, segment.resendts) >= 0)
                {
                    needsend = 1;
                    segment.xmit++;
                    xmit++;
                    if (nodelay == 0)
                    {
                        segment.rto += _imax(segment.rto, (uint)rx_rto);
                    }
                    else
                    {
                        int step = (nodelay < 2) ? (int)segment.rto : rx_rto;
                        segment.rto += (uint)(step / 2);
                    }
                    segment.resendts = current + segment.rto;
                    lost = 1;
                }
                else if (segment.fastack >= resent)
                {
                    if ((int)segment.xmit <= fastlimit || fastlimit <= 0)
                    {
                        needsend = 1;
                        segment.xmit++;
                        segment.fastack = 0;
                        segment.resendts = current + segment.rto;
                        change++;
                    }
                }

                if (needsend != 0)
                {
                    segment.ts = current;
                    segment.wnd = segtmp.wnd;
                    segment.una = rcv_nxt;

                    size = ptr;
                    int need = (int)IKCP_OVERHEAD + (int)segment.len;
                    if (size + need > buffer.Length)
                    {
                        ikcp_output(buffer, ptr);
                        ptr = 0;
                    }

                    // encode segment header
                    encode32u(buffer, ref ptr, segment.conv);
                    encode8u(buffer, ref ptr, (byte)segment.cmd);
                    encode8u(buffer, ref ptr, (byte)segment.frg);
                    encode16u(buffer, ref ptr, (ushort)segment.wnd);
                    encode32u(buffer, ref ptr, segment.ts);
                    encode32u(buffer, ref ptr, segment.sn);
                    encode32u(buffer, ref ptr, segment.una);
                    encode32u(buffer, ref ptr, segment.len);

                    if (segment.len > 0)
                    {
                        Buffer.BlockCopy(segment.data, 0, buffer, ptr, (int)segment.len);
                        ptr += (int)segment.len;
                    }

                    if (segment.xmit >= dead_link)
                    {
                        state = uint.MaxValue;
                    }
                }

                p = next;
            }

            // flash remain segments
            size = ptr;
            if (size > 0)
            {
                ikcp_output(buffer, size);
            }

            // update ssthresh
            if (change != 0)
            {
                uint inflight = snd_nxt - snd_una;
                ssthresh = inflight / 2;
                if (ssthresh < IKCP_THRESH_MIN) ssthresh = IKCP_THRESH_MIN;
                cwnd = ssthresh + resent;
                incr = cwnd * mss;
            }

            if (lost != 0)
            {
                ssthresh = cwnd_ / 2;
                if (ssthresh < IKCP_THRESH_MIN) ssthresh = IKCP_THRESH_MIN;
                cwnd = 1;
                incr = mss;
            }

            if (cwnd < 1)
            {
                cwnd = 1;
                incr = mss;
            }
        }

        public void Update(uint current)
        {
            this.current = current;
            if (this.updated == 0)
            {
                this.updated = 1;
                this.ts_flush = this.current;
            }

            int slap = _itimediff(this.current, this.ts_flush);

            if (slap >= 10000 || slap < -10000)
            {
                this.ts_flush = this.current;
                slap = 0;
            }

            if (slap >= 0)
            {
                this.ts_flush += this.interval;
                if (_itimediff(this.current, this.ts_flush) >= 0)
                {
                    this.ts_flush = this.current + this.interval;
                }
                Flush();
            }
        }

        public uint Check(uint current)
        {
            uint ts_flush = this.ts_flush;
            int tm_flush = int.MaxValue;
            int tm_packet = int.MaxValue;
            uint minimal = 0;

            if (this.updated == 0)
            {
                return current;
            }

            if (_itimediff(current, ts_flush) >= 10000 || _itimediff(current, ts_flush) < -10000)
            {
                ts_flush = current;
            }

            if (_itimediff(current, ts_flush) >= 0)
            {
                return current;
            }

            tm_flush = _itimediff(ts_flush, current);

            var p = snd_buf.First;
            while (p != null)
            {
                var seg = p.Value;
                int diff = _itimediff(seg.resendts, current);
                if (diff <= 0) return current;
                if (diff < tm_packet) tm_packet = diff;
                p = p.Next;
            }

            minimal = (uint)(tm_packet < tm_flush ? tm_packet : tm_flush);
            if (minimal >= interval) minimal = interval;
            return current + minimal;
        }

        #endregion

        #region setters / config

        public int SetMtu(int mtu)
        {
            if (mtu < 50 || mtu < IKCP_OVERHEAD) return -1;
            try
            {
                this.buffer = new byte[(mtu + IKCP_OVERHEAD) * 3];
            }
            catch
            {
                return -2;
            }
            this.mtu = (uint)mtu;
            this.mss = this.mtu - IKCP_OVERHEAD;
            return 0;
        }

        public int Interval(int interval)
        {
            if (interval > 5000) interval = 5000;
            else if (interval < 10) interval = 10;
            this.interval = (uint)interval;
            return 0;
        }

        public int Nodelay(int nodelay, int interval, int resend, int nc)
        {
            if (nodelay >= 0)
            {
                this.nodelay = (uint)nodelay;
                if (nodelay != 0) this.rx_minrto = (int)IKCP_RTO_NDL;
                else this.rx_minrto = (int)IKCP_RTO_MIN;
            }
            if (interval >= 0)
            {
                if (interval > 5000) interval = 5000;
                else if (interval < 10) interval = 10;
                this.interval = (uint)interval;
            }
            if (resend >= 0) this.fastresend = resend;
            if (nc >= 0) this.nocwnd = nc;
            return 0;
        }

        public int WndSize(int sndwnd, int rcvwnd)
        {
            if (sndwnd > 0) this.snd_wnd = (uint)sndwnd;
            if (rcvwnd > 0) this.rcv_wnd = _imax((uint)rcvwnd, IKCP_WND_RCV);
            return 0;
        }

        public int WaitSnd()
        {
            return (int)(nsnd_buf + nsnd_que);
        }

        public static uint GetConv(ReadOnlySpan<byte> ptr)
        {
            int off = 0;
            return decode32u(ptr, ref off);
        }

        #endregion

        #region private misc

        private int wndUnused()
        {
            if (nrcv_que < rcv_wnd) return (int)(rcv_wnd - nrcv_que);
            return 0;
        }

        #endregion
    }
}
