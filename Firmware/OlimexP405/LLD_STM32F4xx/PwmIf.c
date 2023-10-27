#include "stm32f4xx.h"
#include "stm32f4xx_rcc.h"
#include "stm32f4xx_gpio.h"
#include "stm32f4xx_tim.h"
#include "PwmIf.h"

/*Private variables----------------------------------------------------------*/
static TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
static TIM_OCInitTypeDef  TIM_OCInitStructure;
static TIM_BDTRInitTypeDef TIM_BDTRInitStructure;
static uint16_t TimerPeriod = 0;
static uint16_t ChannelPulse = 0, Channe2Pulse = 0;

/*Private methods------------------------------------------------------------*/
void TIM1_Init(int freq, int ch1_dc, int ch2_dc);
void TIM1_GPIO_Init(void);
void TIM1_Enable(void);
void TIM1_Disable(void);

/**
  * @brief  Configures GPIO pins for TIM1
  * @param  None
  * @retval None
  */
void TIM1_GPIO_Init(void) 
{
   GPIO_InitTypeDef GPIO_InitStructure;

   /* GPIOA and GPIOB clocks enable */
   RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);

   /* TIM1 clock enable */
   RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM1, ENABLE);
                         
   /* GPIOB Configuration: BKIN, Channel 1N, 2N and 3N as alternate function push-pull */
   GPIO_InitStructure.GPIO_Pin = GPIO_Pin_13 | GPIO_Pin_14;
   GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
   GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
   GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
   GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
   GPIO_Init(GPIOB, &GPIO_InitStructure);
  
   /* Connect TIM pins to AF1 */
   GPIO_PinAFConfig(GPIOB, GPIO_PinSource13, GPIO_AF_TIM1);
   GPIO_PinAFConfig(GPIOB, GPIO_PinSource14, GPIO_AF_TIM1); 
}
/**
* @brief  Enable PWM channel on given frequency and duty cycle
* @param  name: Name of channel which we want to enable
* @param  freq: Frequency in [Hz] which we want to run a PWM channel
* @param  dc: Duty cycle of channel. i.e. 50 = 50.0 percent
*/
void Pwm_Enable(PwmName name, uint32_t freq, uint32_t dc1, uint32_t dc2)
{
	//On STM32 each channel can have different duty cycle and shared frequency.
	//However my target is to have PWM channel with two outputs, 
	//shared frequency and shared duty cycle on both outputs.
	//Check if duty cycle is not out of range
	if(dc1 > 1000 || dc2 > 1000)
	{
		return;
	}
	//dc1 *= 10; //Duty cycle in driver is expected as 500 = 50.0 percent
	//dc2 *= 10; //Duty cycle in driver is expected as 500 = 50.0 percent
	
	switch(name)
	{
		case PWM_CHANNEL_01:
		  // Maximal frequency on TIM1 (Channel 01) = 1MHz
		  // Then signal is too ugly to be used for anything, but it would be able to go probably much higher
		  // Minimal frequency on TIM1 (Channel 01) = 4100Hz
		  // After some software changes can be even a few Hz if necessary
		  TIM1_GPIO_Init();
		  TIM1_Init(freq, dc1, dc2);
		  TIM1_Enable();
		  break;
		default:
			break;
	}
}

/**
  * @brief  Configures TIM1
  * @param  freq: Frequency in Hz
  *         ch1_dc: Duty cycle of channel. i.e. 500 = 50.0 percent
  *         ch2_dc: Duty cycle of channel. i.e. 500 = 50.0 percent
  * @retval None
  */
void TIM1_Init(int freq, int ch1_dc, int ch2_dc) 
{
  /* -----------------------------------------------------------------------
  1/ Generate 3 complementary PWM signals with 3 different duty cycles:
    
    In this example TIM1 input clock (TIM1CLK) is set to 2 * APB2 clock (PCLK2), 
    since APB2 prescaler is different from 1 (APB2 Prescaler = 2, see system_stm32f4xx.c file).
      TIM1CLK = 2 * PCLK2  
      PCLK2 = HCLK / 2 
      => TIM1CLK = 2*(HCLK / 2) = HCLK = SystemCoreClock
         
    To get TIM1 counter clock at 168 MHz, the prescaler is computed as follows:
       Prescaler = (TIM1CLK / TIM1 counter clock) - 1
       Prescaler = (SystemCoreClock / 168 MHz) - 1 = 0
  
    The objective is to generate PWM signal at 17.57 KHz:
    - TIM1_Period = (SystemCoreClock / 17570) - 1
    To get TIM1 output clock at 17.57 KHz, the period (ARR) is computed as follows:
       ARR = (TIM1 counter clock / TIM1 output clock) - 1
           = 9561
    
  The Three Duty cycles are computed as the following description: 
    TIM1 Channel1 duty cycle = (TIM1_CCR1/ TIM1_ARR)* 100 = 50%
    TIM1 Channel2 duty cycle = (TIM1_CCR2/ TIM1_ARR)* 100 = 25%
    TIM1 Channel3 duty cycle = (TIM1_CCR3/ TIM1_ARR)* 100 = 12.5%
    
    The Timer pulse is calculated as follows:
      - TIM1_CCRx = (DutyCycle * TIM1_ARR)/ 100
    
  2/ Insert a dead time equal to (11/SystemCoreClock) ns
  3/ Configure the break feature, active at High level, and using the automatic 
     output enable feature
  4/ Use the Locking parameters level1.
  
    Note: 
     SystemCoreClock variable holds HCLK frequency and is defined in system_stm32f4xx.c file.
     Each time the core clock (HCLK) changes, user had to call SystemCoreClockUpdate()
     function to update SystemCoreClock variable value. Otherwise, any configuration
     based on this variable will be incorrect.    
  ----------------------------------------------------------------------- */  
  
  /* Compute the value to be set in ARR register to generate signal frequency at "freq" Khz */
  uint16_t prescaler = 0;
  if(freq < 5000)
  {
    //TIM_Period is 16 bit value, so freq < 2600Hz would overflow it. Use prescaller
    prescaler = 64;
    freq = freq * prescaler;
  }
  TimerPeriod = (SystemCoreClock / freq) - 1;

  /* Compute CCR2 value to generate a duty cycle for channel 2 */
  ChannelPulse = (uint16_t) (((uint32_t) ch1_dc * (TimerPeriod - 1)) / 1000);

  /* Compute CCR2 value to generate a duty cycle for channel 3 */
  Channe2Pulse = (uint16_t) (((uint32_t) ch2_dc * (TimerPeriod - 1)) / 1000);

  /* Time Base configuration */
  TIM_TimeBaseStructure.TIM_Prescaler = prescaler;
  TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Down;
  TIM_TimeBaseStructure.TIM_Period = TimerPeriod;
  TIM_TimeBaseStructure.TIM_ClockDivision = 0;
  TIM_TimeBaseStructure.TIM_RepetitionCounter = 0;

  TIM_TimeBaseInit(TIM1, &TIM_TimeBaseStructure);

  /* Channel 1, 2 and 3 Configuration in PWM mode */
  TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM2;
  TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
  TIM_OCInitStructure.TIM_OutputNState = TIM_OutputNState_Enable;
  TIM_OCInitStructure.TIM_Pulse = ChannelPulse;
  TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_Low;
  TIM_OCInitStructure.TIM_OCNPolarity = TIM_OCNPolarity_Low;
  TIM_OCInitStructure.TIM_OCIdleState = TIM_OCIdleState_Set;
  TIM_OCInitStructure.TIM_OCNIdleState = TIM_OCIdleState_Reset;

  TIM_OC1Init(TIM1, &TIM_OCInitStructure);

  TIM_OCInitStructure.TIM_Pulse = Channe2Pulse;
  TIM_OC2Init(TIM1, &TIM_OCInitStructure);

  /* Automatic Output enable, Break, dead time and lock configuration*/
  TIM_BDTRInitStructure.TIM_OSSRState = TIM_OSSRState_Enable;
  TIM_BDTRInitStructure.TIM_OSSIState = TIM_OSSIState_Enable;
  TIM_BDTRInitStructure.TIM_LOCKLevel = TIM_LOCKLevel_1;
  TIM_BDTRInitStructure.TIM_DeadTime = 11;
  TIM_BDTRInitStructure.TIM_Break = TIM_Break_Enable;
  TIM_BDTRInitStructure.TIM_BreakPolarity = TIM_BreakPolarity_High;
  TIM_BDTRInitStructure.TIM_AutomaticOutput = TIM_AutomaticOutput_Enable;

  TIM_BDTRConfig(TIM1, &TIM_BDTRInitStructure);
}

/**
  * @brief  Enables TIM1
  * @param  None
  * @retval None
  */
void TIM1_Enable(void)
{
   /* TIM1 counter enable */
   TIM_Cmd(TIM1, ENABLE);

   /* Main Output Enable */
   TIM_CtrlPWMOutputs(TIM1, ENABLE);
}

/**
  * @brief  Disables TIM1
  * @param  None
  * @retval None
  */
void TIM1_Disable(void)
{
	 /* TIM1 counter enable */
   TIM_Cmd(TIM1, DISABLE);

   /* Main Output Enable */
   TIM_CtrlPWMOutputs(TIM1, DISABLE);
}
