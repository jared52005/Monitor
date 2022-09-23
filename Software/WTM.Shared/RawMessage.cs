using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace WTM
{
    public class RawMessage
    {
        /// <summary>
        /// Type added into IP RAW packet, which will be used for selection of Wireshark dissector
        /// </summary>
        public RawMessageType MessageType { get; set; }
        /// <summary>
        /// When were data received
        /// </summary>
        public ulong Timestamp { get; set; }
        /// <summary>
        /// Data which we want to dissect
        /// </summary>
        public byte[] Frame { get; }
        public uint Id { get; set; }
        public RawMessage(int length)
        {
            Frame = new byte[length];
        }
    }
}
