using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using WTM.Shared;

namespace WTM.KLine
{
    internal class Program
    {
        static int Main(string[] args)
        {
            var pargs = Arguments.Parse(args);
            if (pargs != null)
            {
                if (!pargs.ContainsKey(ArgumentTypes.ComPort))
                {
                    Console.WriteLine("Missing COM port. Aborting.");
                    Console.WriteLine("Usage: -c COM1 -b 10400.");
                }
                else if (!pargs.ContainsKey(ArgumentTypes.Baudrate))
                {
                    Console.WriteLine("Missing Baudrate. Aborting.");
                    Console.WriteLine("Usage: -c COM1 -b 10400.");
                }
                else
                {
                    Passive_Kline_Manager pkm = new Passive_Kline_Manager();
                    pkm.Start(pargs[ArgumentTypes.ComPort] as string, (int)pargs[ArgumentTypes.Baudrate], true);
                    WaitEsc();
                    pkm.Dispose();
                    return 0;
                }
            }
            return -1;
        }

        static void WaitEsc()
        {
            ConsoleKeyInfo key;
            do
            {
                key = Console.ReadKey();
            } while (key.Key != ConsoleKey.Escape);
        }
    }
}
