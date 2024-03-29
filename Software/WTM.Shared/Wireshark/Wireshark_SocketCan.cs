﻿using System;
using System.Collections.Generic;
using System.Linq;
using System.Net.Sockets;
using System.Net;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using System.IO;

namespace WTM.Wireshark
{
    /// <summary>
    /// Send IP RAW data into Wireshark
    /// </summary>
    public class Wireshark_SocketCan : IDisposable
    {
        readonly Object _syncObject = new Object();

        const int _port = 19001;
        Thread _listenThread;
        Queue<CanMessage> _qRaw;
        bool _cancelThread;
        TcpListener _server;

        public TcpClient Client { get; private set; }

        public Wireshark_SocketCan()
        {
            _qRaw = new Queue<CanMessage>();
            _listenThread = new Thread(Listen);
            _listenThread.Start();
        }

        public void Add(CanMessage msg)
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
                0xFF, 0xFF, 0x00, 0x00, 0xE3, 0x00, 0x00, 0x00
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
                    CanMessage cmsg;
                    lock (_syncObject)
                    {
                        cmsg = _qRaw.Dequeue();
                    }
                    byte[] header = PreapreHeader(cmsg);
                    stream.Write(header, 0, header.Length);

                    byte[] payload = PreparePayload(cmsg);
                    stream.Write(payload, 0, payload.Length);
                }
                else
                {
                    Thread.Sleep(20);
                }
            }
        }

        private byte[] PreapreHeader(CanMessage rmsg)
        {
            /* 00-00-00-00 00-00-00-00-10-00-00-00-10-00-00-00
             * Where:
             * 00-00-00-00 = Time Stamp seconds.
             * 00-00-00-00 = Time Stamp micro seconds
             * 10-00-00-00 = Size of packet saved in a file = 16 bytes of socket CAN data
             * 10-00-00-00 = Actual size of packet = 16 bytes of socket CAN data
             */

            byte[] header = new byte[16];

            uint timestamp_seconds = (uint)(rmsg.Timestamp / 1000); //Only second part
            uint timestamp_microseconds = (uint)(rmsg.Timestamp % 1000); //Only remainder from seconds
            timestamp_microseconds = timestamp_microseconds * 1000; //Convert milisecond to microseconds

            WriteLE(header, timestamp_seconds, 0);
            WriteLE(header, timestamp_microseconds, 4);
            WriteLE(header, 16, 8);
            WriteLE(header, 16, 12);

            return header;
        }

        private byte[] PreparePayload(CanMessage rmsg)
        {
            byte[] payload = new byte[16];

            payload[0] = (byte)(rmsg.Id >> 24);
            payload[1] = (byte)(rmsg.Id >> 16);
            payload[2] = (byte)(rmsg.Id >> 8);
            payload[3] = (byte)rmsg.Id;
            if(rmsg.Id > 0x7FF)
            {
                //Flag extended CAN messages
                payload[0] |= 0x80;
            }
            //DLC
            payload[4] = (byte)rmsg.Dlc;
            //Data
            Buffer.BlockCopy(rmsg.Data, 0, payload, 8, rmsg.Dlc);

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
