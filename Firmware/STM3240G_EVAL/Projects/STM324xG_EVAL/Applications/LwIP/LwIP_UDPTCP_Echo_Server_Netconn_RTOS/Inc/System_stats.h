/*******************************************************************************
 * @file    stats.h
 * @author  Jaromir Mayer
 * @brief   Structure holding statistical data. Can be used for debugging or
 *          to inform about system health
 ******************************************************************************
 * @attention
 ******************************************************************************  
 */ 

#include <stdint.h>

#ifndef SYSTEM_STATS_H
#define SYSTEM_STATS_H

typedef enum 
{
    UsbInterface,      //Primary active interface
    UartInterface,     //Secondary interface for STM3240G EVAL and C10_P1A
    SpiInterface,      //Secondary interface for C10_ESP32_P1B and C21 devices
    EthernetInterface, //Secondary interface for STM3240G EVAL (select in PDU router)
} StatsActiveDeviceType;

/**
 * @brief Reset all statistics on 0
 */
void Stats_Reset(void);

/**
 * @brief Update statistics. Call cca once a second
 */
void Stats_Update(void);

/**
 * @brief Get calculated bus load
 */
uint32_t Stats_CanMessages_BusLoad_Get(void);

/**
 * @brief Get total amount of received messages
 */
uint32_t Stats_CanMessages_RxTotal_Get(void);

/**
 * @brief Get total amount of received CAN bytes
 * @note  In sense of 4 bytes for CANID + 1 byte DLC + 0-8 bytes of data
 */
uint32_t Stats_CanBytes_RxTotal_Get(void);

/**
 * @brief Get total amount of received CAN bytes per second
 * @note  In sense of 4 bytes for CANID + 1 byte DLC + 0-8 bytes of data
 */
uint32_t Stats_CanBytes_RxPerSecond_Get(void);

/**
 * @brief Add n bytes to amount of already received bytes
 * @param dlc: DLC of CAN message
 * @param extendedFrame: If 0, then processed as standard frame, otherwise processed as extended frame
 * @param baudrate: used for calculation of bus load
 */
void Stats_CanMessage_RxAdd(uint8_t dlc, uint8_t extendedFrame, uint32_t baudrate);

/**
 * @brief Get total amount of transmitted messages
 */
uint32_t Stats_CanMessages_TxTotal_Get(void);

/**
 * @brief Add n bytes to amount of already received bytes
 * @param dlc: DLC of CAN message
 * @param extendedFrame: If 0, then processed as standard frame, otherwise processed as extended frame
 * @param baudrate: used for calculation of bus load
 */
void Stats_CanMessage_TxAdd(uint8_t dlc, uint8_t extendedFrame, uint32_t baudrate);

/**
 * @brief Get total amount of received bytes
 */
uint64_t Stats_UsbBytes_RxTotal_Get(void);

/**
 * @brief Get total amount of received messages
 */
uint32_t Stats_UsbMessages_RxTotal_Get(void);

/**
 * @brief Add n bytes to amount of already received bytes
 */
void Stats_UsbBytes_RxAdd(uint32_t n);

/**
 * @brief Get total amount of transmitted bytes
 */
uint64_t Stats_UsbBytes_TxTotal_Get(void);

/**
 * @brief Get total amount of transmitted messages
 */
uint32_t Stats_UsbMessages_TxTotal_Get(void);

/**
 * @brief Add n bytes to amount of already transmitted bytes
 */
void Stats_UsbBytes_TxAdd(uint32_t n);

/**
 * @brief Get total amount of received CAN bytes per second
 */
uint32_t Stats_UsbBytes_RxPerSecond_Get(void);

/**
 * @brief Get total amount of received CAN bytes per second
 */
uint32_t Stats_UsbBytes_TxPerSecond_Get(void);

/**
 * @brief Set which device is curently active to talk to user
 */
void Stats_UsbActiveDevice_Set(StatsActiveDeviceType device);

/**
 * @brief Get which device is curently active to talk to user
 */
StatsActiveDeviceType Stats_UsbActiveDevice_Get(void);
#endif
