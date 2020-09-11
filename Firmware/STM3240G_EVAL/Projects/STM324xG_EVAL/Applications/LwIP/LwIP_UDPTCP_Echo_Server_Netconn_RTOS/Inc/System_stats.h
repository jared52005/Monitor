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
 * @brief Get total amount of received bytes
 */
uint32_t Stats_KlineBytes_RxTotal_Get(void);

/**
 * @brief Get total amount of received messages
 */
uint32_t Stats_KlineFrames_RxTotal_Get(void);

/**
 * @brief Add n bytes to amount of already received bytes
 */
void Stats_KlineBytes_RxAdd(uint32_t n);

/**
 * @brief Get total amount of received CAN bytes per second
 */
uint32_t Stats_KlineBytes_RxPerSecond_Get(void);

/**
 * @brief Upload state of DHCP into stats, so it can be shown on LCD
*/
void Stats_DHCP_SetState(uint8_t s);

/**
 * @brief Return state of DHCP 
 */
uint32_t Stats_DHCP_GetState(void);

void Stats_IP_Set(char* ip);
char* Stats_IP_Get(void);
#endif
