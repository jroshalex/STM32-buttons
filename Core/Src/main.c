/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "i2c.h"
#include "spi.h"
#include "usb_device.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
volatile uint8_t btnPressed = 255;   // 0–4 = button index, 255 = none
volatile uint32_t lastPressTime = 0;

// We use volatile here because variable can be modified by external hardware or interrupts, and you don’t want the compiler to optimize out the read or write
// variable is accessed both in the main code and in an interrupt handler (ISR) -- interrupt handler modifies the value, and the main loop needs to react to those changes
// “Hey, this variable can change unexpectedly — it can change in an interrupt! Don’t optimize it away or assume it’s always the same!”

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_I2C1_Init();
  MX_SPI1_Init();
  MX_USB_DEVICE_Init();
  /* USER CODE BEGIN 2 */

  // --- Enable EXTI interrupts for PE2..PE6 manually ---
  // Important: assumes each button pin is EXTI-capable.

  // PE2
  HAL_NVIC_SetPriority(EXTI2_IRQn, 0, 0);	// HAL_NVIC_SetPriority(IRQn, preemption, subpriority) --> higher # = lower priority -- 0 = highest priority... idk preemption
  // ^^^ This determines which interrupt can interrupt another one.
  HAL_NVIC_EnableIRQ(EXTI2_IRQn);	// This unmasks the interrupt in the NVIC so the CPU can receive it -- If you do not call this, EXTI interrupts will NEVER run, even if the pin is configured correctly

  // PE3
  HAL_NVIC_SetPriority(EXTI3_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI3_IRQn);

  // PE4
  HAL_NVIC_SetPriority(EXTI4_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI4_IRQn);

  // EXTI9_5 handles pins 5 and 6
  HAL_NVIC_SetPriority(EXTI9_5_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI9_5_IRQn);

  extern USBD_HandleTypeDef hUsbDeviceFS;
  typedef struct
  {
  	uint8_t MODIFIER;
  	uint8_t RESERVED;
  	uint8_t KEYCODE1;
  	uint8_t KEYCODE2;
  	uint8_t KEYCODE3;
  	uint8_t KEYCODE4;
  	uint8_t KEYCODE5;
  	uint8_t KEYCODE6;
  } keyboardHID;

  keyboardHID release = {0};

  const uint16_t leds[] = {
  	      GPIO_PIN_1,   // white
  	      GPIO_PIN_0,   // green
  	      GPIO_PIN_2,   // yellow
  	      GPIO_PIN_3,   // red
		  GPIO_PIN_4	// blue
  	  };

  int index = 0;
  int direction = 1;  // 1 = forward, -1 = backward
  uint32_t lastPressTime = 0;   // stores time of last button press


// DON'T NEED THIS SHII IN INTERRUPT CODE ANYMORE HAHAHAHHAHAHA-->

//  const uint16_t btns[] = {
//		  GPIO_PIN_2,   // white = copy = ctrl + C
//		  GPIO_PIN_3,   // green = paste = ctrl + V
//		  GPIO_PIN_4,	// yellow = "ur gay"
//		  GPIO_PIN_5,	// red = change tab = ALT + TAB
//		  GPIO_PIN_6	// blue = print screen
//  };

  keyboardHID shortcuts[5] = {
		  {0x01,0,0x06,0,0,0,0,0},		// white
		  {0x01,0,0x19,0,0,0,0,0},		// green
		  {0,0,0x18,0x15,0x0A,0x04,0x1C, 0x2C},		// yellow
		  {0x04,0,0x2B,0,0,0,0,0},		// red
		  {0,0,0x46,0,0,0,0,0}		// blue
  };     // HID keyboard report: [mods, reserved, key1..key6]


  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {

	  // ===================== BUTTON CHECK ===================== //
	  if (btnPressed != 255)
	  {
	      uint8_t id = btnPressed;
	      btnPressed = 255;

	      HAL_GPIO_WritePin(GPIOA, leds[id], GPIO_PIN_SET);

	      USBD_HID_SendReport(&hUsbDeviceFS, (uint8_t*)&shortcuts[id], sizeof(release));
	      HAL_Delay(50);

	      USBD_HID_SendReport(&hUsbDeviceFS, &release, sizeof(release));
	      HAL_Delay(150);

	      HAL_GPIO_WritePin(GPIOA, leds[id], GPIO_PIN_RESET);
	  }


	  // LOADING...

	  if (HAL_GetTick() - lastPressTime > 2000)  // 2000 ms = 2 seconds
	      {
	          HAL_GPIO_WritePin(GPIOA, leds[index], GPIO_PIN_SET);
	          HAL_Delay(150);
	          HAL_GPIO_WritePin(GPIOA, leds[index], GPIO_PIN_RESET);

	          index += direction;

	          if (index == 5 - 1) direction = -1;
	          if (index == 0) direction = 1;
	      }


    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 8;
  RCC_OscInitStruct.PLL.PLLN = 336;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 7;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    static uint32_t lastInterruptTime = 0;
    uint32_t currentTime = HAL_GetTick();

    // Debounce: ignore interrupts that occur within 200 ms of the last one
    if (currentTime - lastInterruptTime < 200) {
        return; // Debounce delay: ignore this interrupt
    }

    // Map pin → button index
    if (GPIO_Pin == GPIO_PIN_2) btnPressed = 0;
    else if (GPIO_Pin == GPIO_PIN_3) btnPressed = 1;
    else if (GPIO_Pin == GPIO_PIN_4) btnPressed = 2;
    else if (GPIO_Pin == GPIO_PIN_5) btnPressed = 3;
    else if (GPIO_Pin == GPIO_PIN_6) btnPressed = 4;

    lastPressTime = currentTime;  // Update last press time
    lastInterruptTime = currentTime;  // Update the time of this interrupt
}

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
