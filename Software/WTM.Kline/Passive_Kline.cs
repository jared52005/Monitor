using System;
using System.Collections.Generic;
using System.Linq;
using System.Net.NetworkInformation;
using System.Text;
using System.Threading;
using System.Threading.Tasks;

namespace WTM.KLine
{
    /// <summary>
    /// Parsing of KLINE bytes into Key Bytes, ISO14230 or KW1281
    /// </summary>
    internal class Passive_Kline : IDisposable
    {

        const int KLINE_BUFFER_SIZE = 0x200;
        const int KLINE_LAST_ACTIVTY_DELAY = 3000; //3000 ms

        enum Kline5BaudInit
        {
            K5I_ConnectionPattern_55,
            K5I_Kb1,
            K5I_Kb2,
            K5I_NKb2,
            K5I_NEcuAddress,
        }
        enum KlineBusState
        {
            /// <summary>
            /// Nothing is happening on the KLINE. Automatically slips into idle after 2~3 seconds and erase buffer
            /// </summary>
            KBS_Idle,
            /// <summary>
            /// Try to parse incomming data as ISO14230. Will get set after 5 baud init or used as default if 5 baud is not parsed correctly
            /// </summary>
            KBS_Iso14230,
            /// <summary>
            /// Try to parse incomming data as KW1281. Will ONLY happen after 5 baud init.
            /// </summary>
            KBS_Kw1281,
        }
        enum KlineKw1281
        {
            Kw1281_Data,
            Kw1281_DataComplement,
            Kw1281_Etx,
        }
        enum KlineIso14230
        {
            Iso14230_Fmt,
            Iso14230_Tgt,
            Iso14230_Src,
            Iso14230_Len,
            Iso14230_Data,
            Iso14230_Cs,
        }
        enum KlineIso14230_FrameParseResult
        {
            Iso14230_Good,
            Iso14230_InvalidCs,
            Iso14230_NotEnoughData,
            Iso14230_InvalidState,
        }
        
        // -- Private variables ---------------------------
        KlineBusState kline_bus_state = KlineBusState.KBS_Idle; //By default parse data as ISO14230
        byte[] _kline_buffer = new byte[KLINE_BUFFER_SIZE];
        //byte[] _kline_frame = new byte[0x110]; //Frame to be sent to Wireshark
        int _kline_buffer_end = 0;
        ulong _kline_last_activity;
        DateTime _start;

        bool _cancelUpdateThread;
        Thread _updateThread;

        public event EventHandler<RawMessage> OnRawFrame;

        public Passive_Kline()
        {
            _start = DateTime.Now;
            _updateThread = new Thread(Passive_Kline_UpdateState);
            _updateThread.Start();
        }


        void Passive_Kline_PrintBuffer(int start, int length)
        {
            Console.WriteLine(BitConverter.ToString(_kline_buffer, start, length));
        }

        ulong GetTime_ms()
        {
            TimeSpan diff = DateTime.Now - _start;
            return (ulong)diff.TotalMilliseconds;
        }

        public void Dispose()
        {
            _cancelUpdateThread = true;
        }

        void Passive_Kline_UpdateState()
        {
            _cancelUpdateThread = false;
            while (!_cancelUpdateThread)
            {
                if ((_kline_last_activity + KLINE_LAST_ACTIVTY_DELAY) < GetTime_ms())
                {
                    if (_kline_buffer_end != 0 || kline_bus_state != KlineBusState.KBS_Idle)
                    {
                        if (_kline_buffer_end != 0)
                        {
                            Passive_Kline_PrintBuffer(0, _kline_buffer_end);
                        }
                        kline_bus_state = KlineBusState.KBS_Idle;
                        _kline_buffer_end = 0;
                        Console.WriteLine("\nKLINE Reset back to default @ {0} ms", GetTime_ms());
                    }
                }
                Thread.Sleep(50);
            }
        }

        KlineIso14230_FrameParseResult Passive_Kline_SearchIso14230_Frame(int offset, ref int start, ref int end)
        {
            int i;
            byte frame_Fmt = 0; //Expected Fmt
            byte frame_Len = 0; //Expected length of a payload
            uint frame_ExpectedLength = 0; //Length of whole frame
            uint frame_FoundLength = 0;
            uint frame_Cs = 0;
            KlineIso14230 iso14230_sm = KlineIso14230.Iso14230_Fmt;
            for (i = offset; i < _kline_buffer_end; i++)
            {
                switch (iso14230_sm)
                {
                    case KlineIso14230.Iso14230_Fmt:
                        start = i;
                        frame_FoundLength = 1;
                        frame_ExpectedLength = 1; //FMT + (CS)
                        frame_Cs = _kline_buffer[i];
                        frame_Fmt = _kline_buffer[i];

                        //Check if length is in FMT
                        frame_Len = (byte)(frame_Fmt & 0x3F);
                        if (frame_Len != 0)
                        {
                            frame_ExpectedLength = frame_ExpectedLength + frame_Len;
                            if (frame_ExpectedLength > _kline_buffer_end)
                            {
                                //Not all data were received
                                return KlineIso14230_FrameParseResult.Iso14230_NotEnoughData;
                            }
                        }

                        //Check if we have address information
                        if ((frame_Fmt & 0x80) == 0x80)
                        {
                            //Yes we do. Next state will be TGT / SRC
                            frame_ExpectedLength += 2;
                            iso14230_sm = KlineIso14230.Iso14230_Tgt;
                        }
                        else
                        {
                            //No we don't. Set next state by frame_Len
                            if (frame_Len == 0)
                            {
                                //FMT.LEN == 0, so next should be LEN byte
                                iso14230_sm = KlineIso14230.Iso14230_Len;
                            }
                            else
                            {
                                //FMT.LEN != 0, so next should be Data byte(s)
                                iso14230_sm = KlineIso14230.Iso14230_Data;
                            }
                        }
                        break;
                    case KlineIso14230.Iso14230_Tgt:
                        frame_FoundLength++;
                        frame_Cs += _kline_buffer[i];
                        iso14230_sm = KlineIso14230.Iso14230_Src;
                        break;
                    case KlineIso14230.Iso14230_Src:
                        frame_FoundLength++;
                        frame_Cs += _kline_buffer[i];
                        if (frame_Len == 0)
                        {
                            iso14230_sm = KlineIso14230.Iso14230_Len;
                        }
                        else
                        {
                            iso14230_sm = KlineIso14230.Iso14230_Data;
                        }
                        break;
                    case KlineIso14230.Iso14230_Len:
                        frame_FoundLength++;
                        frame_Cs += _kline_buffer[i];
                        frame_Len = _kline_buffer[i];
                        frame_ExpectedLength++; //LEN byte
                        frame_ExpectedLength = frame_ExpectedLength + frame_Len;
                        if (frame_ExpectedLength > _kline_buffer_end)
                        {
                            //Not all data were received
                            return KlineIso14230_FrameParseResult.Iso14230_NotEnoughData;
                        }
                        iso14230_sm = KlineIso14230.Iso14230_Data;
                        break;
                    case KlineIso14230.Iso14230_Data:
                        frame_FoundLength++;
                        frame_Cs += _kline_buffer[i];
                        if (frame_FoundLength == frame_ExpectedLength)
                        {
                            iso14230_sm = KlineIso14230.Iso14230_Cs;
                        }
                        break;
                    case KlineIso14230.Iso14230_Cs:
                        if ((byte)frame_Cs == _kline_buffer[i])
                        {
                            end = i + 1;
                            return KlineIso14230_FrameParseResult.Iso14230_Good;
                        }
                        else
                        {
                            //printf("ISO14230: Invalid CS %x vs %x\n", frame_Cs, kline_buffer[i]);
                            return KlineIso14230_FrameParseResult.Iso14230_InvalidCs;
                        }
                    default:
                        return KlineIso14230_FrameParseResult.Iso14230_InvalidState;
                }
            }
            return KlineIso14230_FrameParseResult.Iso14230_NotEnoughData;
        }

        bool Passive_Kline_SearchIso14230(ref int start, ref int end)
        {
            int offset;
            bool result = false;
            KlineIso14230_FrameParseResult frameParseResult;
            //ISO14230 packet must be at least 3 bytes long
            if (_kline_buffer_end < 3)
            {
                return false;
            }
            //Search through buffer
            offset = 0;
            //for(offset = 0; offset < (buffer_length - 3); offset++)
            {
                frameParseResult = Passive_Kline_SearchIso14230_Frame(offset, ref start, ref end);
                if (frameParseResult == KlineIso14230_FrameParseResult.Iso14230_Good)
                {
                    //Process frame
                    result = true;
                    //break;
                }
                else if (frameParseResult == KlineIso14230_FrameParseResult.Iso14230_NotEnoughData)
                {
                    //Wait on more data
                    result = false;
                    //break;
                }
                else
                {
                    _kline_buffer_end = 0;
                    result = false;
                }
            }

            return result;
        }

        bool Passive_Kline_SearchKw1281(ref int start, ref int end)
        {
            int i;
            int expectedLength = 0;
            bool result = false;
            int length = 0;
            int etx = 0x03;
            byte data = 0;
            byte data_c = 0;
            KlineKw1281 kw1281_sm = KlineKw1281.Kw1281_Data;
            /* 0F f0   Length
               1 fe    Block ID
               f6 9    Block Type
               b4 4b 
               5a a5 
               37 c8
               39 c6 
               30 cf 
               37 c8 
               35 ca 
               35 ca 
               31 ce 
               41 be 
               41 be 
               20 df 
               3      Last byte is ETX without a complement.
               */

            //Go through buffer
            for (i = 0; i < _kline_buffer_end; i++)
            {
                switch (kw1281_sm)
                {
                    case KlineKw1281.Kw1281_Data:
                        data = _kline_buffer[i];
                        kw1281_sm = KlineKw1281.Kw1281_DataComplement;
                        if (length == 0)
                        {
                            start = i;
                        }
                        break;
                    case KlineKw1281.Kw1281_DataComplement:
                        data_c = (byte)(~_kline_buffer[i]);
                        if (data == data_c)
                        {
                            length++;
                            if (length == 1)
                            {
                                expectedLength = data;
                            }
                            if (length == expectedLength)
                            {
                                kw1281_sm = KlineKw1281.Kw1281_Etx;
                            }
                            else
                            {
                                kw1281_sm = KlineKw1281.Kw1281_Data;
                            }
                        }
                        else
                        {
                            length = 0;
                            kw1281_sm = KlineKw1281.Kw1281_Data;
                        }
                        break;
                    case KlineKw1281.Kw1281_Etx:
                        data = _kline_buffer[i];
                        if (data == etx)
                        {
                            end = i + 1;
                            result = true;
                        }
                        break;
                    default:
                        break;
                }
            }
            return result;
        }

        /// <summary>
        /// Recognize 5 baud init [00 00] 55 kb1 kb2 ~kb2 addr
        /// </summary>
        /// <param name="start"></param>
        /// <param name="end"></param>
        /// <returns></returns>
        bool Passive_Kline_SearchKeyBytes(ref int start, ref int end)
        {
            int i;
            int length = 0;
            //byte kb1;
            byte kb2 = 0;
            //byte ecuAddress;
            byte nKb2 = 0;
            Kline5BaudInit init_sm = Kline5BaudInit.K5I_ConnectionPattern_55;
            bool result = false;

            //Go through ring buffer
            for (i = 0; i < _kline_buffer_end; i++)
            {
                switch (init_sm)
                {
                    case Kline5BaudInit.K5I_ConnectionPattern_55:
                        if (_kline_buffer[i] == 0x55)
                        {
                            start = i;
                            length = 1;
                            init_sm = Kline5BaudInit.K5I_Kb1;
                        }
                        break;
                    case Kline5BaudInit.K5I_Kb1:
                        //kb1 = kline_buffer[i];
                        init_sm = Kline5BaudInit.K5I_Kb2;
                        length++;
                        break;
                    case Kline5BaudInit.K5I_Kb2:
                        kb2 = _kline_buffer[i];
                        init_sm = Kline5BaudInit.K5I_NKb2;
                        length++;
                        break;
                    case Kline5BaudInit.K5I_NKb2:
                        nKb2 = (byte)(~_kline_buffer[i]);
                        length++;
                        if (nKb2 == kb2 && length == 4)
                        {
                            if (kb2 == 0x8A)
                            {
                                //KW1281 is not sending back ECU address
                                kline_bus_state = KlineBusState.KBS_Kw1281;
                                end = i + 1;
                                result = true;
                            }
                            init_sm = Kline5BaudInit.K5I_NEcuAddress;
                        }
                        else
                        {
                            //Reset SM
                            init_sm = Kline5BaudInit.K5I_ConnectionPattern_55;
                        }
                        break;
                    case Kline5BaudInit.K5I_NEcuAddress:
                        if (kb2 == 0x8F)
                        {
                            kline_bus_state = KlineBusState.KBS_Iso14230;
                            end = i + 1;
                            result = true;
                        }
                        else
                        {
                            Console.WriteLine("Unknown KLINE protocol: %x", kb2);
                        }
                        break;
                    default:
                        break;
                }
            }

            return result;
        }

        /// <summary>
        /// Dequeue data from ring buffer, including junk data between start of buffer and start of packet
        /// </summary>
        /// <param name="start"></param>
        /// <param name="end"></param>
        void Passive_Kline_Dequeue_Iso14230(int start, int end)
        {
            int i;
            int framePos;
            //Dequeue crap
            if (start != 0)
            {
                Console.Write("\nISO14230 crap bytes: ");
                Passive_Kline_PrintBuffer(0, start);
            }

            //Dequeue data
            framePos = 0;
            RawMessage msg = new RawMessage(end - start);
            for (i = start; i != end; i++)
            {
                msg.Frame[framePos] = _kline_buffer[i];
                //_kline_frame[framePos] = _kline_buffer[i];
                _kline_buffer[i] = 0x00;
                framePos++;
            }
            msg.MessageType = RawMessageType.Raw_ISO14230;
            msg.Timestamp = GetTime_ms();
            //Console.WriteLine("\nISO14230 data: " + BitConverter.ToString(msg.Frame, 0, msg.Frame.Length));
            OnRawFrame?.Invoke(this, msg);
            _kline_buffer_end = 0;
        }

        /// <summary>
        /// Dequeue data from ring buffer, including junk data between start of buffer and start of packet
        /// </summary>
        /// <param name="start"></param>
        /// <param name="end"></param>
        void Passive_Kline_Dequeue_Kw1281(int start, int end)
        {
            int i;
            int framePos;
            int length;
            //Dequeue crap
            if (start != 0)
            {
                Console.WriteLine("KW1281 crap bytes: ");
                Passive_Kline_PrintBuffer(0, start);
            }

            //Dequeue data
            framePos = 0;
            length = 0;
            RawMessage msg = new RawMessage((end - start) / 2);
            for (i = start; i != end; i++)
            {
                //Write only even positions. Odd positions are complements
                if (length % 2 == 0)
                {
                    msg.Frame[framePos] = _kline_buffer[i];
                    //printf("%x ", kline_buffer[i]);
                    //Check for End Communication command
                    if ((framePos == 2) && (msg.Frame[framePos] == 0x06))
                    {
                        //If correct, reset kline bus status
                        Console.WriteLine("KLINE bus reset from KW1281 command\n");
                        kline_bus_state = KlineBusState.KBS_Idle;
                    }
                    framePos++;
                }
                _kline_buffer[i] = 0x00;
                length++;
            }
            msg.MessageType = RawMessageType.Raw_KW1281;
            msg.Timestamp = GetTime_ms();
            //Console.WriteLine("\nKW1281 data: " + BitConverter.ToString(msg.Frame, 0, framePos));
            OnRawFrame?.Invoke(this, msg);
            _kline_buffer_end = 0;
        }
        /// <summary>
        /// Try to parse KLINE bytes.Result can be Key Bytes, ISO14230 or KW1281
        /// </summary>
        /// <param name="c"></param>
        /// <returns>True if successfuly processed</returns>
        public bool Passive_Kline_Parse(byte c)
        {
            int start = 0;
            int end = 0;
            //Write data into ring buffer
            _kline_buffer[_kline_buffer_end] = c;
            _kline_buffer_end++;
            //Check if overflow
            if (_kline_buffer_end == KLINE_BUFFER_SIZE)
            {
                Console.WriteLine("KLINE buffer overflow: ");
                Passive_Kline_PrintBuffer(0, _kline_buffer_end - 1);
                _kline_buffer_end = 0;
                kline_bus_state = KlineBusState.KBS_Idle;
                return false;
            }

            _kline_last_activity = GetTime_ms();

            //Process data in the buffer
            switch (kline_bus_state)
            {
                case KlineBusState.KBS_Idle:
                    //Try to parse keybytes
                    if (Passive_Kline_SearchKeyBytes(ref start, ref end))
                    {
                        Passive_Kline_Dequeue_Iso14230(start, end);
                    }
                    else
                    {
                        //Try to parse it as ISO14230
                        if (Passive_Kline_SearchIso14230(ref start, ref end))
                        {
                            //Success, switch state of bus into ISO14230
                            kline_bus_state = KlineBusState.KBS_Iso14230;
                            Passive_Kline_Dequeue_Iso14230(start, end);
                        }
                    }
                    break;
                case KlineBusState.KBS_Iso14230:
                    if (Passive_Kline_SearchIso14230(ref start, ref end))
                    {
                        Passive_Kline_Dequeue_Iso14230(start, end);
                    }
                    break;
                case KlineBusState.KBS_Kw1281:
                    if (Passive_Kline_SearchKw1281(ref start, ref end))
                    {
                        Passive_Kline_Dequeue_Kw1281(start, end);
                    }
                    break;
                default:
                    Console.WriteLine("KLINE: Unknown protocol");
                    break;
            }
            return true;
        }
    }
}
