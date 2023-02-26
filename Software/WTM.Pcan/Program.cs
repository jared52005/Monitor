using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using WTM.Shared;

namespace WTM.Pcan
{
    internal class Program
    {
        static int Main(string[] args)
        {
            var pargs = Arguments.Parse(args);
            if (pargs != null)
            {
                if (!pargs.ContainsKey(ArgumentTypes.Baudrate))
                {
                    Console.WriteLine("Missing Baudrate. Aborting.");
                    Console.WriteLine("Usage: -b 500000 [-i canId.xml]");
                }
                else
                {
                    string pathCanIds = string.Empty;
                    if(pargs.ContainsKey( ArgumentTypes.CanIdsFile))
                    {
                        pathCanIds = pargs[ArgumentTypes.CanIdsFile] as string;
                    }
                    Passive_Can_Manager pcm = new Passive_Can_Manager();
                    pcm.Start((int)pargs[ArgumentTypes.Baudrate], pathCanIds);
                    WaitEsc();
                    pcm.Dispose();
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
