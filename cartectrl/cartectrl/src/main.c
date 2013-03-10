#include <stm32f4xx.h>
#include <stm32f4xx_gpio.h>
#include <stm32f4xx_flash.h>
#include <stm32f4xx_rcc.h>
#include <misc.h>
#include "usbd_cdc_core.h"
#include "usbd_usr.h"
#include "usbd_desc.h"
#include "usbd_cdc_vcp.h"

#include "stm32f4xx_conf.h"


USB_OTG_CORE_HANDLE  USB_OTG_dev __attribute__ ((aligned (4)));

void Delay(volatile uint32_t nCount) {
  while(nCount--) {
  }
}

#define DELAYELEM 2000000L

/* This funcion shows how to initialize
 * the GPIO pins on GPIOD and how to configure
 * them as inputs and outputs
 */
void init_GPIO(void){

	/* This TypeDef is a structure defined in the
	 * ST's library and it contains all the properties
	 * the corresponding peripheral has, such as output mode,
	 * pullup / pulldown resistors etc.
	 *
	 * These structures are defined for every peripheral so
	 * every peripheral has it's own TypeDef. The good news is
	 * they always work the same so once you've got a hang
	 * of it you can initialize any peripheral.
	 *
	 * The properties of the periperals can be found in the corresponding
	 * header file e.g. stm32f4xx_gpio.h and the source file stm32f4xx_gpio.c
	 */
	GPIO_InitTypeDef GPIO_InitStruct;

	/* This enables the peripheral clock to the GPIOD IO module
	 * Every peripheral's clock has to be enabled
	 *
	 * The STM32F4 Discovery's User Manual and the STM32F407VGT6's
	 * datasheet contain the information which peripheral clock has to be used.
	 *
	 * It is also mentioned at the beginning of the peripheral library's
	 * source file, e.g. stm32f4xx_gpio.c
	 */
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOD, ENABLE);

	/* In this block of instructions all the properties
	 * of the peripheral, the GPIO port in this case,
	 * are filled with actual information and then
	 * given to the Init function which takes care of
	 * the low level stuff (setting the correct bits in the
	 * peripheral's control register)
	 *
	 *
	 * The LEDs on the STM324F Discovery are connected to the
	 * pins PD12 thru PD15
	 */
	GPIO_InitStruct.GPIO_Pin = GPIO_Pin_15 | GPIO_Pin_14 | GPIO_Pin_13 | GPIO_Pin_12; // we want to configure all LED GPIO pins
	GPIO_InitStruct.GPIO_Mode = GPIO_Mode_OUT; 		// we want the pins to be an output
	GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz; 	// this sets the GPIO modules clock speed
	GPIO_InitStruct.GPIO_OType = GPIO_OType_PP; 	// this sets the pin type to push / pull (as opposed to open drain)
	GPIO_InitStruct.GPIO_PuPd = GPIO_PuPd_NOPULL; 	// this sets the pullup / pulldown resistors to be inactive
	GPIO_Init(GPIOD, &GPIO_InitStruct); 			// this finally passes all the values to the GPIO_Init function which takes care of setting the corresponding bits.

	/* This enables the peripheral clock to
	 * the GPIOA IO module
	 */
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);

	/* Here the GPIOA module is initialized.
	 * We want to use PA0 as an input because
	 * the USER button on the board is connected
	 * between this pin and VCC.
	 */
	GPIO_InitStruct.GPIO_Pin = GPIO_Pin_0;		  // we want to configure PA0
	GPIO_InitStruct.GPIO_Mode = GPIO_Mode_IN; 	  // we want it to be an input
	GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;//this sets the GPIO modules clock speed
	GPIO_InitStruct.GPIO_OType = GPIO_OType_PP;   // this sets the pin type to push / pull (as opposed to open drain)
	GPIO_InitStruct.GPIO_PuPd = GPIO_PuPd_DOWN;   // this enables the pulldown resistor --> we want to detect a high level
	GPIO_Init(GPIOA, &GPIO_InitStruct);			  // this passes the configuration to the Init function which takes care of the low level stuff
}

void initClock(void)
{
  /* SYSCLK, HCLK, PCLK configuration ----------------------------------------*/
  /* At this stage the HSI is already enabled */

  /* Enable Prefetch Buffer and set Flash Latency */
  #if 0
  FLASH_SetLatency(FLASH_Latency_5);
  FLASH_PrefetchBufferCmd(ENABLE);

  FLASH_InstructionCacheCmd(ENABLE);
  FLASH_DataCacheCmd(ENABLE);

  RCC_APB1PeriphClockCmd(RCC_APB1ENR_PWREN, ENABLE);
  RCC_APB1PeriphResetCmd(RCC_APB1ENR_PWREN, DISABLE);
  PWR_MainRegulatorModeConfig(PWR_Regulator_Voltage_Scale1);

  RCC_DeInit();
  RCC_HSEConfig(RCC_HSE_ON);
  RCC_WaitForHSEStartUp(); // NOTE: return value discarded!
  RCC_PLLConfig(RCC_PLLSource_HSE, 8, 336, 2, 7);
  RCC_PLLCmd(ENABLE);
  RCC_HCLKConfig(RCC_HCLK_Div1);   // APB1 = 42MHz
  RCC_PCLK1Config(RCC_HCLK_Div4);   // APB1 = 42MHz
  RCC_PCLK2Config(RCC_HCLK_Div2);   // APB2 = 84MHz

  RCC_SYSCLKConfig(RCC_SYSCLKSource_PLLCLK);

  #elseif 0
  RCC->CFGR = 0x00009400;

  RCC->PLLCFGR = 0x04005410;
  /* Enable PLL */
  RCC->CR |= RCC_CR_PLLON;


  /* Wait till PLL is ready */
  while((RCC->CR & RCC_CR_PLLRDY) == 0)
  {
  }

  /* Select PLL as system clock source */
  RCC->CFGR &= (uint32_t)((uint32_t)~(RCC_CFGR_SW));
  RCC->CFGR |= (uint32_t)RCC_CFGR_SW_PLL;

  /* Wait till PLL is used as system clock source */
  while ((RCC->CFGR & (uint32_t)RCC_CFGR_SWS) != (uint32_t)RCC_CFGR_SWS_PLL)
  {
  }
  #else

  /* PLL_VCO = (HSE_VALUE or HSI_VALUE / PLL_M) * PLL_N */
#define PLL_M      8
#define PLL_N      336

/* SYSCLK = PLL_VCO / PLL_P */
#define PLL_P      2

/* USB OTG FS, SDIO and RNG Clock =  PLL_VCO / PLLQ */
#define PLL_Q      7

/* PLLI2S_VCO = (HSE_VALUE Or HSI_VALUE / PLL_M) * PLLI2S_N
   I2SCLK = PLLI2S_VCO / PLLI2S_R */
#define PLLI2S_N   192
#define PLLI2S_R   5

   /* Reset the RCC clock configuration to the default reset state ------------*/
  /* Set HSION bit */
  RCC->CR |= (uint32_t)0x00000001;

  /* Reset CFGR register */
  RCC->CFGR = 0x00000000;

  /* Reset HSEON, CSSON and PLLON bits */
  RCC->CR &= (uint32_t)0xFEF6FFFF;

  /* Reset PLLCFGR register */
  RCC->PLLCFGR = 0x24003010;

  /* Reset HSEBYP bit */
  RCC->CR &= (uint32_t)0xFFFBFFFF;

  /* Disable all interrupts */
  RCC->CIR = 0x00000000;

    __IO uint32_t StartUpCounter = 0, HSEStatus = 0;

  /* Enable HSE */
  RCC->CR |= ((uint32_t)RCC_CR_HSEON);

  /* Wait till HSE is ready and if Time out is reached exit */
  do
  {
    HSEStatus = RCC->CR & RCC_CR_HSERDY;
    StartUpCounter++;
  } while((HSEStatus == 0) && (StartUpCounter != HSE_STARTUP_TIMEOUT));

  if ((RCC->CR & RCC_CR_HSERDY) != RESET)
  {
    HSEStatus = (uint32_t)0x01;
  }
  else
  {
    HSEStatus = (uint32_t)0x00;
  }

  if (HSEStatus == (uint32_t)0x01)
  {
    /* Enable high performance mode, System frequency up to 168 MHz */
    RCC->APB1ENR |= RCC_APB1ENR_PWREN;
    PWR->CR |= PWR_CR_PMODE;

    /* HCLK = SYSCLK / 1*/
    RCC->CFGR |= RCC_CFGR_HPRE_DIV1;

    /* PCLK2 = HCLK / 2*/
    RCC->CFGR |= RCC_CFGR_PPRE2_DIV2;

    /* PCLK1 = HCLK / 4*/
    RCC->CFGR |= RCC_CFGR_PPRE1_DIV4;

    /* Configure the main PLL */
    RCC->PLLCFGR = PLL_M | (PLL_N << 6) | (((PLL_P >> 1) -1) << 16) |
                   (RCC_PLLCFGR_PLLSRC_HSE) | (PLL_Q << 24);

    /* Enable the main PLL */
    RCC->CR |= RCC_CR_PLLON;

    /* Wait till the main PLL is ready */
    while((RCC->CR & RCC_CR_PLLRDY) == 0)
    {
    }

    /* Configure Flash prefetch, Instruction cache, Data cache and wait state */
    FLASH->ACR = FLASH_ACR_ICEN |FLASH_ACR_DCEN |FLASH_ACR_LATENCY_5WS;

    /* Select the main PLL as system clock source */
    RCC->CFGR &= (uint32_t)((uint32_t)~(RCC_CFGR_SW));
    RCC->CFGR |= RCC_CFGR_SW_PLL;

    /* Wait till the main PLL is used as system clock source */
    while ((RCC->CFGR & (uint32_t)RCC_CFGR_SWS ) != RCC_CFGR_SWS_PLL);
    {
    }
  }
  else
  { /* If HSE fails to start-up, the application will have wrong clock
         configuration. User can add here some code to deal with this error */
  }

  #endif
}

int main(void){

  // initialize the GPIO pins we need
  initClock();
  init_GPIO();
  int len;
  uint8_t buf[255];
  /* This flashed the LEDs on the board once
   * Two registers are used to set the pins (pin level is VCC)
   * or to reset the pins (pin level is GND)
   *
   * BSRR stands for bit set/reset register
   * it is seperated into a high and a low word (each of 16 bit size)
   *
   * A logical 1 in BSRRL will set the pin and a logical 1 in BSRRH will
   * reset the pin. A logical 0 in either register has no effect
   */
  int i=0;
  for (;i<4;++i)
  {
      GPIOD->BSRRL = 0xF000; // set PD12 thru PD15
      Delay(10000000L);		 // wait a short period of time
      GPIOD->BSRRH = 0xF000; // reset PD12 thru PD15
      Delay(10000000L);
  }
//*
   USBD_Init(&USB_OTG_dev,
            USB_OTG_FS_CORE_ID,
            &USR_desc,
            &USBD_CDC_cb,
            &USR_cb);
            //*/
  GPIOD->BSRRL = 0x2000;

  //int x = 0;
  for (;;)
  {
    len = VCP_get_string(&buf[0]);
		if(len)
		    VCP_send_str(&buf[0]);
     // GPIOD->ODR ^= 0x8000;
      //while (x++ < 1000000);
      //x=0;
  }
/*
		GPIOD->BSRRL = 0x1000; // this sets LED1 (green)
		Delay(DELAYELEM);
		GPIOD->BSRRL = 0x2000; // this sets LED2 (orange)
		Delay(DELAYELEM);
		GPIOD->BSRRL = 0x4000; // this sets LED3 (red)
		Delay(DELAYELEM);
		GPIOD->BSRRL = 0x8000; // this sets LED4
		Delay(DELAYELEM);

		GPIOD->BSRRH = 0x1000; // this resets LED1
		Delay(DELAYELEM);
		GPIOD->BSRRH = 0x2000; // this resets LED2
		Delay(DELAYELEM);
		GPIOD->BSRRH = 0x4000; // this resets LED3
		Delay(DELAYELEM);
		GPIOD->BSRRH = 0x8000; // this resets LED4 (blue)
		Delay(DELAYELEM);
	}
	*/
}
