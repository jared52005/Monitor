using System;
using System.Collections.Generic;
using System.IO.Ports;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using WTM.Protocols;
using WTM.Wireshark;

namespace WTM.J2534
{
    internal class Passive_Can_Manager : IDisposable
    {
        J2534_CanIf _can;
        Passive_ISO15765 _pisotp;
        Wireshark_SocketCan _ws_can;
        Wireshark_Raw _ws_raw;

        public void Dispose()
        {
            _can.Dispose();
            _ws_can.Dispose();
            _ws_raw.Dispose();
        }

        public void Start(string dllName)
        {
            _ws_can = new Wireshark_SocketCan();
            _ws_raw = new Wireshark_Raw();
            _pisotp = new Passive_ISO15765();
            _pisotp.OnRawFrame += _canPdu_OnRawFrame;
            //Create Create CAN input
            _can = new J2534_CanIf(dllName);
            //Route CAN messages on passive protocols
            _can.OnReceiveCanFrame += _can_OnReceiveCanFrame;
            Console.WriteLine($"Ready @ {dllName}");
        }


        private void _canPdu_OnRawFrame(object sender, RawMessage e)
        {
            Console.WriteLine($"{e.MessageType} @ {e.Timestamp}ms [{BitConverter.ToString(e.Frame)}]");
            _ws_raw.Add(e);
        }

        private void _can_OnReceiveCanFrame(object sender, CanMessage e)
        {
            //Console.WriteLine(e);
            _ws_can.Add(e);

            if(_pisotp.Passive_Iso15765_Parse(e))
            {
                return;
            }
        }
    }
}
