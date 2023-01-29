using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.IO.Ports;
using WTM.Wireshark;
using WTM.Protocols;

namespace WTM.KLine
{
    internal class Passive_Kline_Manager : IDisposable
    {
        bool _verbose;
        SerialPort _sp;
        Passive_Kline _pk;
        Wireshark_Raw _raw;

        int _defaultBaudrate;

        public void Dispose()
        {
            _sp.Close();
            _sp.Dispose();
            _pk.Dispose();
            _raw.Dispose();
        }

        public void Start(string comPort, int baudrate, bool verbose = false)
        {
            _defaultBaudrate = baudrate;
            _verbose = verbose;
            _raw = new Wireshark_Raw();
            _pk = new Passive_Kline();
            _pk.OnRawFrame += _pk_OnRawFrame;
            _pk.OnDefault += OnDefault;
            //Create VCP
            _sp = new SerialPort(comPort, baudrate);
            _sp.Open();
            //Route bytes from VCP to Passive KLINE
            _sp.DataReceived += _sp_DataReceived; 
            Console.WriteLine($"Ready @ {comPort}:{baudrate}");
            //Route passive KLINE to Wireshark
            //[Optional] Change baudrate when device finds requests to do so.
        }

        private void OnDefault(object sender, EventArgs e)
        {
            if(_sp.BaudRate != _defaultBaudrate)
            {
                Console.WriteLine($"Setting baudrate back to {_defaultBaudrate}");
                _sp.BaudRate = _defaultBaudrate;
            }
        }

        private void _pk_OnRawFrame(object sender, RawMessage e)
        {
            if (_verbose)
            {
                Console.WriteLine();
            }
            Console.WriteLine($"{e.MessageType} @ {e.Timestamp}ms [{BitConverter.ToString(e.Frame)}]");
            _raw.Add(e);
            ParseStartDiagnosticSession(e);
        }

        private void _sp_DataReceived(object sender, SerialDataReceivedEventArgs e)
        {
            while (_sp.BytesToRead != 0)
            {
                byte c = (byte)_sp.ReadByte();
                if (_verbose)
                {
                    Console.Write(" {0:X2}", c);
                }
                _pk.Passive_Kline_Parse(c);
            }
        }

        private void ParseStartDiagnosticSession(RawMessage e)
        {
            if(e.Frame.Length == 0)
            {
                return;
            }
            int length;
            int datagramPosition = 1;
            int fmt = e.Frame[0];

            //Keybytes
            if(fmt == 0x55)
            {
                return;
            }

            //Parse physical addressing (if included)
            int addressing = (fmt & 0xC0) >> 6;
            if (addressing == 0x02)
            {
                datagramPosition += 2;
            }

            //Parse length in byte
            int fmt_len = fmt & 0x3F;
            if (fmt_len != 0)
            {
                length = fmt_len;
            }
            else
            {
                length = e.Frame[datagramPosition];
                datagramPosition++;
            }

            if(datagramPosition >= e.Frame.Length)
            {
                return;
            }

            //Console.WriteLine($"Datagram: {BitConverter.ToString(e.Frame, datagramPosition, length)}");
            if (e.Frame[datagramPosition] == 0x50 && length == 3)
            {
                uint baudrate = CalculateBaudRateFromByte(e.Frame[datagramPosition + 2]);
                if (_sp.BaudRate != baudrate)
                {
                    Console.WriteLine($"Found new baudrate: {baudrate}");
                    _sp.BaudRate = (int)baudrate;
                }
            }
        }

        private uint CalculateBaudRateFromByte(byte baudRateByte)
        {
            byte xUpper = (byte)(baudRateByte >> 0x5);
            xUpper &= 0x7;

            byte yLower = (byte)(baudRateByte & 0x1F);

            byte xPow = (byte)(0x1 << xUpper);

            var baudRate = (uint)((xPow * (yLower + 32) * 6400) / 32);

            return baudRate;
        }
    }
}
