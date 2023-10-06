#include "stm32f4xx.h"
#include "UartIf.h"
#include "stdio.h"
#include "SystemIf.h"
#include "GptIf.h"
#include "UsbIf.h"
#include "Usb_Vcp_If.h"
#include "rtos_utils.h"
#include "application.h"
//******************************************************************************
#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"
#include "croutine.h"
//******************************************************************************
/* Private variables ---------------------------------------------------------*/
xTaskHandle Task_Handle;
/* Private function prototypes -----------------------------------------------*/
static void taskMainThread(void *argument);
static void taskSysTick(void *argument);

/**
  * @brief  Main program.
  * @param  None
  * @retval None
  */
int main(void) 
{
	RCC_ClocksTypeDef RCC_ClockFreq;

	//Check DFUB flag. If found, jump on DFU bootloader
  	if(*((uint32_t*)0x20000000) == 0x44465542)
  	{
    	*((uint32_t*)0x20000000) = 0x0; //Delete flag to prevent possible loop
    	System_JumpOnSystemBootloader();
  	}
	/*!< At this stage the microcontroller clock setting is already configured,
	   this is done through SystemInit() function which is called from startup
	   file (startup_stm32f4xx.s) before to branch to application main.
	   To reconfigure the default setting of SystemInit() function, refer to
	   system_stm32f4xx.c file
	 */
	
	/*!< Most systems default to the wanted configuration, with the noticeable 
		exception of the STM32 driver library. If you are using an STM32 with 
		the STM32 driver library then ensure all the priority bits are assigned 
		to be preempt priority bits by calling 
		NVIC_PriorityGroupConfig( NVIC_PriorityGroup_4 ); before the RTOS is started.
	*/
	RCC_GetClocksFreq(&RCC_ClockFreq, HSE_VALUE);
	NVIC_PriorityGroupConfig( NVIC_PriorityGroup_4 );
	
	//1) Start main thread which is taking care of all functionality
	xTaskCreate(taskMainThread, (const char *)"MainThread", configMINIMAL_STACK_SIZE * 4, NULL, tskIDLE_PRIORITY, &Task_Handle);
	vTaskStartScheduler();
	
	/* We should never get here as control is now taken by the scheduler */
	for( ;; );
}

void taskMainThread(void* argument)
{
	//Init timer (port.c has some retarded init, which does not work correctly)
	SysTick_Config(configCPU_CLOCK_HZ / configTICK_RATE_HZ);
	Application();
}

struct __FILE { int handle; /* Add whatever you need here */ };
FILE __stdout;
FILE __stdin;

/**
 * @brief this method is called from printf() to handle single chars
 */
int fputc(int ch, FILE *f) 
{
  Uart_Putc_Debug((char)ch);
  TM_USB_VCP_Putc((char)ch);
  return(ch);
}

#ifdef USE_FULL_ASSERT
/*******************************************************************************
 * Function Name  : assert_failed
 * Description    : Reports the name of the source file and the source line number
 *                  where the assert_param error has occurred.
 * Input          : - file: pointer to the source file name
 *                  - line: assert_param error line source number
 * Output         : None
 * Return         : None
 *******************************************************************************/
void assert_failed(uint8_t* file, uint32_t line)
{
    /* User can add his own implementation to report the file name and line number,
     ex: t_printf("Wrong parameters value: file %s on line %d\r\n", file, line) */

    /* Infinite loop */
    while (1)
    {}
}
#endif
