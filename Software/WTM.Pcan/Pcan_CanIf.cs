using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.InteropServices;
using System.Runtime.Remoting.Messaging;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
//  PCANBasic.cs
//
//  ~~~~~~~~~~~~
//
//  PCAN-Basic API
//
//  ~~~~~~~~~~~~
//
//  ------------------------------------------------------------------
//  Author : Keneth Wagner
//	Last change: 24.04.2015 Wagner
//
//  Language: C# 1.0
//  ------------------------------------------------------------------
//
//  Copyright (C) 1999-2015  PEAK-System Technik GmbH, Darmstadt
//  more Info at http://www.peak-system.com 
//

namespace WTM.Pcan
{
    using TPCANHandle = System.UInt16;

    class Pcan_CanIf : ICanIf
    {
        bool _enabled;

        public event EventHandler<CanMessage> OnReceiveCanFrame;
        /// <summary>
        /// Saves the handle of a PCAN hardware
        /// </summary>
        private TPCANHandle m_PcanHandle;

        /// <summary>
        /// Saves the type of a non-plug-and-play hardware
        /// </summary>
        private TPCANType m_HwType = TPCANType.PCAN_TYPE_ISA;

        /// <summary>
        /// Receive-Event
        /// </summary>
        private System.Threading.AutoResetEvent m_ReceiveEvent;

        /// <summary>
        /// Thread for message reading (using events)
        /// </summary>
        private System.Threading.Thread m_ReadThread;

        /// <summary>
        /// Handles of the current available PCAN-Hardware
        /// </summary>
        private TPCANHandle[] m_HandlesArray;

        public int Baudrate { get; }

        /// <summary>
        /// Initialize PeakCAN Driver
        /// </summary>
        public Pcan_CanIf(int baudrate)
        {
            // Creates the event used for signalize incomming messages 
            m_ReceiveEvent = new AutoResetEvent(false);

            //Could be loaded dynamically over Foreach and enum.type
            // Creates an array with all possible PCAN-Channels
            m_HandlesArray = new TPCANHandle[]
            {
                     PCANBasic.PCAN_USBBUS1,
                     PCANBasic.PCAN_USBBUS2,
                     PCANBasic.PCAN_USBBUS3,
                     PCANBasic.PCAN_USBBUS4,
                     PCANBasic.PCAN_USBBUS5,
                     PCANBasic.PCAN_USBBUS6,
                     PCANBasic.PCAN_USBBUS7,
                     PCANBasic.PCAN_USBBUS8,
                     PCANBasic.PCAN_USBBUS9,
                     PCANBasic.PCAN_USBBUS10,
                     PCANBasic.PCAN_USBBUS11,
                     PCANBasic.PCAN_USBBUS12,
                     PCANBasic.PCAN_USBBUS13,
                     PCANBasic.PCAN_USBBUS14,
                     PCANBasic.PCAN_USBBUS15,
                     PCANBasic.PCAN_USBBUS16,
            };

            //Enable PeakCAN device first
            if (!InitDevice(baudrate))
            {
                throw new Exception("Cannot continue, unable to enable PeakCAN device");
            }
            _enabled = true;
            Baudrate = baudrate;

            //Start thread for reading of data
            ThreadStart threadDelegate = new ThreadStart(this.CANReadThreadFunc);
            m_ReadThread = new Thread(threadDelegate);
            m_ReadThread.IsBackground = true;
            m_ReadThread.Start();
        }

        /// <summary>
        /// Help Function used to get an error as text
        /// </summary>
        /// <param name="error">Error code to be translated</param>
        /// <returns>A text with the translated error</returns>
        private string GetFormatedError(TPCANStatus error)
        {
            StringBuilder strTemp;

            // Creates a buffer big enough for a error-text
            //
            strTemp = new StringBuilder(256);
            // Gets the text using the GetErrorText API function
            // If the function success, the translated error is returned. If it fails,
            // a text describing the current error is returned.
            //
            if (PCANBasic.GetErrorText(error, 0, strTemp) != TPCANStatus.PCAN_ERROR_OK)
                return string.Format("An error occurred. Error-code's text ({0:X}) couldn't be retrieved", error);
            else
                return strTemp.ToString();
        }

        /// <summary>
        /// Returns list of active devices on PeakBasic API
        /// </summary>
        /// <param name="availableHandles">List of devices as handles. Devices are limited only on PlugAndPlay</param>
        /// <returns>True, if successful</returns>
        /// <remarks>
        /// There is bug directly in the PeakCAN API driver for x64 Windows which neads this workaround
        /// This bug is seen as a BadFormatException of library
        /// 1 - If you are using Win64 and creating Win64 application, then you need to have 64bit driver in System32 and SysWOW64
        /// 2 - If you are using Win64 and creating Win64 application, then you need to have 32bit driver in both System32 abd SysWOW64
        /// Otherwise you need to have correct driver directly in the root application which is not comfortable.
        /// </remarks>
        private bool GetDeviceState(out List<TPCANHandle> availableHandles)
        {
            //Finds PeakCAN and returns information about it
            UInt32 iBuffer;
            TPCANStatus stsResult = TPCANStatus.PCAN_ERROR_UNKNOWN;
            availableHandles = new List<ushort>();

            // Detect Plug&Play PeakCAN hardware
            try
            {
                for (int i = 0; i < m_HandlesArray.Length; i++)
                {
                    if (m_HandlesArray[i] >= PCANBasic.PCAN_DNGBUS1)
                    {
                        // Checks for a Plug&Play Handle and, according with the return value, includes it
                        // into the list of available hardware channels.
                        stsResult = PCANBasic.GetValue(m_HandlesArray[i], TPCANParameter.PCAN_CHANNEL_CONDITION, out iBuffer, sizeof(UInt32));
                        if ((stsResult == TPCANStatus.PCAN_ERROR_OK) && ((iBuffer & PCANBasic.PCAN_CHANNEL_AVAILABLE) == PCANBasic.PCAN_CHANNEL_AVAILABLE))
                        {
                            //Check if CAN channel is FD capable (Not used)
                            //stsResult = PCANBasic.GetValue(m_HandlesArray[i], TPCANParameter.PCAN_CHANNEL_FEATURES, out iBuffer, sizeof(UInt32));
                            //isFD = (stsResult == TPCANStatus.PCAN_ERROR_OK) && ((iBuffer & PCANBasic.FEATURE_FD_CAPABLE) == PCANBasic.FEATURE_FD_CAPABLE);

                            //Add found handler into list of available handlers 
                            availableHandles.Add(m_HandlesArray[i]);
                        }
                    }
                }
            }
            catch (DllNotFoundException)
            {
                Console.WriteLine("Unable to find the library: PCANBasic.dll !");
                return false;
            }
            catch (Exception e)
            {
                Console.WriteLine(e.Message);
                return false;
            }

            //Check if error did not occured during checking for available P&P channels
            if (stsResult != TPCANStatus.PCAN_ERROR_OK)
            {
                Console.WriteLine(GetFormatedError(stsResult));
                return false;
            }
            return true;
        }

        private bool InitDevice(int baudrate)
        {
            //Translate baudrate into PeakCAN enum
            TPCANBaudrate m_Baudrate;
            switch (baudrate)
            {
                case 1000000:
                    m_Baudrate = TPCANBaudrate.PCAN_BAUD_1M;
                    break;
                case 800000:
                    m_Baudrate = TPCANBaudrate.PCAN_BAUD_800K;
                    break;
                case 500000:
                    m_Baudrate = TPCANBaudrate.PCAN_BAUD_500K;
                    break;
                case 250000:
                    m_Baudrate = TPCANBaudrate.PCAN_BAUD_250K;
                    break;
                case 125000:
                    m_Baudrate = TPCANBaudrate.PCAN_BAUD_125K;
                    break;
                case 100000:
                    m_Baudrate = TPCANBaudrate.PCAN_BAUD_100K;
                    break;
                case 95238:
                case 95000:
                    m_Baudrate = TPCANBaudrate.PCAN_BAUD_95K;
                    break;
                case 83333:
                case 83000:
                    m_Baudrate = TPCANBaudrate.PCAN_BAUD_83K;
                    break;
                case 50000:
                    m_Baudrate = TPCANBaudrate.PCAN_BAUD_50K;
                    break;
                case 47000:
                case 47619:
                    m_Baudrate = TPCANBaudrate.PCAN_BAUD_47K;
                    break;
                default:
                    throw new NotImplementedException(string.Format("Baudare {0} is not supported by driver or device", baudrate));
            }

            //Find avaiable devices and init first found
            List<TPCANHandle> AvailableDevicesHanldes;
            if (GetDeviceState(out AvailableDevicesHanldes))
            {
                if (AvailableDevicesHanldes.Count != 0)
                {
                    //Handles are found, try to connect to first one
                    if (InitDevice(AvailableDevicesHanldes[0], m_Baudrate))
                    {
                        return true;
                    }
                }
            }

            return false;
        }

        /// <summary>
        /// Release PeakCAN from system
        /// </summary>
        public void Dispose()
        {
            _enabled = false;
            // Releases a current connected PCAN-Basic channel
            PCANBasic.Uninitialize(m_PcanHandle);
            if (m_ReadThread != null)
            {
                m_ReadThread.Abort();
                m_ReadThread.Join();
                m_ReadThread = null;
            }
        }

        /// <summary>
        /// Inits device with provided handle
        /// </summary>
        /// <param name="deviceHandle">Handle to Plug and Play device</param>
        /// <param name="m_Baudrate">Chosen specific CAN baudrate</param>
        /// <returns>True, if init was successful</returns>
        private bool InitDevice(TPCANHandle deviceHandle, TPCANBaudrate m_Baudrate)
        {
            //Init PeakCAN device, if it is successfully found
            TPCANStatus stsResult;

            // Connects a selected PCAN-Basic channel
            stsResult = PCANBasic.Initialize(
                         deviceHandle,
                         m_Baudrate,
                         m_HwType,
                         100, //Port (Used only for Not P&P devices)
                         3);  //Interrupt (Used only for Not P&P devices)

            if (stsResult != TPCANStatus.PCAN_ERROR_OK)
            {
                if (stsResult != TPCANStatus.PCAN_ERROR_CAUTION)
                {
                    Console.WriteLine(GetFormatedError(stsResult));
                }
                else
                {
                    Console.WriteLine("The bitrate being used is different than the given one");
                    stsResult = TPCANStatus.PCAN_ERROR_OK;
                }
            }
            else
            {
                m_PcanHandle = deviceHandle;
            }

            // Sets the connection status of the main-form
            //
            return stsResult == TPCANStatus.PCAN_ERROR_OK;
        }

        private void CANReadThreadFunc()
        {
            UInt32 iBuffer;
            TPCANStatus stsResult;

            iBuffer = Convert.ToUInt32(m_ReceiveEvent.SafeWaitHandle.DangerousGetHandle().ToInt32());
            // Sets the handle of the Receive-Event.
            //
            stsResult = PCANBasic.SetValue(m_PcanHandle, TPCANParameter.PCAN_RECEIVE_EVENT, ref iBuffer, sizeof(UInt32));

            if (stsResult != TPCANStatus.PCAN_ERROR_OK)
            {
                Console.WriteLine(GetFormatedError(stsResult));
                return;
            }

            // While this mode is selected
            while (_enabled)
            {
                // Waiting for Receive-Event
                // 
                if (m_ReceiveEvent.WaitOne(50))
                    // Process Receive-Event using .NET Invoke function
                    // in order to interact with Winforms UI (calling the 
                    // function ReadMessages)
                    // 
                    ReadMessages();
            }
        }

        private void ReadMessages()
        {
            TPCANStatus stsResult;

            // We read at least one time the queue looking for messages.
            // If a message is found, we look again trying to find more.
            // If the queue is empty or an error occurr, we get out from
            // the dowhile statement.
            //			
            do
            {
                stsResult = ReadMessage();
                if (stsResult == TPCANStatus.PCAN_ERROR_ILLOPERATION)
                    break;

            } while (_enabled && (!Convert.ToBoolean(stsResult & TPCANStatus.PCAN_ERROR_QRCVEMPTY)));
        }

        private TPCANStatus ReadMessage()
        {
            TPCANMsg CANMsg;
            TPCANTimestamp CANTimeStamp;
            TPCANStatus stsResult;

            // We execute the "Read" function of the PCANBasic
            stsResult = PCANBasic.Read(m_PcanHandle, out CANMsg, out CANTimeStamp);
            if (stsResult == TPCANStatus.PCAN_ERROR_OK)
            {
                // We process the received message
                ProcessMessage(CANMsg, CANTimeStamp);
            }
            return stsResult;
        }

        private void ProcessMessage(TPCANMsg theMsg, TPCANTimestamp itsTimeStamp)
        {
            byte[] data = new byte[theMsg.LEN];
            if (data.Length > 8 || data.Length == 0)
            {
                return;
            }

            Buffer.BlockCopy(theMsg.DATA, 0, data, 0, data.Length);
            CanMessage cmsg = new CanMessage(data, (int)theMsg.ID);
            cmsg.Timestamp = itsTimeStamp.millis;
            OnReceiveCanFrame?.Invoke(this, cmsg);
        }
    }
}

