using System;
using System.Collections.Generic;
using System.IO.Ports;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using WTM.Protocols;
using WTM.Wireshark;

namespace WTM.Pcan
{
    internal class Passive_Can_Manager : A_Passive_Can_Manager
    {
        public void Start(int baudarate, string pathCanIds)
        {
            ICanIf can = new Pcan_CanIf(baudarate);
            Start(can, pathCanIds);
        }
    }
}
