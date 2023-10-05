# SLCAN  
This folder contains implemenation of SLCAN on Olimex P405

## Variants
Each variant contains list of global symbols necessary for proper compilation
**STM32F4xx**
  * STM32F4x5 - Cv1.0; [CAN, KLINE] `USE_STDPERIPH_DRIVER`, `STM32F4XX`, `HSE_VALUE=8000000`, `DEBUG_TRACE`, `OLIMEXP405_Cv10`
  * STM32F4x5 - Cv2.0; [CAN, KLINE] `USE_STDPERIPH_DRIVER`, `STM32F4XX`, `HSE_VALUE=8000000`, `DEBUG_TRACE`, `OLIMEXP405_Cv20`

## Firmware Structure  
  * APP_STM32F4xx - Application build from STM32F4xx source codes and BSW
  * BSW_ - Basic software common for both applications, independent on HAL layer
  * RTOS - FreeRTOS to separate emulated tasks on STM32F4xx
  * LLD_STM32F4xx - Low Level drivers for STM32F4xx
