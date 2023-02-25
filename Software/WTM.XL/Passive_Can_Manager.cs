using System;
using System.Collections.Generic;
using System.IO.Ports;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using WTM.Protocols;
using WTM.Wireshark;

namespace WTM.XL
{
    internal class Passive_Can_Manager : A_Passive_Can_Manager
    {
        public void Start()
        {
            Start(new XL_CanIf());
        }
    }
}
