/*******************************************************************************
 * @brief   Init user (key) button to generate IRQ via ETXI15. Button is then
 *          linked to Task_Hub to change the baudrate.
 ******************************************************************************
 * @attention
 ******************************************************************************  
 */ 

#include <stdint.h>
#include <stdio.h>

/**
 * @brief Init User (Key) button to EXTI15
 */
void ButtonIf_Init(void);
