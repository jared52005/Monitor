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
#include "string.h"

typedef struct 
{
 uint64_t ElementsRx; //How many bits or bytes did we received
 uint32_t MsgsRx;  //How many messages did we received
}PduStats;


static PduStats _kline;
static PduStats _can;
static uint32_t _klineBytesReceivedPrevious;
static uint32_t _klineBytesReceivedPerSecond;

static uint32_t _canBusLoad; //How much is CAN bus loaded in percents (xxx.y = xxxy)
static uint32_t _canBitsStdMsg[] = {51, 60, 70, 79, 89, 99, 108, 118, 127}; //How many bits is in standard message by DLC
static uint32_t _canBitsExtMsg[] = {75, 84, 94, 103, 113, 123, 132, 142, 151}; //How many bits is in extended message by DLC
static uint32_t _canBaudrate;
static uint32_t _canBytesReceived;
static uint32_t _canBytesReceivedPrevious;
static uint32_t _canBytesReceivedPerSecond;

static uint32_t _lastTime;

static uint32_t _dhcpState;
static char _ipAddress[20];

void Stats_Reset(void)
{
    _kline.ElementsRx = 0;
    _kline.MsgsRx = 0;
    
    _can.ElementsRx = 0;
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
    uint64_t totalBits = _can.ElementsRx;
    canBusLoad = totalBits / diffTime;
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

    //Update communication parameters for KLINE
    if(_kline.ElementsRx != 0)
    {
        _klineBytesReceivedPerSecond = (uint32_t)((_kline.ElementsRx - _klineBytesReceivedPrevious) / diffTime);
    }
    _klineBytesReceivedPrevious = _kline.ElementsRx;
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
 * @brief Get total amount of received bytes
 */
uint32_t Stats_KlineBytes_RxTotal_Get(void)
{
    return _kline.ElementsRx;
}

/**
 * @brief Get total amount of received messages
 */
uint32_t Stats_KlineFrames_RxTotal_Get(void)
{
    return _kline.MsgsRx;
}

/**
 * @brief Add n bytes to amount of already received bytes
 */
void Stats_KlineBytes_RxAdd(uint32_t n)
{
    _kline.MsgsRx++;
    _kline.ElementsRx += n;
}

/**
 * @brief Get total amount of received CAN bytes per second
 */
uint32_t Stats_KlineBytes_RxPerSecond_Get(void)
{
    return _klineBytesReceivedPerSecond;
}

/**
 * @brief Upload state of DHCP into stats, so it can be shown on LCD
*/
void Stats_DHCP_SetState(uint8_t s)
{
    _dhcpState = (uint32_t)s;
}

/**
 * @brief Return state of DHCP 
 */
uint32_t Stats_DHCP_GetState(void)
{
    return _dhcpState;
}

/**
 * @brief Save IP address to be shown on LCD
 */
void Stats_IP_Set(char* ip)
{
    strcpy(_ipAddress, ip);
}

/**
 * @brief Get IP address
 */
char* Stats_IP_Get(void)
{
    return _ipAddress;
}
