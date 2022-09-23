using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace WTM.KLine
{
    internal class Program
    {
        static void Main(string[] args)
        {
            Passive_Kline_Manager pkm = new Passive_Kline_Manager();
            pkm.Start("COM9", 10400);
            Console.Read();
        }
    }
}
