using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace WTM.Protocols
{
    public class Passive_VWTP20
    {
        // -- Private defintions
        enum TCPI1
        {
            // Last message of consecutive frame. 
            // ACK not expected
            TCPI1_CFrame_LastMessageNoAck = 0x30,
            // 2x - Consecutive frame.
            // ACK not expected
            TCPI1_CFrame_Flow = 0x20,
            // 1x - Last message of consecutive frame.
            // Expected ACK"
            TCPI1_CFrame_LastMessageAck = 0x10,
            // 0x - Block size has been reached, send ACK that RX is still OK.
            TCPI1_CFrame_BlockSizeReachedAck = 0x00,
            // Bx - Ack, "X" is SEQ
            TCPI1_Ack = 0xB0,
            // A0 - Paramters request
            TCPI1_Connection_ParamRequest = 0xA0,
            // A1 - Parameters response. Returning paramters from A0 or A3
            TCPI1_Connection_ParamResponse = 0xA1,
            // A3 - Channel test (keep alive)
            TCPI1_Connection_Test = 0xA3,
            // A4 - Break; Receiver discards all data since last ACK
            TCPI1_Connection_Break = 0xA4,
            // A8	Disconect
            TCPI1_Connection_Disconnect = 0xA8,
            // Unkown TCPI, used as deafult one
            TCPI1_Unkown,
        }

        // -- Private variables
        uint testerId = 0xFFFFFFFF;
        uint ecuId = 0xFFFFFFFF;
        byte[] tp20_frame = new byte[0x100];
        int tp20_frame_count;
        bool flag_DatagramReceived = false;
        bool flag_expectingAck = false;

        public event EventHandler<RawMessage> OnRawFrame;

        /// <summary>
        /// Method is checking for 0x200~0x2FF CAN IDs
        /// </summary>
        /// <param name="msg"></param>
        /// <returns></returns>
        bool Passive_Vwtp20_BroadcastChannel(CanMessage msg)
        {
            //Only Messages in brodcast channel 0x200 ~ 0x2FF and DLC == 7
            if (msg.Id > 0x300 || (msg.Id & 0xF00) != 0x200 || msg.Dlc != 7)
            {
                return false;
            }
            byte ecuAddress = msg.Data[0];
            byte opcode = msg.Data[1];
            ushort txid = BitConverter.ToUInt16(msg.Data, 2);
            ushort rxid = BitConverter.ToUInt16(msg.Data, 4);
            //byte appType = msg.Frame[6];
            Console.WriteLine("ECU at address {0:x}. TXID=[{1:x}] / RXID=[{2:x}]", ecuAddress, txid, rxid);

            if (msg.Id == 0x200)
            {
                //Request to ECU on brodacst channel
                ecuId = 0xFFFFFFFF;
                testerId = 0xFFFFFFFF;
            }
            else
            {
                //Response from ECU - Only react on positive response
                if (opcode == 0xD0)
                {
                    ecuId = rxid;
                    testerId = txid;
                    tp20_frame_count = 0;
                    flag_DatagramReceived = false;
                }
            }
            return true;
        }

        bool Passive_Vwtp20_UnicastChannel(CanMessage msg)
        {
            int rtcpi;
            //int seq;
            int id = msg.Id;
            if (id != testerId && id != ecuId)
            {
                return false;
            }

            //Parse TCPI byte
            rtcpi = msg.Data[0];
            //seq = rtcpi & 0xF;
            TCPI1 tcpi = TCPI1.TCPI1_Unkown;
            if ((rtcpi & 0xF0) == 0xA0)
            {
                //Use whole TCPI for parsing - Ax are constants
                tcpi = (TCPI1)rtcpi;
            }
            else
            {
                //Use only first 4 bits for parsing, which are constants
                tcpi = (TCPI1)(rtcpi & 0xF0);
            }
            switch (tcpi)
            {
                case TCPI1.TCPI1_CFrame_LastMessageNoAck:
                    Buffer.BlockCopy(msg.Data, 0, tp20_frame, tp20_frame_count, msg.Dlc);
                    //memcpy(tp20_frame + tp20_frame_count, msg.Frame, msg.Dlc);
                    tp20_frame_count += msg.Dlc;
                    flag_DatagramReceived = true;
                    flag_expectingAck = false;
                    break;
                case TCPI1.TCPI1_CFrame_Flow:
                    //Received one frame from several frames
                    Buffer.BlockCopy(msg.Data, 1, tp20_frame, tp20_frame_count, msg.Dlc - 1);
                    //memcpy(tp20_frame + tp20_frame_count, msg.Frame + 1, msg.Dlc - 1);
                    tp20_frame_count += msg.Dlc - 1;
                    break;
                case TCPI1.TCPI1_CFrame_LastMessageAck:
                    //Received last message of block expecting ACK
                    Buffer.BlockCopy(msg.Data, 1, tp20_frame, tp20_frame_count, msg.Dlc - 1);
                    //memcpy(tp20_frame + tp20_frame_count, msg.Frame + 1, msg.Dlc - 1);
                    tp20_frame_count += msg.Dlc - 1;
                    flag_DatagramReceived = true;
                    flag_expectingAck = true;
                    break;
                case TCPI1.TCPI1_CFrame_BlockSizeReachedAck:
                    //Block has ended, expecting ACK
                    Buffer.BlockCopy(msg.Data, 1, tp20_frame, tp20_frame_count, msg.Dlc - 1);
                    //memcpy(tp20_frame + tp20_frame_count, msg.Frame + 1, msg.Dlc - 1);
                    tp20_frame_count += msg.Dlc - 1;
                    flag_expectingAck = true;
                    break;
                case TCPI1.TCPI1_Ack:
                    //Datagram received
                    flag_expectingAck = false;
                    break;
                case TCPI1.TCPI1_Connection_Break:
                case TCPI1.TCPI1_Connection_Disconnect:
                    if (tcpi == TCPI1.TCPI1_Connection_Disconnect)
                    {
                        //Reset IDs
                        ecuId = 0xFFFFFFFF;
                        testerId = 0xFFFFFFFF;
                    }
                    break;
                default:
                    break;
            }

            if (flag_DatagramReceived == true)
            {
                //Check minimal length
                if (tp20_frame_count < 3)
                {
                    //Too small.
                    Console.WriteLine("TP20 Datagram is smaller than 3 bytes.");
                }
                //Validate datagram header
                else
                {
                    int dLength = tp20_frame[0] << 8 | tp20_frame[1];
                    if (dLength != tp20_frame_count - 2)
                    {
                        //Now there is some kind of special case used for errors (maybe)
                        if ((dLength ^ 0x8000) == tp20_frame_count - 2)
                        {
                            tp20_frame_count = tp20_frame_count - 2;
                            AddNewMessage(msg.Id, msg.Timestamp);
                            Console.WriteLine("TP20 Datagram header starts on 0x8000");
                        }
                        else
                        {
                            //Datagram seems to have invalid header
                            Console.WriteLine("TP20 Datagram has invalid header.");
                        }
                    }
                    else
                    {
                        //Seems fine
                        tp20_frame_count = tp20_frame_count - 2;
                        AddNewMessage(msg.Id, msg.Timestamp);
                    }
                }
                tp20_frame_count = 0;
                flag_DatagramReceived = false;
            }
            return true;
        }

        private void AddNewMessage(int id, long timestamp)
        {
            RawMessage rmsg = new RawMessage(tp20_frame_count);
            rmsg.MessageType = RawMessageType.Raw_VWTP20;
            rmsg.Timestamp = (ulong)timestamp;
            rmsg.Id = (uint)id;
            Buffer.BlockCopy(tp20_frame, 2, rmsg.Frame, 0, tp20_frame_count);
            OnRawFrame(this, rmsg);
        }

        /// <summary>
        /// Parse incomming messages into VWTP2.0
        /// </summary>
        /// <param name="cmsg"></param>
        /// <returns></returns>
        public bool Passive_Vwtp20_Parse(CanMessage cmsg)
        {
            if (Passive_Vwtp20_BroadcastChannel(cmsg) == true)
            {
                return true;
            }
            if (Passive_Vwtp20_UnicastChannel(cmsg) == true)
            {
                return true;
            }
            return false;
        }

    }
}
