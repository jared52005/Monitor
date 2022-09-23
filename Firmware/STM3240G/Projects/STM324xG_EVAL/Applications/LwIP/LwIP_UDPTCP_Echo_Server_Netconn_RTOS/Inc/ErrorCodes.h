/*******************************************************************************
* @file    ErrorCodes.h
* @author  Jaromir Mayer
* @brief   Errors shared over all modules for easier error interpretation on use side
******************************************************************************
* @attention
******************************************************************************  
*/

#ifndef ERROR_CODES_H
#define ERROR_CODES_H
/**
* @brief  List of errors, which cable can generate in uint32 format
*/
typedef enum
{
	ERROR_OK               = 0x00000000, // No error
	ERROR_GENERAL          = 0x7FFFFFFF, // Default error
	ERROR_INVALID_OPCODE   = 0x00000001, // Used unexpected opcode
	ERROR_DATA_EMPTY       = 0x00000002, //<! There are no data in buffer
	ERROR_DATA_OVERFLOW    = 0x00000003, //<! Data in buffer has overflown
	ERROR_DATA_FULL        = 0x00000004, //<! Receive buffer is full
	ERROR_TASK_START_FAIL  = 0x00000010, //<! Unable to start task

	ERROR_CAN_TOO_BIG      = 0x00000101, //<! We are trying to transmit more than 8 bytes of data
	ERROR_CAN_NOT_ENABLED  = 0x00000102, //<! CAN peripheral is not enabled
	ERROR_CAN_IS_PASSIVE   = 0x00000103, //<! Can't transmit on CAN when in passive mode
    ERROR_CAN_TX_STUFF     = 0x00000104, //<! CAN Stuff Error; Possible reason: 120R missing
    ERROR_CAN_TX_FORM      = 0x00000105, //<! CAN Form Error
    ERROR_CAN_TX_ACK       = 0x00000106, //<! CAN Acknowledgment Error; Possible reason: No device on bus to ACK our message
    ERROR_CAN_TX_BIT_REC   = 0x00000107, //<! CAN Bit recessive Error
    ERROR_CAN_TX_BIT_DOM   = 0x00000108, //<! CAN Bit dominant Error Possible reason: CAN-L / CAN-H swapped
    ERROR_CAN_TX_CRC       = 0x00000109, //<! CAN CRC Error	
	ERROR_CAN_TX_TIMEOUT   = 0x0000010A, //<! When transmission has timed out. 
	ERROR_CAN_TX_PENDING   = 0x0000010B, //<! Message is pending (CAN arbitration not won yet)
	ERROR_CAN_UNS_BAUDRATE = 0x00000110, //<! Unsupported baudrate
	ERROR_CAN_INV_CLK      = 0x00000111, //<! TSEQ1/TSEQ2/SJW is set for specific clocks (i.e. 42MHz) but setup routine found, different clocks
    
    ERROR_ISO15765_DISABLED = 0x00000201,   //<! Transport protocol is not enabled. Use Iso15765_Enable
	ERROR_ISO15765_TX_TIMEOUT = 0x00000202, //<! Timeout during attempting to transmit data

	ERROR_VWTP20_DISABLED   = 0x00000301, //<! UART or Timer is not inited for correct operation.
	ERROR_VWTP20_TX_TIMEOUT = 0x00000302, //<! Timeout transmission if there is no change in states for VWTP20_TRANSMIT_TIMEOUT
    

    ERROR_ISO9141_DISABLED  = 0x00000401, //<! UART or Timer is not inited for correct operation.

	ERROR_GPIO_INVALID_NAME = 0x00000501, //<! Invalid name of GPIO pin (not implemented in device)

	ERROR_PWM_INVALID_NAME  = 0x00000601, //<! Unknown or not supported name of channel
	ERROR_PWM_OUT_OF_RANGE  = 0x00000602, //<! Arguments for PWM are invalid (too big frequency, etc.)

	ERROR_SYSTEM_ID_ALREADY_SET = 0x00000701, //<! ID is already set in memory
	ERROR_SYSTEM_ID_CHANGE_NOT_SUPPORTED= 0x00000702, //<! Can not change ID for this MCU. Probably hardcoded in Silicone

	ERROR_USB_NOT_CONNECTED    = 0x00000801, //!< Not connected to PC 
	ERROR_USB_CONNECTED        = 0x00000802, //!< Connected to PC
	ERROR_USB_DEVICE_SUSPENDED = 0x00000803, //!< Device is suspended
	ERROR_USB_DEVICE_RESUMED   = 0x00000804, //!< Device is resumed
}ErrorCodes;
#endif
