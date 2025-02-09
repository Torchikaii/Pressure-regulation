/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2022 STMicroelectronics.
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

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "mano.h"

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
ADC_HandleTypeDef hadc;

I2C_HandleTypeDef hi2c2;

SPI_HandleTypeDef hspi1;

UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */

int counter = 0;
int mux_counter = 0;

float targetPressure = 5.0;  // default pressure 5.0 bar
float currentPressure = 0.0;
uint8_t editMode = 1;  // start in edit mode
uint8_t flashState = 0;       // Used for flashing the display
uint32_t buttonPressTime[2] = {0, 0};
uint8_t buttonHeld[2] = {0, 0};
uint32_t lastUpdateTime = 0;  // Timer tracking for periodic updates

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_ADC_Init(void);
static void MX_I2C2_Init(void);
static void MX_SPI1_Init(void);
static void MX_USART2_UART_Init(void);
/* USER CODE BEGIN PFP */
void DisplayPressure(float pressure);
float ReadPressure(void);
void ControlRelay(float currentPressure, float targetPressure);
void HandleButtonPress(uint8_t button);
void CheckInputs(void);
void ProcessButtonPress(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin, uint8_t buttonIndex);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

void CheckInputs(void)
{
  // Read switch state
  if ((HAL_GPIO_ReadPin(GPIOB, SW1_Pin) == GPIO_PIN_RESET)) // Switch closed
  {
    editMode = 0;
  }
  else // Switch open
  {
    editMode = 1;
		//HAL_GPIO_TogglePin(RGB_LD1_GPIO_Port,RGB_LD1_Pin); // LED on
  }
	
	// Check button states for both buttons
  ProcessButtonPress(Button_1_Kamami_GPIO_Port, Button_1_Kamami_Pin, 0);
  ProcessButtonPress(Button_2_Kamami_GPIO_Port, Button_2_Kamami_Pin, 1);
}

#define Vref 3.3
#define Vstep Vref/4096 // 12 bit ADC
float ReadPressure(void)
{
	
	float pressure;
	uint32_t ADCpa0;
	float Vpa0;
	static unsigned char bADCReady=1;
	
	//if (HAL_ADCEx_Calibration_Start(&hadc, ADC_SINGLE_ENDED) != HAL_OK) {
//		Error_Handler();
//	}
	
	if (bADCReady==1) {
	// starting ADC
		if (HAL_ADC_Start(&hadc) != HAL_OK) {
			// Start Conversation Error 
			Error_Handler();
		} 
		else bADCReady=0; // ADC conversion in progress, you cannot start another ADC conversion
	}
	
	HAL_ADC_PollForConversion(&hadc, 10);

	// Check if the continous conversion of regular channel is finished
	if ((HAL_ADC_GetState(&hadc) & HAL_ADC_STATE_REG_EOC) == HAL_ADC_STATE_REG_EOC) {
 
		//##-6- Get the converted value of regular channel ########################
		ADCpa0 = HAL_ADC_GetValue(&hadc);// Read ADC result
		Vpa0=ADCpa0*Vstep; // calculate voltage
		//pressure = 3.8783 * Vpa0 - 1.2928;
		pressure = 2 * 3.8783 * Vpa0 - 1.2928;
		bADCReady=1; // you can start another ADC conversion
	}
 
	return pressure;
	
  //return ((adcValue * 3.3 / 4095.0 - 0.5) / 2.5) * 10.34214;
	//return ((adcValue * 3.3 / 4095.0 - 0.5) / 2.5) * 100.34214;
	
	//return 6.2;
}

void ProcessButtonPress(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin, uint8_t buttonIndex)
{
  if (HAL_GPIO_ReadPin(GPIOx, GPIO_Pin) == GPIO_PIN_RESET)
  {
    if (!buttonHeld[buttonIndex])
    {
      buttonPressTime[buttonIndex] = HAL_GetTick();
      buttonHeld[buttonIndex] = 1;
    }
    else
    {
      uint32_t pressDuration = HAL_GetTick() - buttonPressTime[buttonIndex];
      if (pressDuration > 700) // Long press
      {
        targetPressure += (buttonIndex == 0 ? 1.0 : -1.0);
        buttonPressTime[buttonIndex] = HAL_GetTick();
      }
    }
  }
  else if (buttonHeld[buttonIndex]) // Button released
  {
    uint32_t pressDuration = HAL_GetTick() - buttonPressTime[buttonIndex];
    if (pressDuration <= 700) // Short press
    {
      targetPressure += (buttonIndex == 0 ? 0.1 : -0.1);
    }
    buttonHeld[buttonIndex] = 0;
  }

  // Clamp target pressure
  if (targetPressure < 0)
    targetPressure = 0;
  if (targetPressure > 8.0)
    targetPressure = 8.0;
}


void DisplayPressure(float pressure)
{
	
	const uint32_t DECIMAL_POINT = DPseg; // Bitmask for decimal point segment
	
	const uint32_t digitPatterns[10] = {
    Aseg | Bseg | Cseg | Dseg | Eseg | Fseg,           // 0
    Bseg | Cseg,                                       // 1
    Aseg | Bseg | Dseg | Eseg | Gseg,                   // 2
    Aseg | Bseg | Cseg | Dseg | Gseg,                   // 3
    Bseg | Cseg | Fseg | Gseg,                         // 4
    Aseg | Cseg | Dseg | Fseg | Gseg,                   // 5
    Aseg | Cseg | Dseg | Eseg | Fseg | Gseg,            // 6
    Aseg | Bseg | Cseg,                                 // 7
    Aseg | Bseg | Cseg | Dseg | Eseg | Fseg | Gseg,      // 8
    Aseg | Bseg | Cseg | Dseg | Fseg | Gseg             // 9
	};
	
	/*
  if (pressure < 0) // Blank display
  {
    // Logic to turn off display
    GPIOC->ODR &= ~(COM1 | COM2 | COM3 | COM4);
    return;
  }
	*/
		//	float tmp_pressure = pressure; // temporary pressure value
  int integerPart = (int)pressure;
  int decimalPart = (int)((pressure - integerPart) * 10); //incorect ?

   // Extract individual digits
  int digits[4] = {
     integerPart,        // Units digit
     decimalPart,             // First decimal digit
     -1                       // Placeholder if needed for further extension
  };
	
	if (mux_counter == 6) GPIOC->ODR = digitPatterns[digits[0]] | DECIMAL_POINT | ((COM3) << 8);
	else if (mux_counter == 1) GPIOC->ODR = digitPatterns[digits[1]] | ((COM4) << 8);
	//GPIOC->ODR = digitPatterns[digits[1]] | ((COM3) << 8);
	//GPIOC->ODR = digitPatterns[digits[0]] | ((COM2) << 8);
	
	// Use the counter to select which digit to display
   
	
}


void ControlRelay(float currentPressure, float targetPressure) {
  if (currentPressure < targetPressure - 0.1)
  {
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_14, GPIO_PIN_SET); // Turn on relay
  }
  else if (currentPressure > targetPressure + 0.1)
  {
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_14, GPIO_PIN_RESET); // Turn off relay
  }
}
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
  MX_ADC_Init();
  MX_I2C2_Init();
  MX_SPI1_Init();
  MX_USART2_UART_Init();
  /* USER CODE BEGIN 2 */
  HAL_GPIO_WritePin(RGB_LD4_GPIO_Port,RGB_LD4_Pin,GPIO_PIN_RESET);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
	
  while (1)
  {
		
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */

		CheckInputs();
	
		if (!editMode) // We are in regulation mode
      {
        if (counter % 300 == 0) // Flash display at 5 Hz
        {
          flashState = !flashState;
        }

        if (flashState) DisplayPressure(currentPressure);
				else GPIOC->ODR = ~(COM1 | COM2 | COM3 | COM4);

        ControlRelay(currentPressure, targetPressure);
      }
      else // We are in edit mode
      {
        DisplayPressure(targetPressure);
      }
		
		//HAL_GPIO_WritePin(GPIOB, GPIO_PIN_14, GPIO_PIN_SET);	
		/*
		if (counter % 100 == 0)
		{
	
		//CheckInputs();
		HAL_GPIO_TogglePin(RGB_LD1_GPIO_Port,RGB_LD1_Pin); // LED on
		
		}
		*/
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
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_MSI;
  RCC_OscInitStruct.MSIState = RCC_MSI_ON;
  RCC_OscInitStruct.MSICalibrationValue = 0;
  RCC_OscInitStruct.MSIClockRange = RCC_MSIRANGE_5;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_MSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USART2;
  PeriphClkInit.Usart2ClockSelection = RCC_USART2CLKSOURCE_PCLK1;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief ADC Initialization Function
  * @param None
  * @retval None
  */
static void MX_ADC_Init(void)
{

  /* USER CODE BEGIN ADC_Init 0 */

  /* USER CODE END ADC_Init 0 */

  ADC_ChannelConfTypeDef sConfig = {0};

  /* USER CODE BEGIN ADC_Init 1 */

  /* USER CODE END ADC_Init 1 */

  /** Configure the global features of the ADC (Clock, Resolution, Data Alignment and number of conversion)
  */
  hadc.Instance = ADC1;
  hadc.Init.OversamplingMode = DISABLE;
  hadc.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV1;
  hadc.Init.Resolution = ADC_RESOLUTION_12B;
  hadc.Init.SamplingTime = ADC_SAMPLETIME_1CYCLE_5;
  hadc.Init.ScanConvMode = ADC_SCAN_DIRECTION_FORWARD;
  hadc.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc.Init.ContinuousConvMode = DISABLE;
  hadc.Init.DiscontinuousConvMode = DISABLE;
  hadc.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
  hadc.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc.Init.DMAContinuousRequests = DISABLE;
  hadc.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
  hadc.Init.Overrun = ADC_OVR_DATA_PRESERVED;
  hadc.Init.LowPowerAutoWait = DISABLE;
  hadc.Init.LowPowerFrequencyMode = ENABLE;
  hadc.Init.LowPowerAutoPowerOff = DISABLE;
  if (HAL_ADC_Init(&hadc) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure for the selected ADC regular channel to be converted.
  */
  sConfig.Channel = ADC_CHANNEL_0;
  sConfig.Rank = ADC_RANK_CHANNEL_NUMBER;
  if (HAL_ADC_ConfigChannel(&hadc, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN ADC_Init 2 */

  /* USER CODE END ADC_Init 2 */

}

/**
  * @brief I2C2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C2_Init(void)
{

  /* USER CODE BEGIN I2C2_Init 0 */

  /* USER CODE END I2C2_Init 0 */

  /* USER CODE BEGIN I2C2_Init 1 */

  /* USER CODE END I2C2_Init 1 */
  hi2c2.Instance = I2C2;
  hi2c2.Init.Timing = 0x00000708;
  hi2c2.Init.OwnAddress1 = 0;
  hi2c2.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c2.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c2.Init.OwnAddress2 = 0;
  hi2c2.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
  hi2c2.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c2.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c2) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Analogue filter
  */
  if (HAL_I2CEx_ConfigAnalogFilter(&hi2c2, I2C_ANALOGFILTER_ENABLE) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Digital filter
  */
  if (HAL_I2CEx_ConfigDigitalFilter(&hi2c2, 0) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C2_Init 2 */

  /* USER CODE END I2C2_Init 2 */

}

/**
  * @brief SPI1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI1_Init(void)
{

  /* USER CODE BEGIN SPI1_Init 0 */

  /* USER CODE END SPI1_Init 0 */

  /* USER CODE BEGIN SPI1_Init 1 */

  /* USER CODE END SPI1_Init 1 */
  /* SPI1 parameter configuration*/
  hspi1.Instance = SPI1;
  hspi1.Init.Mode = SPI_MODE_MASTER;
  hspi1.Init.Direction = SPI_DIRECTION_2LINES;
  hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi1.Init.NSS = SPI_NSS_SOFT;
  hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_2;
  hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi1.Init.CRCPolynomial = 7;
  if (HAL_SPI_Init(&hspi1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI1_Init 2 */

  /* USER CODE END SPI1_Init 2 */

}

/**
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART2_UART_Init(void)
{

  /* USER CODE BEGIN USART2_Init 0 */

  /* USER CODE END USART2_Init 0 */

  /* USER CODE BEGIN USART2_Init 1 */

  /* USER CODE END USART2_Init 1 */
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 115200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  huart2.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart2.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
/* USER CODE BEGIN MX_GPIO_Init_1 */
/* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOC, Seg_G_Pin|Seg_D_Pin|Seg_E_Pin|Seg_C_Pin
                          |Seg_B_Pin|Seg_F_Pin|Seg_A_Pin|Decimal_Point_Pin
                          |COM4_Pin|COM3_Pin|COM2_Pin|COM1_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_14|GPIO_OutPB15_Pin|DS18B20_Pin_Pin|RGB_LD4_Pin
                          |RGB_LD1_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : B1_Pin */
  GPIO_InitStruct.Pin = B1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(B1_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : Seg_G_Pin Seg_D_Pin Seg_E_Pin Seg_C_Pin
                           Seg_B_Pin Seg_F_Pin Seg_A_Pin Decimal_Point_Pin
                           COM4_Pin COM3_Pin COM2_Pin COM1_Pin */
  GPIO_InitStruct.Pin = Seg_G_Pin|Seg_D_Pin|Seg_E_Pin|Seg_C_Pin
                          |Seg_B_Pin|Seg_F_Pin|Seg_A_Pin|Decimal_Point_Pin
                          |COM4_Pin|COM3_Pin|COM2_Pin|COM1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pin : LD2_Pin */
  GPIO_InitStruct.Pin = LD2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LD2_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : Button_1_Kamami_Pin SW1_Pin Button_2_Kamami_Pin */
  GPIO_InitStruct.Pin = Button_1_Kamami_Pin|SW1_Pin|Button_2_Kamami_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pins : PB14 GPIO_OutPB15_Pin DS18B20_Pin_Pin RGB_LD4_Pin
                           RGB_LD1_Pin */
  GPIO_InitStruct.Pin = GPIO_PIN_14|GPIO_OutPB15_Pin|DS18B20_Pin_Pin|RGB_LD4_Pin
                          |RGB_LD1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pin : INT1_LIS35DE_Pin */
  GPIO_InitStruct.Pin = INT1_LIS35DE_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(INT1_LIS35DE_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : RGB_LD3_Pin RGB_LD2_Pin */
  GPIO_InitStruct.Pin = RGB_LD3_Pin|RGB_LD2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI4_15_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI4_15_IRQn);

/* USER CODE BEGIN MX_GPIO_Init_2 */
/* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

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
