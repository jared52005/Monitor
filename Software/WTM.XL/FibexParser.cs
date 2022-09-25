using System;
using System.Collections;
using System.Collections.Generic;
using System.Text;
using System.Xml;
using System.Xml.XPath;
using System.IO;
using vxlapi_NET;

namespace WTM.XL
{
    static class FibexParser
    {
        /// <summary>
        /// Loads the application settings from a FIBEX file and returns the values in a class
        /// according to the XL API .NET Wrapper
        ///
        ///   NOTE: - THIS EXAMPLE SUPPORTS FLEXRAY FIBEX VERSION 2.0.1 FILES.
        ///           Other Flexray Fibex versions may or may not work.
        ///           Fibex 3.x.x versions doesn't work!
        ///         - The used Fibex file must contain the definitions of one node only. If several nodes are present
        ///           in the file, the result will be undefined - in most cases the values of the last node in the
        ///           Fibex will be used.
        ///         - The used Fibex file must contain the definitions of one cluster only.
        ///
        /// </summary>
        //--------------------------------------------------------------------------------------------------------
        public static Exception GetConfig(string fileName, out XLClass.xl_fr_cluster_configuration frConfig, out FibexDescription desc)
        {
            frConfig = new XLClass.xl_fr_cluster_configuration();
            desc = new FibexDescription();
            XmlDocument doc = new XmlDocument();
            XmlNodeList nodeList;
            doc.Load(fileName);

            // Unused API parameters
            frConfig.busGuardianEnable = 0;
            frConfig.busGuardianTick = 0;
            frConfig.externalClockCorrectionMode = 0;
            frConfig.pKeySlotUsedForStartup = 0;
            frConfig.pKeySlotUsedForSync = 0;
            frConfig.gChannels = 0;
            frConfig.vExternOffsetControl = 0;
            frConfig.vExternRateControl = 0;

            try
            {
                // Read CLUSTER section of the config
                nodeList = doc.GetElementsByTagName("fx:SPEED");
                frConfig.baudrate = Convert.ToUInt32(nodeList[0].InnerText);

                nodeList = doc.GetElementsByTagName("flexray:COLD-START-ATTEMPTS");
                frConfig.gColdStartAttempts = Convert.ToUInt16(nodeList[0].InnerText);

                nodeList = doc.GetElementsByTagName("flexray:LISTEN-NOISE");
                frConfig.gListenNoise = Convert.ToUInt16(nodeList[0].InnerText);

                nodeList = doc.GetElementsByTagName("flexray:MACRO-PER-CYCLE");
                frConfig.gMacroPerCycle = Convert.ToUInt16(nodeList[0].InnerText);

                nodeList = doc.GetElementsByTagName("flexray:MAX-WITHOUT-CLOCK-CORRECTION-FATAL");
                frConfig.gMaxWithoutClockCorrectionFatal = Convert.ToUInt16(nodeList[0].InnerText);

                nodeList = doc.GetElementsByTagName("flexray:MAX-WITHOUT-CLOCK-CORRECTION-PASSIVE");
                frConfig.gMaxWithoutClockCorrectionPassive = Convert.ToUInt16(nodeList[0].InnerText);

                nodeList = doc.GetElementsByTagName("flexray:NETWORK-MANAGEMENT-VECTOR-LENGTH");
                frConfig.gNetworkManagementVectorLength = Convert.ToUInt16(nodeList[0].InnerText);

                nodeList = doc.GetElementsByTagName("flexray:NUMBER-OF-MINISLOTS");
                frConfig.gNumberOfMinislots = Convert.ToUInt16(nodeList[0].InnerText);

                nodeList = doc.GetElementsByTagName("flexray:NUMBER-OF-STATIC-SLOTS");
                frConfig.gNumberOfStaticSlots = Convert.ToUInt16(nodeList[0].InnerText);

                nodeList = doc.GetElementsByTagName("flexray:OFFSET-CORRECTION-START");
                frConfig.gOffsetCorrectionStart = Convert.ToUInt16(nodeList[0].InnerText);

                nodeList = doc.GetElementsByTagName("flexray:PAYLOAD-LENGTH-STATIC");
                frConfig.gPayloadLengthStatic = Convert.ToUInt16(nodeList[0].InnerText);

                nodeList = doc.GetElementsByTagName("flexray:SYNC-NODE-MAX");
                frConfig.gSyncNodeMax = Convert.ToUInt16(nodeList[0].InnerText);

                nodeList = doc.GetElementsByTagName("flexray:ACTION-POINT-OFFSET");
                frConfig.gdActionPointOffset = Convert.ToUInt16(nodeList[0].InnerText);

                nodeList = doc.GetElementsByTagName("flexray:DYNAMIC-SLOT-IDLE-PHASE");
                frConfig.gdDynamicSlotIdlePhase = Convert.ToUInt16(nodeList[0].InnerText);

                nodeList = doc.GetElementsByTagName("flexray:MINISLOT");
                frConfig.gdMinislot = Convert.ToUInt16(nodeList[0].InnerText);

                nodeList = doc.GetElementsByTagName("flexray:MINISLOT-ACTION-POINT-OFFSET");
                frConfig.gdMiniSlotActionPointOffset = Convert.ToUInt16(nodeList[0].InnerText);

                nodeList = doc.GetElementsByTagName("flexray:N-I-T");
                frConfig.gdNIT = Convert.ToUInt16(nodeList[0].InnerText);

                nodeList = doc.GetElementsByTagName("flexray:STATIC-SLOT");
                frConfig.gdStaticSlot = Convert.ToUInt16(nodeList[0].InnerText);

                nodeList = doc.GetElementsByTagName("flexray:SYMBOL-WINDOW");
                frConfig.gdSymbolWindow = Convert.ToUInt16(nodeList[0].InnerText);

                nodeList = doc.GetElementsByTagName("flexray:T-S-S-TRANSMITTER");
                frConfig.gdTSSTransmitter = Convert.ToUInt16(nodeList[0].InnerText);

                nodeList = doc.GetElementsByTagName("flexray:WAKE-UP-SYMBOL-RX-IDLE");
                frConfig.gdWakeupSymbolRxIdle = Convert.ToUInt16(nodeList[0].InnerText);

                nodeList = doc.GetElementsByTagName("flexray:WAKE-UP-SYMBOL-RX-LOW");
                frConfig.gdWakeupSymbolRxLow = Convert.ToUInt16(nodeList[0].InnerText);

                nodeList = doc.GetElementsByTagName("flexray:WAKE-UP-SYMBOL-RX-WINDOW");
                frConfig.gdWakeupSymbolRxWindow = Convert.ToUInt16(nodeList[0].InnerText);

                nodeList = doc.GetElementsByTagName("flexray:WAKE-UP-SYMBOL-TX-IDLE");
                frConfig.gdWakeupSymbolTxIdle = Convert.ToUInt16(nodeList[0].InnerText);

                nodeList = doc.GetElementsByTagName("flexray:WAKE-UP-SYMBOL-TX-LOW");
                frConfig.gdWakeupSymbolTxLow = Convert.ToUInt16(nodeList[0].InnerText);

                nodeList = doc.GetElementsByTagName("flexray:CAS-RX-LOW-MAX");
                frConfig.gdCASRxLowMax = Convert.ToUInt16(nodeList[0].InnerText);

                nodeList = doc.GetElementsByTagName("flexray:CLUSTER-DRIFT-DAMPING");
                frConfig.pClusterDriftDamping = Convert.ToUInt16(nodeList[0].InnerText);

                //nodeList = doc.GetElementsByTagName("flexray:MACROTICK");
                // calculated by API
                frConfig.gdMacrotick = 0;

                //nodeList = doc.GetElementsByTagName("flexray:MICROTICK");
                // calculated by API
                frConfig.pdMicrotick = 0;


                // Read CONTROLLER section of the config
                nodeList = doc.GetElementsByTagName("flexray:CLUSTER-DRIFT-DAMPING");
                frConfig.pClusterDriftDamping = Convert.ToUInt16(nodeList[0].InnerText);

                nodeList = doc.GetElementsByTagName("flexray:DECODING-CORRECTION");
                frConfig.pDecodingCorrection = Convert.ToUInt16(nodeList[0].InnerText);

                nodeList = doc.GetElementsByTagName("flexray:DELAY-COMPENSATION-A");
                frConfig.pDelayCompensationA = Convert.ToUInt16(nodeList[0].InnerText);

                nodeList = doc.GetElementsByTagName("flexray:DELAY-COMPENSATION-B");
                frConfig.pDelayCompensationB = Convert.ToUInt16(nodeList[0].InnerText);

                nodeList = doc.GetElementsByTagName("flexray:EXTERN-OFFSET-CORRECTION");
                frConfig.pExternOffsetCorrection = Convert.ToUInt16(nodeList[0].InnerText);

                nodeList = doc.GetElementsByTagName("flexray:EXTERN-RATE-CORRECTION");
                frConfig.pExternRateCorrection = Convert.ToUInt16(nodeList[0].InnerText);

                nodeList = doc.GetElementsByTagName("flexray:LATEST-TX");
                frConfig.pLatestTx = Convert.ToUInt16(nodeList[0].InnerText);

                nodeList = doc.GetElementsByTagName("flexray:MACRO-INITIAL-OFFSET-A");
                frConfig.pMacroInitialOffsetA = Convert.ToUInt16(nodeList[0].InnerText);

                nodeList = doc.GetElementsByTagName("flexray:MACRO-INITIAL-OFFSET-B");
                frConfig.pMacroInitialOffsetB = Convert.ToUInt16(nodeList[0].InnerText);

                nodeList = doc.GetElementsByTagName("flexray:MICRO-INITIAL-OFFSET-A");
                frConfig.pMicroInitialOffsetA = Convert.ToUInt16(nodeList[0].InnerText);

                nodeList = doc.GetElementsByTagName("flexray:MICRO-INITIAL-OFFSET-B");
                frConfig.pMicroInitialOffsetB = Convert.ToUInt16(nodeList[0].InnerText);

                nodeList = doc.GetElementsByTagName("flexray:MICRO-PER-CYCLE");
                frConfig.pMicroPerCycle = Convert.ToUInt32(nodeList[0].InnerText);

                nodeList = doc.GetElementsByTagName("flexray:OFFSET-CORRECTION-OUT");
                frConfig.pOffsetCorrectionOut = Convert.ToUInt16(nodeList[0].InnerText);

                nodeList = doc.GetElementsByTagName("flexray:RATE-CORRECTION-OUT");
                frConfig.pRateCorrectionOut = Convert.ToUInt16(nodeList[0].InnerText);

                nodeList = doc.GetElementsByTagName("flexray:SAMPLES-PER-MICROTICK");
                frConfig.pSamplesPerMicrotick = Convert.ToUInt16(nodeList[0].InnerText);

                nodeList = doc.GetElementsByTagName("flexray:SINGLE-SLOT-ENABLED");
                if (nodeList[0].InnerText == "true")
                {
                    frConfig.pSingleSlotEnabled = 1;
                }
                else
                {
                    frConfig.pSingleSlotEnabled = 0;
                }

                nodeList = doc.GetElementsByTagName("flexray:WAKE-UP-PATTERN");
                frConfig.pWakeupPattern = Convert.ToUInt16(nodeList[0].InnerText);

                nodeList = doc.GetElementsByTagName("flexray:ALLOW-HALT-DUE-TO-CLOCK");
                if (nodeList[0].InnerText == "true")
                {
                    frConfig.pAllowHaltDueToClock = 1;
                }
                else
                {
                    frConfig.pAllowHaltDueToClock = 0;
                }

                nodeList = doc.GetElementsByTagName("flexray:ALLOW-PASSIVE-TO-ACTIVE");
                frConfig.pAllowPassiveToActive = Convert.ToUInt16(nodeList[0].InnerText);

                nodeList = doc.GetElementsByTagName("flexray:ACCEPTED-STARTUP-RANGE");
                frConfig.pdAcceptedStartupRange = Convert.ToUInt16(nodeList[0].InnerText);

                nodeList = doc.GetElementsByTagName("flexray:LISTEN-TIMEOUT");
                frConfig.pdListenTimeout = Convert.ToUInt32(nodeList[0].InnerText);

                nodeList = doc.GetElementsByTagName("flexray:MAX-DRIFT");
                frConfig.pdMaxDrift = Convert.ToUInt16(nodeList[0].InnerText);

                nodeList = doc.GetElementsByTagName("flexray:MAX-DYNAMIC-PAYLOAD-LENGTH");
                frConfig.pMaxPayloadLengthDynamic = Convert.ToUInt16(nodeList[0].InnerText);


                nodeList = doc.GetElementsByTagName("fx:CHANNEL");
                frConfig.pChannels = 0;
                foreach (XmlNode child in nodeList)
                {
                    string value = child["ho:SHORT-NAME"].InnerText;
                    if (value.Equals("Channel_A"))
                    {
                        frConfig.pChannels |= 1;
                    }
                    else if (value.Equals("Channel_B"))
                    {
                        frConfig.pChannels |= 2;
                    }
                }

                frConfig.pWakeupChannel = XLDefine.XL_FlexRay_FlagsChip.XL_FR_CHANNEL_A;
            }

            catch (Exception e)
            {
                return new Exception("Keyword not found! " + e.Message);
            }

            //FibexDescription extraction
            try
            {
                nodeList = doc.GetElementsByTagName("ho:LONG-NAME");
                desc.Name = nodeList[0].InnerText;
                nodeList = doc.GetElementsByTagName("ho:DESC");
                desc.Description = nodeList[0].InnerText;
            }
            catch
            {
                //Not important
            }
            return null;
        }
    }

    class FibexDescription
    {
        public string Name { get; set; }
        public string Description { get; set; }

        public FibexDescription()
        {
            Name = string.Empty;
            Description = string.Empty;
        }
    }
}