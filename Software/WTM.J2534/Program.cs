using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using WTM.Shared;

namespace WTM.J2534
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
                    Console.WriteLine("Usage: -b 500000 [-f canId.xml] [-dll op20pt32.dll]");
                }
                else
                {
                    string pathCanIds = string.Empty;
                    string dll = string.Empty;
                    if (pargs.ContainsKey(ArgumentTypes.CanIdsFile))
                    {
                        pathCanIds = pargs[ArgumentTypes.CanIdsFile] as string;
                    }
                    if (pargs.ContainsKey(ArgumentTypes.J2534Dll))
                    {
                        dll = pargs[ArgumentTypes.J2534Dll] as string;
                    }
                    Passive_Can_Manager pcm = new Passive_Can_Manager();
                    pcm.Start(dll, (int)pargs[ArgumentTypes.Baudrate], pathCanIds);
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
