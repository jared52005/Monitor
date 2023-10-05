/*******************************************************************************
* @file    UsbIf.h
* @version V1.0
* @date    05-November-2018
* @brief   
******************************************************************************
* @attention
******************************************************************************  
*/

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief VCP Result Enumerations
 */
typedef enum {
	USB_VCP_OK,                   /*!< Everything ok */
	USB_VCP_ERROR,                /*!< An error occurred */
	USB_VCP_RECEIVE_BUFFER_FULL,  /*!< Receive buffer is full */
	USB_VCP_RECEIVE_BUFFER_OVER,  /*!< USB Receive buffer has been overflown. Reset device*/
	USB_VCP_TRANSMIT_BUFFER_FULL, /*!< Transmit buffer is full*/
	USB_VCP_DATA_OK,              /*!< Data OK */
	USB_VCP_DATA_EMPTY,           /*!< Data empty */
	USB_VCP_NOT_CONNECTED,        /*!< Not connected to PC */
	USB_VCP_CONNECTED,            /*!< Connected to PC */
	USB_VCP_DEVICE_SUSPENDED,     /*!< Device is suspended */
	USB_VCP_DEVICE_RESUMED        /*!< Device is resumed */
} USB_VCP_Result;

USB_VCP_Result USB_Init(void);
USB_VCP_Result USB_TransmitPacket(uint8_t* DataArray, uint32_t Length);
uint16_t USB_ReceivePacket(uint8_t* buffer, uint16_t bufsize);
USB_VCP_Result USB_Putc(uint8_t c);
