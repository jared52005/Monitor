using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace WTM.J2534
{
    internal class Program
    {
        static void Main(string[] args)
        {
            J2534_CanIf can = new J2534_CanIf("op20pt32.dll");
            Console.ReadKey();
        }
    }
}
