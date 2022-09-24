using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.Remoting.Messaging;
using System.Text;

namespace WTM
{
    /// <summary>
    /// FlexRay - Protocol Specification V2.1 Rev.A.pdf pg 90
    /// </summary>
    public enum FlexRayFlags
    {
        FLAG_StartupFrameIndicator = 1,
        FLAG_SyncFrameIndicator = 2,
        FLAG_NullFrameIndicator = 4,
        FLAG_PayloadPreambleIndicator = 8,
        FLAG_ReservedBit = 0x10,
    }
    /// <summary>
    /// Setup of FlexRay message as described in FlexRay v2.1 specification
    /// </summary>
    public class FlexRayMessage
    {
        public long Timestamp { get; set; }
        /// <summary>
        /// TBD - Create enum with list of possible flags
        /// </summary>
        public FlexRayFlags Flags { get; set; }
        /// <summary>
        /// 11 bit Frame ID
        /// </summary>
        public int FrameId { get; set; }
        /// <summary>
        /// Payload length in uint16. Must be always even.
        /// </summary>
        public int PayloadLength { get; set; }
        /// <summary>
        /// CRC of header
        /// </summary>
        public int HeaderCrc { get; set; }
        /// <summary>
        /// 6 bit cycle count
        /// </summary>
        public int CycleCount { get; set; }
        /// <summary>
        /// Data transferred in the frame
        /// </summary>
        public byte[] Data { get; set; }
        /// <summary>
        /// CRC of whole frame
        /// </summary>
        public int Crc { get; set; }

        /// <summary>
        /// Verify length and CRCs of FlexRay frame
        /// </summary>
        /// <returns></returns>
        public bool IsMessageValid()
        {
            if (PayloadLength != 0)
            {
                //Must contain data
                if (Data == null)
                {
                    return false;
                }
                //Payload Length * 2 is size of data in bytes
                if (Data.Length != PayloadLength * 2)
                {
                    return false;
                }
            }
            if (CycleCount > 63)
            {
                return false;
            }

            if (!VerifyHeaderCrc())
            {
                return false;
            }
            if (!VerifyFrameCrc())
            {
                return false;
            }
            return true;
        }

        private bool VerifyHeaderCrc()
        {
            /*
             * ISO17458 - FlexRay\FlexRay - Protocol Specification V2.1 Rev.A.pdf
             * Page 93 - Verification is 20 bits long. So this mean that I do apply CRC with given polynomial on bits instead of bytes?
             */
            return true;
        }

        private bool VerifyFrameCrc()
        {
            /*
             * ISO17458 - FlexRay\FlexRay - Protocol Specification V2.1 Rev.A.pdf
             * Page 99
             */
            return true;
        }

        /// <summary>
        /// Convert data in the FlexRay frame into human readable form
        /// </summary>
        /// <returns></returns>
        public override string ToString()
        {
            //[FrameId:CyCnt] - data <Invalid>
            string validity = string.Empty;
            if (!IsMessageValid())
            {
                validity = "<Invalid>";
            }
            string data = "NF";
            if (Data != null)
            {
                data = BitConverter.ToString(Data).Replace("-", " ");
            }
            return $"[{FrameId:X}:{CycleCount}] {data} {validity}";
        }
    }
}
