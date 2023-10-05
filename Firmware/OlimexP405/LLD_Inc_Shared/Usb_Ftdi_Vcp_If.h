/*******************************************************************************
* @file    Usb_Ftdi_Vcp_If.h
* @author  Jaromir Mayer
* @version V1.0
* @date    05-November-2018
* @brief   Generic interface for FTDI FT245 emulation
******************************************************************************
* @attention
******************************************************************************  
*/ 
#include <stdint.h>
#include <stdbool.h>

/**
 * @brief  Default buffer length
 * @note   Increase this value if you need more memory for VCP receive data
 */
#ifndef FTDI_FIFO_SIZE
#define FTDI_FIFO_SIZE 2048
#endif
 
/**
 * @brief VCP Result Enumerations
 */
typedef enum {
	TM_USB_VCP_OK,                   /*!< Everything ok */
	TM_USB_VCP_ERROR,                /*!< An error occurred */
	TM_USB_VCP_RECEIVE_BUFFER_FULL,  /*!< Receive buffer is full */
	TM_USB_VCP_TRANSMIT_BUFFER_FULL, /*!< Transmit buffer is full*/
	TM_USB_VCP_DATA_OK,              /*!< Data OK */
	TM_USB_VCP_DATA_EMPTY,           /*!< Data empty */
	TM_USB_VCP_NOT_CONNECTED,        /*!< Not connected to PC */
	TM_USB_VCP_CONNECTED,            /*!< Connected to PC */
	TM_USB_VCP_DEVICE_SUSPENDED,     /*!< Device is suspended */
	TM_USB_VCP_DEVICE_RESUMED        /*!< Device is resumed */
} TM_USB_VCP_Result;


/**
 * @brief  Initializes USB VCP via FTDI
 * @param  None
 * @retval TM_USB_VCP_OK
 */
TM_USB_VCP_Result TM_USB_VCP_Init(void);

/**
 * @brief  Refresh FIFO inside USB (Only for FTDI emulation)
 * @param  *c: pointer to store new character to
 * @retval Character status:
 *            - TM_USB_VCP_DATA_OK: Character is valid inside *c_str
 *            - TM_USB_VCP_DATA_EMPTY: No character in *c
 */
TM_USB_VCP_Result TM_USB_VCP_Refresh(void);

/**
 * @brief  Refresh FTDI driver via SysTick (Only for FTDI emulation)
 * @param  *c: pointer to store new character to
 * @retval Character status:
 *            - TM_USB_VCP_DATA_OK: Character is valid inside *c_str
 *            - TM_USB_VCP_DATA_EMPTY: No character in *c
 */
TM_USB_VCP_Result TM_USB_VCP_SysTick_Refresh(void);

/**
 * @brief  Gets received character from internal buffer
 * @param  *c: pointer to store new character to
 * @retval Character status:
 *            - TM_USB_VCP_DATA_OK: Character is valid inside *c_str
 *            - TM_USB_VCP_DATA_EMPTY: No character in *c
 */
TM_USB_VCP_Result TM_USB_VCP_Getc(uint8_t* c);

/**
 * @brief  Puts character to USB VCP
 * 
 * @note   Writes data into write buffer, but does not send them 
 *         until '\n' is written. 
 * @param  c: character to send over USB
 * @retval TM_USB_VCP_OK
 */
TM_USB_VCP_Result TM_USB_VCP_Putc(volatile char c);

/**
 * @brief  Puts string to USB VCP
 * @param  *str: Pointer to string variable ending '\n'
 * @retval TM_USB_VCP_OK
 *         TM_USB_VCP_ERROR: We want to send data, but data are not ending '\n'
 */
TM_USB_VCP_Result TM_USB_VCP_Puts(char* str);

/**
 * @brief  Sends array of data to USB VCP
 * @note   Writes data directly to FTDI driver up to FIFO_LENGTH
 * @param  *DataArray: Pointer to 8-bit data array to be sent over USB
 * @param  Length: Number of elements to sent in units of bytes
 * @retval Sending status
 */
TM_USB_VCP_Result TM_USB_VCP_Send(uint8_t* DataArray, uint32_t Length);
//bool USBWriteBuffer(uint32_t lenght, uint8_t *buffer);
//void USBMoveToNextMessage(void); //When receiving is done

/**
 * @brief  Gets VCP status
 * @param  None
 * @retval Device status:
 *            - TM_USB_VCP_CONNECTED: Connected to computer
 *            - other: Not connected and not ready to communicate
 */
TM_USB_VCP_Result TM_USB_VCP_GetStatus(void);
