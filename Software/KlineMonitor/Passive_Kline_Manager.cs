using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.IO.Ports;

namespace KlineMonitor
{
    internal class Passive_Kline_Manager : IDisposable
    {
        SerialPort _sp;
        Passive_Kline _pk;
        Wireshark_Raw _raw;

        public void Dispose()
        {
            _sp.Close();
            _sp.Dispose();
            _pk.Dispose();
        }

        public void Start(string comPort, int baudrate)
        {
            _raw = new Wireshark_Raw();
            _pk = new Passive_Kline();
            _pk.OnRawFrame += _pk_OnRawFrame;
            //Create VCP
            _sp = new SerialPort(comPort, baudrate);
            _sp.Open();
            //Route bytes from VCP to Passive KLINE
            _sp.DataReceived += _sp_DataReceived; 
            Console.WriteLine($"Ready @ {comPort}:{baudrate}");
            //Route passive KLINE to Wireshark
            //[Optional] Change baudrate when device finds requests to do so.
        }

        private void _pk_OnRawFrame(object sender, RawMessage e)
        {
            Console.WriteLine($"{e.MessageType} @ {e.Timestamp}ms [{BitConverter.ToString(e.Frame)}]");
            _raw.Add(e);
        }

        private void _sp_DataReceived(object sender, SerialDataReceivedEventArgs e)
        {
            while (_sp.BytesToRead != 0)
            {
                byte c = (byte)_sp.ReadByte();
                //Console.Write(" {0:X2}", c);
                _pk.Passive_Kline_Parse(c);
            }
        }
    }
}
