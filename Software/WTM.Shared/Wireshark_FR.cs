using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Net.Sockets;
using System.Net;
using System.Text;
using System.Threading;

namespace WTM
{
    /// <summary>
    /// Send FlexRay data into Wireshark
    /// </summary>
    public class Wireshark_FR : IDisposable
    {
        readonly Object _syncObject = new Object();

        const int _port = 19002;
        Thread _listenThread;
        Queue<FlexRayMessage> _qRaw;
        bool _cancelThread;
        TcpListener _server;

        public TcpClient Client { get; private set; }

        public Wireshark_FR()
        {
            _qRaw = new Queue<FlexRayMessage>();
            _listenThread = new Thread(Listen);
            _listenThread.Start();
        }

        public bool IsConnected
        {
            get 
            {
                if (Client == null)
                {
                    return false;
                }
                return Client.Connected;
            }
        }

        public void Add(FlexRayMessage msg)
        {
            if (Client == null)
            {
                return;
            }
            if (!Client.Connected)
            {
                return;
            }
            lock (_syncObject)
            {
                _qRaw.Enqueue(msg);
            }
        }

        private void Listen()
        {
            _cancelThread = false;

            try
            {
                _server = new TcpListener(IPAddress.Any, _port);
                // Start listening for client requests.
                _server.Start();
                Console.WriteLine($"Waiting on connection on {_port}");

                while (!_cancelThread)
                {
                    Client = _server.AcceptTcpClient();
                    Console.WriteLine("Client connected");
                    try
                    {
                        Send();
                    }
                    catch (IOException)
                    {
                        Console.WriteLine("Client disonnected");
                    }
                    lock (_syncObject)
                    {
                        _qRaw.Clear();
                    }
                }
            }
            catch (SocketException ex)
            {
                Console.WriteLine("Server-SocketException" + ex.Message);
            }
            finally
            {
                //server.Stop();
            }
        }

        private void Send()
        {
            NetworkStream stream = Client.GetStream();

            //Write header
            byte[] fileHeader =
            {
                0xD4, 0xC3, 0xB2, 0xA1, 0x02, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                0xFF, 0xFF, 0x00, 0x00, 0xD2, 0x00, 0x00, 0x00
            };
            stream.Write(fileHeader, 0, fileHeader.Length);

            while (!_cancelThread)
            {
                //Check if we are still connected
                if (!Client.Connected)
                {
                    return;
                }
                //If we have more than one element in queue, process it, or wait 20ms
                if (_qRaw.Count > 0)
                {
                    FlexRayMessage rmsg;
                    lock (_syncObject)
                    {
                        rmsg = _qRaw.Dequeue();
                    }
                    byte[] header = PreapreHeader(rmsg);
                    stream.Write(header, 0, header.Length);

                    byte[] payload = PreparePayload(rmsg);
                    stream.Write(payload, 0, payload.Length);
                }
                else
                {
                    Thread.Sleep(20);
                }
            }
        }

        private byte[] PreapreHeader(FlexRayMessage fmsg)
        {
            /* 00-00-00-00 00-00-00-00-10-00-00-00-10-00-00-00
             * Where:
             * 00-00-00-00 = Time Stamp seconds.
             * 00-00-00-00 = Time Stamp micro seconds
             * 10-00-00-00 = Size of packet saved in a file = 20 bytes of header + Frame length
             * 10-00-00-00 = Actual size of packet = 20 bytes of header + Frame length
             */

            byte[] header = new byte[16];

            uint timestamp_seconds = (uint)(fmsg.Timestamp / 1000); //Only second part (I know there should be Unix time, but I am too lazy to get RTC or NTP working)
            uint timestamp_microseconds = (uint)(fmsg.Timestamp % 1000); //Only remainder from seconds
            timestamp_microseconds = timestamp_microseconds * 1000; //Convert milisecond to microseconds

            WriteLE(header, timestamp_seconds, 0);
            WriteLE(header, timestamp_microseconds, 4);
            uint packetLength = 7; //FlexRay header size
            if(fmsg.Data != null)
            {
                packetLength = (uint)(fmsg.Data.Length + 7);
            }
            WriteLE(header, packetLength, 8);
            WriteLE(header, packetLength, 12);

            return header;
        }

        private byte[] PreparePayload(FlexRayMessage fmsg)
        {
            int length = 7;
            if (fmsg.Data != null)
            {
                length += fmsg.Data.Length;
            }
            byte[] wsFrFrame = new byte[length];

            //Measurement header
            wsFrFrame[0] = 1; //FlexRay Frame
            //Error flags
            //TBD [1]
            //FlexRay header has 5 bytes, composed as
            //Flags-FID-DLC-HCRC-CYC
            //    5- 11-  7-  11-  6 = 40 bits = 5 bytes

            //Null Frame is active in 0. Invert the bit
            fmsg.Flags = fmsg.Flags ^ FlexRayFlags.FLAG_NullFrameIndicator;

            wsFrFrame[2] = (byte)((int)fmsg.Flags << 3 | ((fmsg.FrameId >> 8) & 3));
            wsFrFrame[3] = (byte)(fmsg.FrameId & 0xFF);
            wsFrFrame[4] = (byte)(fmsg.PayloadLength << 1 | ((fmsg.HeaderCrc >> 10) & 1));
            wsFrFrame[5] = (byte)((fmsg.HeaderCrc >> 2) & 0xFF);
            wsFrFrame[6] = (byte)(fmsg.HeaderCrc << 6 | (fmsg.CycleCount & 0x3F));

            //Copy data
            if (fmsg.Data != null)
            {
                Buffer.BlockCopy(fmsg.Data, 0, wsFrFrame, 7, fmsg.Data.Length);
            }
            return wsFrFrame;
        }

        private void WriteLE(byte[] array, uint data, int offset)
        {
            array[offset] = (byte)data;
            array[offset + 1] = (byte)(data >> 8);
            array[offset + 2] = (byte)(data >> 16);
            array[offset + 3] = (byte)(data >> 24);
        }

        public void Dispose()
        {
            _cancelThread = true;
            _server.Stop();

            if (Client != null)
            {
                if (Client.Connected)
                {
                    Client.Close();
                }
            }
        }
    }
}
