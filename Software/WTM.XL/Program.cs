using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace WTM.XL
{
    internal class Program
    {
        static int Main(string[] args)
        {
            Passive_Can_Manager pcm = new Passive_Can_Manager();
            pcm.Start();
            WaitEsc();
            pcm.Dispose();
            return 0;
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
