using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace WTM
{
    public interface ICanIf : IDisposable
    {
        event EventHandler<CanMessage> OnReceiveCanFrame;
    }
}
