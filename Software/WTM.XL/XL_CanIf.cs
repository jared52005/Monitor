using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using vxlapi_NET;

namespace WTM.XL
{
    internal class XL_CanIf
    {
        // Driver access through XLDriver (wrapper)
        private XLDriver _canDriver = new XLDriver();
        private String _appName = "wstrafficmon";

        // Driver configuration
        private XLClass.xl_driver_config driverConfig = new XLClass.xl_driver_config();

        // Variables required by XLDriver
        private XLDefine.XL_HardwareType _hwType = XLDefine.XL_HardwareType.XL_HWTYPE_VIRTUAL;
        private uint _hwIndex = 0;
        private uint _hwChannel = 0;
        private int _portHandle = -1;
        private int _eventHandle = -1;
        private UInt64 _accessMask = 0;
        private UInt64 _permissionMask = 0;
        private UInt64 _txMask = 0;

        // RX thread
        private Thread _rxThread;
        private bool _killRxThread;
        AutoResetEvent _mutexWaitOnInit;

        public event EventHandler<CanMessage> OnReceiveCanFrame;
        public event EventHandler<byte[]> OnReceivePduFrame;

        public XLDefine.XL_Status Status { get; set; }

        public XL_CanIf()
        {
            Status = XLDefine.XL_Status.XL_ERROR; //Default setup
            _mutexWaitOnInit = new AutoResetEvent(false);

            _rxThread = new Thread(RXThread);
            if (_rxThread.ThreadState != ThreadState.Running)
            {
                Console.WriteLine($"Thread state is {_rxThread.ThreadState} Enabling thread and waiting on result");
                _mutexWaitOnInit.Reset();
                _rxThread.Start();
                if (!_mutexWaitOnInit.WaitOne(1000))
                {
                    Console.WriteLine("Vector: Unable to init device and start receiving");
                    //throw new Exception();
                }
            }
        }

        /// <summary>
        /// This will only init device on the other end. It really depends how device
        /// in Vector config is setup (baudrate)
        /// </summary>
        /// <returns>True if inited successfully</returns>
        bool InitCANtransmitter()
        {
            // Open XL Driver
            Status = _canDriver.XL_OpenDriver();
            if (Status != XLDefine.XL_Status.XL_SUCCESS)
            {
                Console.WriteLine("XL_OpenDriver: " + Status);
                return false;
            }

            // Get XL Driver configuration
            Status = _canDriver.XL_GetDriverConfig(ref driverConfig);
            if (Status != XLDefine.XL_Status.XL_SUCCESS)
            {
                Console.WriteLine("XL_GetDriverConfig: " + Status);
                return false;
            }

            // If the application name cannot be found in VCANCONF...
            if ((_canDriver.XL_GetApplConfig(_appName, 0, ref _hwType, ref _hwIndex, ref _hwChannel, XLDefine.XL_BusTypes.XL_BUS_TYPE_CAN) != XLDefine.XL_Status.XL_SUCCESS))
            {
                //...create the item with two CAN channels
                _canDriver.XL_SetApplConfig(_appName, 0, XLDefine.XL_HardwareType.XL_HWTYPE_NONE, 0, 0, XLDefine.XL_BusTypes.XL_BUS_TYPE_CAN);
                PrintAssignError();
                return false; //Needs to be invoked again
            }

            else // else try to read channel assignments*/
            {
                // Read setting of CAN1
                _canDriver.XL_GetApplConfig(_appName, 0, ref _hwType, ref _hwIndex, ref _hwChannel, XLDefine.XL_BusTypes.XL_BUS_TYPE_CAN);

                // Notify user if no channel is assigned to this application 
                if (_hwType == XLDefine.XL_HardwareType.XL_HWTYPE_NONE)
                {
                    PrintAssignError();
                    return false;
                }

                _accessMask = _canDriver.XL_GetChannelMask(_hwType, (int)_hwIndex, (int)_hwChannel);
                _txMask = _accessMask; // this channel is used for Tx

                _permissionMask = _accessMask;

                // Open port 
                Status = _canDriver.XL_OpenPort(ref _portHandle, _appName, _accessMask, ref _permissionMask, 1024, XLDefine.XL_InterfaceVersion.XL_INTERFACE_VERSION, XLDefine.XL_BusTypes.XL_BUS_TYPE_CAN);
                if (Status != XLDefine.XL_Status.XL_SUCCESS)
                {
                    Console.WriteLine("XL_OpenPort: " + Status);
                    return false;
                }

                // Check port
                Status = _canDriver.XL_CanRequestChipState(_portHandle, _accessMask);
                if (Status != XLDefine.XL_Status.XL_SUCCESS)
                {
                    Console.WriteLine("XL_CanRequestChipState: " + Status);
                    return false;
                }

                // Activate channel
                Status = _canDriver.XL_ActivateChannel(_portHandle, _accessMask, XLDefine.XL_BusTypes.XL_BUS_TYPE_CAN, XLDefine.XL_AC_Flags.XL_ACTIVATE_NONE);
                if (Status != XLDefine.XL_Status.XL_SUCCESS)
                {
                    Console.WriteLine("XL_ActivateChannel: " + Status);
                    return false;
                }

                // Get RX event handle
                Status = _canDriver.XL_SetNotification(_portHandle, ref _eventHandle, 1);
                if (Status != XLDefine.XL_Status.XL_SUCCESS)
                {
                    Console.WriteLine("XL_SetNotification: " + Status);
                    return false;
                }

                // Reset time stamp clock
                _canDriver.XL_ResetClock(_portHandle);
                if (Status != XLDefine.XL_Status.XL_SUCCESS)
                {
                    Console.WriteLine("XL_ResetClock: " + Status);
                    return false;
                }
            }
            return XLDefine.XL_Status.XL_SUCCESS == Status;
        }

        public void Dispose()
        {
            _killRxThread = true;
            _rxThread = null;
        }

        /// <summary>
        /// Error message if channel assignment is not valid.
        /// </summary>
        private void PrintAssignError()
        {
            //Console.WriteLine("\nPlease check application settings of \"" + _appName + " CAN1/CAN2\" \nand assign it to an available hardware channel and restart application.");
            _canDriver.XL_PopupHwConfig();
        }

        /// <summary>
        /// RX thread waits for Vector interface events and displays filtered CAN messages.
        /// </summary>
        private void RXThread()
        {
            if (!InitCANtransmitter())
            {
                return;
            }
            else
            {
                //Provide validation that thread is ready and running
                _mutexWaitOnInit.Set();
            }

            _killRxThread = false;
            // Create new object containing received data 
            XLClass.xl_event receivedEvent = new XLClass.xl_event();

            // Result of XL Driver function calls
            XLDefine.XL_Status xlStatus = XLDefine.XL_Status.XL_SUCCESS;

            // Result values of WaitForSingleObject 
            XLDefine.WaitResults waitResult = new XLDefine.WaitResults();

            int handle = 0;
            _canDriver.XL_SetNotification(_portHandle, ref handle, 1);

            while (!_killRxThread)
            {
                // Wait for hardware events
                waitResult = _canDriver.XL_WaitForSingleObject(handle, 10);

                // If event occurred...
                if (waitResult != XLDefine.WaitResults.WAIT_TIMEOUT)
                {
                    // ...init xlStatus first
                    xlStatus = XLDefine.XL_Status.XL_SUCCESS;

                    // afterwards: while hw queue is not empty...
                    while (xlStatus != XLDefine.XL_Status.XL_ERR_QUEUE_IS_EMPTY)
                    {
                        // ...receive data from hardware.
                        xlStatus = _canDriver.XL_Receive(_portHandle, ref receivedEvent);

                        //  If receiving succeed....
                        if (xlStatus == XLDefine.XL_Status.XL_SUCCESS)
                        {
                            if ((receivedEvent.flags & XLDefine.XL_MessageFlags.XL_EVENT_FLAG_OVERRUN) != 0)
                            {
                                Console.WriteLine("XL_EVENT_FLAG_OVERRUN");
                            }

                            // ...and data is a Rx msg...
                            if (receivedEvent.tag == XLDefine.XL_EventTags.XL_RECEIVE_MSG)
                            {
                                if ((receivedEvent.tagData.can_Msg.flags & XLDefine.XL_MessageFlags.XL_CAN_MSG_FLAG_OVERRUN) != 0)
                                {
                                    Console.WriteLine("XL_CAN_MSG_FLAG_OVERRUN");
                                }

                                // ...check various flags
                                if ((receivedEvent.tagData.can_Msg.flags & XLDefine.XL_MessageFlags.XL_CAN_MSG_FLAG_ERROR_FRAME)
                                    == XLDefine.XL_MessageFlags.XL_CAN_MSG_FLAG_ERROR_FRAME)
                                {
                                    Console.WriteLine("ERROR FRAME");
                                }

                                else if ((receivedEvent.tagData.can_Msg.flags & XLDefine.XL_MessageFlags.XL_CAN_MSG_FLAG_REMOTE_FRAME)
                                    == XLDefine.XL_MessageFlags.XL_CAN_MSG_FLAG_REMOTE_FRAME)
                                {
                                    Console.WriteLine("REMOTE FRAME");
                                }

                                else
                                {                                    
                                    byte[] data = receivedEvent.tagData.can_Msg.data.Take(receivedEvent.tagData.can_Msg.dlc).ToArray();
                                    int id = (int)receivedEvent.tagData.can_Msg.id & 0x1FFFFFFF;

                                    CanMessage frame = new CanMessage(data, id);
                                    //Timestamp value is in nanoseconds, generated with 8us precision per XL Driver Library
                                    frame.Timestamp = (long)(receivedEvent.timeStamp / 1000);
                                    OnReceiveCanFrame?.Invoke(this, frame);
                                }
                            }
                        }

                        //Leave thread when requested
                        if (_killRxThread)
                        {
                            break;
                        }
                    }
                }
                // No event occurred
            }

            //Close communication with device
            Status = _canDriver.XL_ClosePort(_portHandle);
            if (Status != XLDefine.XL_Status.XL_SUCCESS)
            {
                //Log error
                Console.WriteLine("XL_ClosePort failed");
            }
            Status = _canDriver.XL_CloseDriver();
            if (Status != XLDefine.XL_Status.XL_SUCCESS)
            {
                //Log error
                Console.WriteLine("XL_CloseDriver failed");
            }
        }
    }
}

