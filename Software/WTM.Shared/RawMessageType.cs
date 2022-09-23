using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace WTM
{
    /// <summary>
    /// Type which will select correct Wireshark dissectors
    /// </summary>
    public enum RawMessageType : byte
    {
        Raw_ISO14230 = 0x91,
        Raw_KW1281 = 0x92,
        Raw_VWTP20 = 0x93,
        Raw_ISO15765 = 0x94,
    }
}
