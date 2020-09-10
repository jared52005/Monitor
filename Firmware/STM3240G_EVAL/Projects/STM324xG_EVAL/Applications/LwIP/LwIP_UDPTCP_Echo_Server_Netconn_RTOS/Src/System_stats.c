/*******************************************************************************
 * @file    stats.c
 * @author  Jaromir Mayer
 * @brief   Structure holding statistical data. Can be used for debugging or
 *          to inform about system health
 ******************************************************************************
 * @attention
 ******************************************************************************  
 */ 

#include "System_stats.h"
#include "rtos_utils.h"

typedef struct 
{
 uint64_t ElementsTx; //How many bits or bytes did we transmit
 uint64_t ElementsRx; //How many bits or bytes did we received
 uint32_t MsgsTx;  //How many mesages did we transmitted
 uint32_t MsgsRx;  //How many messages did we received
}PduStats;


static PduStats _usb;
static PduStats _can;
static StatsActiveDeviceType _usbDevice;
static uint32_t _usbBytesReceivedPrevious;
static uint32_t _usbBytesReceivedPerSecond;
static uint32_t _usbBytesTransmitedPrevious;
static uint32_t _usbBytesTransmitedPerSecond;

static uint32_t _canBusLoad; //How much is CAN bus loaded in percents (xxx.y = xxxy)
static uint32_t _canBitsStdMsg[] = {51, 60, 70, 79, 89, 99, 108, 118, 127}; //How many bits is in standard message by DLC
static uint32_t _canBitsExtMsg[] = {75, 84, 94, 103, 113, 123, 132, 142, 151}; //How many bits is in extended message by DLC
static uint32_t _canBaudrate;
static uint32_t _canBytesReceived;
static uint32_t _canBytesReceivedPrevious;
static uint32_t _canBytesReceivedPerSecond;

static uint32_t _lastTime;

void Stats_Reset(void)
{
    _usb.ElementsTx = 0;
    _usb.ElementsRx = 0;
    _usb.MsgsTx = 0;
    _usb.MsgsRx = 0;
    _usbBytesReceivedPrevious = 0;
    _usbBytesReceivedPerSecond = 0;
    _usbBytesTransmitedPrevious = 0;
    _usbBytesTransmitedPerSecond = 0;
    
    _can.ElementsTx = 0;
    _can.ElementsRx = 0;
    _can.MsgsTx = 0;
    _can.MsgsRx = 0;
    _canBusLoad = 0;
    _canBaudrate = 0;
    _canBytesReceived = 0;
    _canBytesReceivedPrevious = 0;
    _canBytesReceivedPerSecond = 0;
    _lastTime = GetTime_ms();
}

/**
 * @brief Update statistics. Call cca once a second
 */
void Stats_Update(void)
{
    double canBusLoad;
    double diffTime = (GetTime_ms() - _lastTime) / 1000.0; //Get diff time in seconds
    uint64_t totalBits = _can.ElementsTx + _can.ElementsRx;
    canBusLoad = totalBits / diffTime;
    _can.ElementsTx = 0;
    _can.ElementsRx = 0;
    _lastTime = GetTime_ms();

    if(canBusLoad != 0)
    {
        canBusLoad = canBusLoad / _canBaudrate;
        _canBusLoad = (uint32_t)(canBusLoad * 100);
    }
    else
    {
        _canBusLoad = 0;
    }

    if(_canBytesReceived != 0)
    {
        _canBytesReceivedPerSecond = (uint32_t)((_canBytesReceived - _canBytesReceivedPrevious) / diffTime);
    }
    _canBytesReceivedPrevious = _canBytesReceived;

    //Update User interface communication (USB, UART, SPI) stats
    if(_usb.ElementsRx != 0)
    {
        _usbBytesReceivedPerSecond = (uint32_t)((_usb.ElementsRx - _usbBytesReceivedPrevious) / diffTime);
    }
    _usbBytesReceivedPrevious = _usb.ElementsRx;
    if(_usb.ElementsTx != 0)
    {
        _usbBytesTransmitedPerSecond = (uint32_t)((_usb.ElementsTx - _usbBytesTransmitedPrevious) / diffTime);
    }
    _usbBytesTransmitedPrevious = _usb.ElementsTx;
}

/**
 * @brief Get calculated bus load
 */
uint32_t Stats_CanMessages_BusLoad_Get()
{
    return _canBusLoad;
}

/**
 * @brief Get total amount of received messages
 */
uint32_t Stats_CanMessages_RxTotal_Get()
{
    return _can.MsgsRx;
}

/**
 * @brief Get total amount of received CAN bytes
 * @note  In sense of 4 bytes of timestamp [ms] 4 bytes for CANID + 1 byte DLC + 0-8 bytes of data
 */
uint32_t Stats_CanBytes_RxTotal_Get(void)
{
    return _canBytesReceived;   
}

/**
 * @brief Get total amount of received CAN bytes per second
 * @note  In sense of 4 bytes of timestamp [ms] 4 bytes for CANID + 1 byte DLC + 0-8 bytes of data
 */
uint32_t Stats_CanBytes_RxPerSecond_Get(void)
{
    return _canBytesReceivedPerSecond;
}

/**
 * @brief Add n bytes to amount of already received bytes
 * @param dlc: DLC of CAN message
 * @param extendedFrame: If 0, then processed as standard frame, otherwise processed as extended frame
 * @param baudrate: used for calculation of bus load
 */
void Stats_CanMessage_RxAdd(uint8_t dlc, uint8_t extendedFrame, uint32_t baudrate)
{
    _can.MsgsRx++;
    _canBaudrate = baudrate;
    if(extendedFrame == 0)
    {
        _can.ElementsRx += _canBitsStdMsg[dlc];
    }
    else
    {
        _can.ElementsRx += _canBitsExtMsg[dlc];
    }
    _canBytesReceived += (9 + dlc); //Timestamp_ms = 4, CANID = 4, DLC = 1, Data[DLC]
}

/**
 * @brief Get total amount of transmitted messages
 */
uint32_t Stats_CanMessages_TxTotal_Get(void)
{
    return _can.MsgsTx;
}

/**
 * @brief Add n bytes to amount of already received bytes
 * @param dlc: DLC of CAN message
 * @param extendedFrame: If 0, then processed as standard frame, otherwise processed as extended frame
 * @param baudrate: used for calculation of bus load
 */
void Stats_CanMessage_TxAdd(uint8_t dlc, uint8_t extendedFrame, uint32_t baudrate)
{
    _can.MsgsTx++;
    _canBaudrate = baudrate;
    if(extendedFrame == 0)
    {
        _can.ElementsTx += _canBitsStdMsg[dlc];
    }
    else
    {
        _can.ElementsTx += _canBitsExtMsg[dlc];
    }
}

/**
 * @brief Get total amount of received bytes
 */
uint64_t Stats_UsbBytes_RxTotal_Get(void)
{
    return _usb.ElementsRx;
}

/**
 * @brief Get total amount of received messages
 */
uint32_t Stats_UsbMessages_RxTotal_Get(void)
{
    return _usb.MsgsRx;
}

/**
 * @brief Add n bytes to amount of already received bytes
 */
void Stats_UsbBytes_RxAdd(uint32_t n)
{
    _usb.MsgsRx++;
    _usb.ElementsRx += n;
}

/**
 * @brief Get total amount of transmitted bytes
 */
uint64_t Stats_UsbBytes_TxTotal_Get(void)
{
    return _usb.ElementsTx;
}

/**
 * @brief Get total amount of transmitted messages
 */
uint32_t Stats_UsbMessages_TxTotal_Get(void)
{
    return _usb.MsgsTx;
}

/**
 * @brief Get total amount of received CAN bytes per second
 */
uint32_t Stats_UsbBytes_RxPerSecond_Get(void)
{
    return _usbBytesReceivedPerSecond;
}

/**
 * @brief Get total amount of received CAN bytes per second
 */
uint32_t Stats_UsbBytes_TxPerSecond_Get(void)
{
    return _usbBytesTransmitedPerSecond;
}

/**
 * @brief Add n bytes to amount of already transmitted bytes
 */
void Stats_UsbBytes_TxAdd(uint32_t n)
{
    _usb.MsgsTx++;
    _usb.ElementsTx += n;
}

/**
 * @brief Set which device is curently active to talk to user
 */
void Stats_UsbActiveDevice_Set(StatsActiveDeviceType device)
{
    _usbDevice = device;
}

/**
 * @brief Get which device is curently active to talk to user
 */
StatsActiveDeviceType Stats_UsbActiveDevice_Get(void)
{
    return _usbDevice;
}
