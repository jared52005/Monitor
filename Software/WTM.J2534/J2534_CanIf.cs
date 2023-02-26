using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Runtime.Remoting.Channels;
using System.Security.Cryptography;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using SAE.J2534;


namespace WTM.J2534
{
    internal class J2534_CanIf : ICanIf
    {
        bool m_endThread;
        Thread _rxThread;

        API m_j2534Api;
        Device m_j2534Interface;
        Channel m_j2534Channel;

        public event EventHandler<CanMessage> OnReceiveCanFrame;

        public J2534_CanIf(string dllName, int baudrate)
        {
            //Get full path to DLL
            var q = APIFactory.GetAPIinfo();
            string fileName = string.Empty;
            foreach(var info in q)
            {
                if (string.IsNullOrEmpty(dllName))
                {
                    Console.WriteLine($" - Available {info.Name} DllName: {Path.GetFileName(info.Filename)}");
                }
                else if (info.Filename.Contains(dllName))
                {
                    fileName = info.Filename;
                }
            }

            if(string.IsNullOrEmpty(fileName) || string.IsNullOrEmpty(dllName))
            {
                Console.WriteLine($"J2534 DLL {dllName} not found. See possible candidates above");
                return;
            }

            m_j2534Api = APIFactory.GetAPI(fileName);
            m_j2534Interface = m_j2534Api.GetDevice();
            if (m_j2534Interface == null)
            {
                Console.WriteLine("Unable to open device");
                return;
            }

            m_j2534Channel = m_j2534Interface.GetChannel(Protocol.CAN, (Baud)baudrate, ConnectFlag.NONE);
            MessageFilter msgPattern = new MessageFilter(UserFilterType.PASSALL, null);
            int filterId = m_j2534Channel.StartMsgFilter(msgPattern);

            _rxThread = new Thread(RxThread_CanMessages);
            _rxThread.Start();

            m_j2534Channel.ClearRxBuffer();
        }

        public void Dispose()
        {
            m_endThread = true;
        }

        private void RxThread_CanMessages()
        {
            Console.WriteLine("readMessages started");
            m_endThread = false;
            m_j2534Channel.DefaultRxTimeout = 100;
            while (!m_endThread)
            {
                //Receive message
                GetMessageResults Response = m_j2534Channel.GetMessage();
                if (Response == null)
                {
                    continue;
                }

                if (Response == null)
                {
                    continue;
                }
                foreach (Message m in Response.Messages)
                {
                    int id = 0;
                    byte[] data = m.Data.Skip(4).ToArray();
                    id |= m.Data[0] << 24;
                    id |= m.Data[1] << 16;
                    id |= m.Data[2] << 8;
                    id |= m.Data[3];

                    CanMessage msg = new CanMessage(data, id);
                    if (m.Timestamp == 0)
                    {
                        DateTime dtnow = DateTime.Now;
                        msg.Timestamp += dtnow.Hour * 3600 * 1000;
                        msg.Timestamp += dtnow.Minute * 60 * 1000;
                        msg.Timestamp += dtnow.Second * 1000;
                        msg.Timestamp += dtnow.Millisecond;
                    }
                    else
                    {
                        msg.Timestamp = m.Timestamp; //Is it in miliseconds?
                    }
                    OnReceiveCanFrame?.Invoke(this, msg);
                }
            }
        }
    }
}
