using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Xml.Linq;

namespace WTM
{
    /// <summary>
    /// Container for CAN message
    /// </summary>
    public class CanMessage
    {
        public long Timestamp { get; set; }
        /// <summary>
        /// Body of message
        /// </summary>
        public byte[] Data { get; }

        /// <summary>
        /// CAN ID of message
        /// </summary>
        public int ID { get; }

        /// <summary>
        /// DLC of message
        /// </summary>
        public int DLC { get { return Data.Length; } }

        /// <summary>
        /// Creating CAN message, data field and CAN ID
        /// </summary>
        /// <param name="data">Data of message</param>
        /// <param name="id">CAN ID of message</param>
        public CanMessage(byte[] data, int id)
        {
            ID = id;

            //Check if data is maximally 8 bytes long. If not make them
            if (data.Length > 8)
            {
                Data = new byte[8];
            }
            else
            {
                Data = new byte[data.Length];
            }

            Buffer.BlockCopy(data, 0, Data, 0, data.Length);
        }

        /// <summary>
        /// Creates string from message
        /// </summary>
        /// <returns>String like i.e.: 7F1 [0D 3F 0D 2A 0D 4C]</returns>
        public override string ToString()
        {
            string msg = string.Format("{0:X3} [{1}]", ID, BitConverter.ToString(Data).Replace('-', ' '));
            return msg;
        }
    }
}
