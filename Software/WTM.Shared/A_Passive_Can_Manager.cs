using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using WTM.Filter;
using WTM.Protocols;
using WTM.Wireshark;

namespace WTM
{
    public abstract class A_Passive_Can_Manager : IDisposable
    {
        ICanIf _can;
        CanIds _canIds;
        Passive_ISO15765 _pisotp;
        Passive_VWTP20 _pvwtp20;
        Wireshark_SocketCan _ws_can;
        Wireshark_Raw _ws_raw;

        public void Dispose()
        {
            _can.Dispose();
            _ws_can.Dispose();
            _ws_raw.Dispose();
            _canIds.Dispose();
        }

        public void Start(ICanIf can, string canidsPath = null)
        {
            _canIds = new CanIds(canidsPath);

            _ws_can = new Wireshark_SocketCan();
            _ws_raw = new Wireshark_Raw();
            _pisotp = new Passive_ISO15765();
            _pisotp.OnRawFrame += _canPdu_OnRawFrame;
            _pvwtp20 = new Passive_VWTP20();
            _pvwtp20.OnRawFrame += _canPdu_OnRawFrame;
            //Create Create CAN input
            _can = can;
            //Route CAN messages on passive protocols
            _can.OnReceiveCanFrame += _can_OnReceiveCanFrame;
            Console.WriteLine($"Ready");
        }


        private void _canPdu_OnRawFrame(object sender, RawMessage e)
        {
            Console.WriteLine($"{e.MessageType} @ {e.Timestamp}ms [{BitConverter.ToString(e.Frame)}]");
            _ws_raw.Add(e);
        }

        private void _can_OnReceiveCanFrame(object sender, CanMessage e)
        {
            if(_canIds.Ignore(e))
            {
                return;
            }

            //Console.WriteLine(e);
            _ws_can.Add(e);

            if (_canIds.IsIso15765(e))
            {
                if (_pisotp.Passive_Iso15765_Parse(e))
                {
                    return;
                }
            }
            if (_pvwtp20.Passive_Vwtp20_Parse(e))
            {
                return;
            }
        }
    }
}
