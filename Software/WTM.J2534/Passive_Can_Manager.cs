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
    internal class Passive_Can_Manager : A_Passive_Can_Manager
    {
        public void Start(string dllName, int baudrate, string pathCanIds)
        {
            ICanIf can = new J2534_CanIf(dllName, baudrate);
            Start(can, pathCanIds);
        }
    }
}
