# How to calcualte bus load?

ETAS BUSMASTER software is having a [neat functions in BusStatisticCAN.cpp](https://github.com/rbei-etas/busmaster/blob/master/Sources/BUSMASTER/Application/BusStatisticCAN.cpp) for calculation of bus load. This is important when we are having troubles with busy networks which we are trying to flash on.

## How many bits are in a CAN message?
For standard message we have:  
`UINT unBitsStdMsg[] = {51, 60, 70, 79, 89, 99, 108, 118, 127};`

And for extended message we have:  
`UINT unBitsExtMsg[] = {75, 84, 94, 103, 113, 123, 132, 142, 151};`

Notice that there is 9 variations. We can have message with DLC=0 up to message with DLC=8 which is 9 possible alterations of CAN message size

## Received bits
Everytime when we receive a message, then by using its DLC we will select amount of received bits
```
//Regular message
if (m_sCurrEntry.m_ucEXTENDED == 0)
{
    m_sSubBusStatistics[ nCurrentChannelIndex ].m_unTxSTDMsgCount++;  
    m_sSubBusStatistics[ nCurrentChannelIndex ].m_unTotalBitsperSec += m_unBitsStdMsg[nDLC];
}
else
{
    m_sSubBusStatistics[ nCurrentChannelIndex ].m_unTxEXTDMsgCount++;  
    m_sSubBusStatistics[ nCurrentChannelIndex ].m_unTotalBitsperSec += m_unBitsExdMsg[nDLC];
}
...
// RTR message
if (m_sCurrEntry.m_ucEXTENDED == 0)
{
    m_sSubBusStatistics[ nCurrentChannelIndex ].m_unTxSTD_RTRMsgCount++;
    m_sSubBusStatistics[ nCurrentChannelIndex ].m_unTotalBitsperSec += m_unBitsStdMsg[0];
}
else
{
    m_sSubBusStatistics[ nCurrentChannelIndex ].m_unTxEXTD_RTRMsgCount++;
    m_sSubBusStatistics[ nCurrentChannelIndex ].m_unTotalBitsperSec += m_unBitsExdMsg[0];
}
```

## Calcualtion of bus load
First we have some differential time to sample bus load. Usually it is 1 second. This means that amount of received bits will get divided by diffTime = 1 and then reset amount of received bits.
```
DOUBLE dBusLoad  = m_sBusStatistics[ nChannelIndex ].m_unTotalBitsperSec / m_dDiffTime;
m_sBusStatistics[ nChannelIndex ].m_unTotalBitsperSec = 0; //Reset amount of received bits for another calcualtion
m_sSubBusStatistics[ nChannelIndex ].m_unTotalBitsperSec = 0;
```

Then if intermediate dBusLoad != 0 we can calculate dBusLoad by dividing baudrate per second.
This will get us ratio of used bits over capacity of a bus.
```
if(dBusLoad != 0)
{
    dBusLoad = dBusLoad / ( m_sBusStatistics[ nChannelIndex ].m_dBaudRate);
}
```

To get percentage bus load we can use:  
`dBusLoad = dBusLoad * defMAX_PERCENTAGE_BUS_LOAD;`  
Where defMAX_PERCENTAGE_BUS_LOAD = 100.0

Then if previous peak bus load is higher than actually calculated, set m_dPeakBusLoad = dBusLoad
```
// check for peak load
if( dBusLoad > m_sBusStatistics[ nChannelIndex ].m_dPeakBusLoad )
{
    // if peak load is greater than or equal to 100% assign it 99.99%
    if( dBusLoad > defMAX_PERCENTAGE_BUS_LOAD )
    {
        dBusLoad = defMAX_PERCENTAGE_BUS_LOAD_ALLOWED;
    }
    m_sBusStatistics[ nChannelIndex ].m_dPeakBusLoad = dBusLoad ;
}
```

To get average bus load, we will use total amount of busload divided by amount of samples. Most probably we are going to sample once in 1 second.
```
// Calculate avarage bus load
m_sBusStatistics[ nChannelIndex ].m_dTotalBusLoad += dBusLoad;
// Increament samples
m_sBusStatistics[ nChannelIndex ].m_nSamples++;
// Calculate Avarage bus load
if(bIsConnected)
{
    m_sBusStatistics[ nChannelIndex ].m_dAvarageBusLoad =
    m_sBusStatistics[ nChannelIndex ].m_dTotalBusLoad /
    m_sBusStatistics[ nChannelIndex ].m_nSamples;
}
```