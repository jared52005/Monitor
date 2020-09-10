/*******************************************************************************
 * @file    Module_Lcd.h
 * @author  Jaromir Mayer
 * @brief   Module controlling generic LCD display, which can write lines
 ******************************************************************************
 * @attention
 ******************************************************************************  
 */ 

#include <stdint.h>

/**
* @brief  Task for printing data on LCD. Low prioririty task. Run only once in 100ms
*/
void Application_Lcd_Task(void const* pvParameters);

/**
* @brief  Setup LCD
*/
void Application_Lcd_Init(void);
