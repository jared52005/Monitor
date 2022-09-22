using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace KlineMonitor
{
    internal class RawMessage
    {
        public RawMessageType MessageType { get; set; }
        public ulong Timestamp { get; set; }
        public byte[] Frame { get; }

        public RawMessage(int length)
        {
            Frame = new byte[length];
        }
    }
}
