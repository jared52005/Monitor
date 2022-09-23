using System;
using System.Collections.Generic;
using System.Linq;
using System.Net.Sockets;
using System.Net;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using System.IO;

namespace WTM
{
    /// <summary>
    /// Send IP RAW data into Wireshark
    /// </summary>
    public class Wireshark_Raw : IDisposable
    {
        readonly Object _syncObject = new Object();

        const int _port = 19000;
        Thread _listenThread;
        Queue<RawMessage> _qRaw;
        bool _cancelThread;
        TcpListener _server;

        public TcpClient Client { get; private set; }

        public Wireshark_Raw()
        {
            _qRaw = new Queue<RawMessage>();
            _listenThread = new Thread(Listen);
            _listenThread.Start();
        }

        public void Add(RawMessage msg)
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
                    catch(IOException)
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
                0xFF, 0xFF, 0x00, 0x00, 0x65, 0x00, 0x00, 0x00
            };
            stream.Write(fileHeader, 0, fileHeader.Length);

            while(!_cancelThread)
            {
                //Check if we are still connected
                if(!Client.Connected)
                {
                    return;
                }
                //If we have more than one element in queue, process it, or wait 20ms
                if (_qRaw.Count > 0)
                {
                    RawMessage rmsg;
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

        private byte[] PreapreHeader(RawMessage rmsg)
        {
            /* 00-00-00-00 00-00-00-00-10-00-00-00-10-00-00-00
             * Where:
             * 00-00-00-00 = Time Stamp seconds.
             * 00-00-00-00 = Time Stamp micro seconds
             * 10-00-00-00 = Size of packet saved in a file = 20 bytes of header + Frame length
             * 10-00-00-00 = Actual size of packet = 20 bytes of header + Frame length
             */

            byte[] header = new byte[16];

            uint timestamp_seconds = (uint)(rmsg.Timestamp / 1000); //Only second part (I know there should be Unix time, but I am too lazy to get RTC or NTP working)
            uint timestamp_microseconds = (uint)(rmsg.Timestamp % 1000); //Only remainder from seconds
            timestamp_microseconds = timestamp_microseconds * 1000; //Convert milisecond to microseconds

            WriteLE(header, timestamp_seconds, 0);
            WriteLE(header, timestamp_microseconds, 4);
            WriteLE(header, (uint)(rmsg.Frame.Length + 20), 8);
            WriteLE(header, (uint)(rmsg.Frame.Length + 20), 12);

            return header;
        }

        private byte[] PreparePayload(RawMessage rmsg)
        {
            ushort sequence = 0;

            int totalLength = rmsg.Frame.Length + 20;
            byte[] payload = new byte[totalLength];
            payload[0] = 0x45; //Version 4, 5 words (5*4=20 bytes)
            payload[1] = 0x00; //Differential services
            payload[2] = (byte)(totalLength >> 8); //Total size
            payload[3] = (byte)(totalLength);
            payload[4] = (byte)(sequence >> 8); //Identification, should be unique number
            payload[5] = (byte)(sequence);
            payload[6] = 0x40; //Don't fragment
            payload[7] = 0x00;
            payload[8] = 0x80; //TTL
            payload[9] = (byte)(rmsg.MessageType); //Undefined protocol. Used together with datagrams
            payload[10] = 0x00; //Header checksum (disabled)
            payload[11] = 0x00;
            payload[12] = 192; //Source
            payload[13] = 168;
            payload[14] = 0;
            payload[15] = 1;
            payload[16] = (byte)(rmsg.Id >> 24); //Destination
            payload[17] = (byte)(rmsg.Id >> 16);
            payload[18] = (byte)(rmsg.Id >> 8);
            payload[19] = (byte)(rmsg.Id);
            //Data
            Buffer.BlockCopy(rmsg.Frame, 0, payload, 20, rmsg.Frame.Length);
            return payload;
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

            if(Client != null)
            {
                if(Client.Connected)
                {
                    Client.Close();
                }
            }
        }
    }
}
