using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace WTM.Protocols
{
    public class Passive_ISO15765
    {
        // -- Private variables
        byte[] iso15765_frame = new byte[4096];
        int iso15765_frame_position = 0;
        int iso15765_frame_expectedLength = 0;
        int iso15765_frame_expectedSN;

        public event EventHandler<RawMessage> OnRawFrame;

        void Passive_Iso15765_VerifyPreviousDatagram()
        {
            if (iso15765_frame_position != 0)
            {
                Console.WriteLine("We are trying to process another datagram, even that we still have datagram with length 0x{0:x} bytes in buffer", iso15765_frame_position);
                Console.WriteLine("Datagram is still missing 0x{0:x} bytes to be complete", iso15765_frame_expectedLength);
                iso15765_frame_position = 0;
            }
        }

        void Passive_Iso15765_SingleFrame(CanMessage cmsg)
        {
            int i;
            Passive_Iso15765_VerifyPreviousDatagram();
            int length = cmsg.Data[0] & 0xF;
            if (length > cmsg.Dlc - 1)
            {
                return;
            }

            for (i = 1; i < length + 1; i++)
            {
                iso15765_frame[iso15765_frame_position] = cmsg.Data[i];
                iso15765_frame_position++;
            }
            AddNewMessage(cmsg.Id, cmsg.Timestamp);
            iso15765_frame_position = 0;
        }

        void Passive_Iso15765_FirstFrame(CanMessage cmsg)
        {
            int i;
            Passive_Iso15765_VerifyPreviousDatagram();
            iso15765_frame_expectedLength = ((cmsg.Data[0] & 0xF) << 8) | cmsg.Data[1];
            iso15765_frame_expectedSN = 1; //Always starting on 1
            for (i = 2; i < cmsg.Dlc; i++)
            {
                iso15765_frame[iso15765_frame_position] = cmsg.Data[i];
                iso15765_frame_position++;
                iso15765_frame_expectedLength--;

                if (iso15765_frame_expectedLength == 0)
                {
                    AddNewMessage(cmsg.Id, cmsg.Timestamp);
                    iso15765_frame_position = 0;
                    break;
                }
            }
        }

        void Passive_Iso15765_ConsequtiveFrame(CanMessage cmsg)
        {
            int i;
            int receivedSN;
            if ((cmsg.Data[0] & 0xF) == iso15765_frame_expectedSN)
            {
                iso15765_frame_expectedSN++;
                iso15765_frame_expectedSN = iso15765_frame_expectedSN & 0xF;
            }
            else
            {
                receivedSN = (cmsg.Data[0] & 0xF);
                Console.WriteLine("Expected S/N = 0x{0:X}. Provided 0x{1:X}", iso15765_frame_expectedSN, receivedSN);
                iso15765_frame_expectedSN = receivedSN + 1;
            }

            for (i = 1; i < cmsg.Dlc; i++)
            {
                iso15765_frame[iso15765_frame_position] = cmsg.Data[i];
                iso15765_frame_position++;
                iso15765_frame_expectedLength--;

                if (iso15765_frame_expectedLength == 0)
                {
                    AddNewMessage(cmsg.Id, cmsg.Timestamp);
                    iso15765_frame_position = 0;
                    break;
                }
            }
        }

        private void AddNewMessage(int id, long timestamp)
        {
            RawMessage rmsg = new RawMessage(iso15765_frame_position);
            rmsg.MessageType = RawMessageType.Raw_ISO15765;
            rmsg.Timestamp = (ulong)timestamp;
            rmsg.Id = (uint)id;
            Buffer.BlockCopy(iso15765_frame, 0, rmsg.Frame, 0, iso15765_frame_position);
            OnRawFrame(this, rmsg);
        }

        /// <summary>
        /// Try to parse CAN as ISO15765 protocol
        /// </summary>
        /// <param name="cmsg"></param>
        /// <returns>True if successfuly processed</returns>
        public bool Passive_Iso15765_Parse(CanMessage cmsg)
        {
            int NPCI;
            if (cmsg.Dlc == 0)
            {
                //Only non-zero lengths
                return false;
            }
            NPCI = cmsg.Data[0] >> 4;

            //Switch according the first 4 bits
            switch (NPCI)
            {
                case 0:
                    Passive_Iso15765_SingleFrame(cmsg);
                    break;
                case 1:
                    Passive_Iso15765_FirstFrame(cmsg);
                    break;
                case 2:
                    Passive_Iso15765_ConsequtiveFrame(cmsg);
                    break;
                case 3:
                    //Do nothing with flow control frame
                    break;
                default:
                    Console.WriteLine("Invalid PCI byte was provided!");
                    return false;
            }
            return true;
        }

    }
}
