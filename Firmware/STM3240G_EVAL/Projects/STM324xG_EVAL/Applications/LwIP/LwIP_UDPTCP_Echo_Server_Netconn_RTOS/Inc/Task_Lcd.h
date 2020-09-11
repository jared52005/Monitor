/*******************************************************************************
  * @brief   Module controlling generic LCD display, which can write lines
 ******************************************************************************
 * @attention
 ******************************************************************************  
 */ 

#include <stdint.h>

/**
* @brief  Task for printing data on LCD. Low prioririty task. Run only once in 100ms
*/
void Task_Lcd(void const* pvParameters);

/**
* @brief  Setup LCD
*/
void Task_Lcd_Init(void);
