/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2024 STMicroelectronics.
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
#include <stdio.h>

#include "ssd1306.h"
#include "ssd1306_fonts.h"

#include "ring_buffer.h"
#include "keypad.h"
#define DEBOUNCE_TIME 200
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
I2C_HandleTypeDef hi2c1;

UART_HandleTypeDef huart2;
UART_HandleTypeDef huart3;

/* USER CODE BEGIN PV */

#define USART2_BUFFER_SIZE 8
uint8_t usart2_buffer[USART2_BUFFER_SIZE];
ring_buffer_t usart2_rb;
uint8_t usart2_rx;

uint32_t left_toggles = 0;
uint32_t left_last_press_tick = 0;
uint16_t right_toggles=0;
uint32_t counter_left=0;
uint32_t last_debounce_time_left = 0;
uint32_t last_debounce_time_right = 0,last_debounce_time_hazard = 0;
uint32_t counter_right=0,counter_hazard=0;
uint32_t last_debounce_time_parking=0,counter_parking=0,parking_toggle=0;


/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_I2C1_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_USART3_UART_Init(void);
/* USER CODE BEGIN PFP */


/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
int _write(int file, char *ptr, int len)
{
  HAL_UART_Transmit(&huart2, (uint8_t *)ptr, len, 10);
  return len;
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
  /* Data received in USART2 */
  if (huart->Instance == USART2) {
	  usart2_rx = USART2->RDR; // leyendo el byte recibido de USART2
	  ring_buffer_write(&usart2_rb, usart2_rx); // put the data received in buffer
	  //HAL_UART_Receive_IT(&huart2, &usart2_rx, 1); // enable interrupt to continue receiving
	  ATOMIC_SET_BIT(USART2->CR1, USART_CR1_RXNEIE); // usando un funcion mas liviana para reducir memoria
  }
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
	uint32_t current_time = HAL_GetTick();
	//uint8_t key_pressed = keypad_scan(GPIO_Pin);

	 if (GPIO_Pin == S1_Pin){

			if (current_time - last_debounce_time_left >= DEBOUNCE_TIME) {

				  // Tiempo de reset de 1 segundo, exeptuando el reset despues de dos pulsasiones.
		        if (current_time - last_debounce_time_left > 1000 && counter_left < 2) {
		             counter_left = 0;
		         }



		// se inicia el contador de pulsos
			    counter_left++;
			    last_debounce_time_left = current_time;

			    if(counter_left==1){
			    	right_toggles=0;
			    	counter_right=0;



	        	    HAL_UART_Transmit(&huart2, "S1\r\n", 4, 10);
			    	left_toggles = 6;
					ssd1306_Fill(Black);
					ssd1306_SetCursor(25, 30);
					ssd1306_WriteString("left Light ON", Font_7x10, White);
					ssd1306_UpdateScreen();

			    	}
			    else if(counter_left==2){

			    	HAL_UART_Transmit(&huart2, "S1_toggles\r\n",12, 10);
			    	left_toggles = 0xEEEEEEE;  // Contador muy grande, hace que haya un parpadeo practicamente infito.

					ssd1306_Fill(Black);
					ssd1306_SetCursor(25, 30);
					ssd1306_WriteString("left Light ON", Font_7x10, White);
					ssd1306_UpdateScreen();



			    	}
			     else if (counter_left>=3){
			    	 // si hay una tercera pulsasion se resetean los contadores (se apaga el led)
			    	 HAL_UART_Transmit(&huart2, "S1_off\r\n",8, 10);
			    	 counter_left=0;
			    	 left_toggles = 0;
					  ssd1306_Fill(Black);
					  ssd1306_SetCursor(25, 30);
					  ssd1306_WriteString("left Light OFF", Font_7x10, White);
					  ssd1306_UpdateScreen();

			    	}




			  }

	 }

	 if (GPIO_Pin == S2_Pin){

			if (current_time - last_debounce_time_left >= DEBOUNCE_TIME) {

				  // Tiempo de reset de 1 segundo, exeptuando el reset despues de dos pulsasiones.
		        if (current_time - last_debounce_time_right > 1000 && counter_right < 2) {
		             counter_right = 0;
		         }



		// se inicia el contador de pulsos
			    counter_right++;
			    last_debounce_time_right = current_time;

			    if(counter_right==1){
			    	left_toggles=0;
			    	counter_left=0;

	        	    HAL_UART_Transmit(&huart2, "S2\r\n", 4, 10);
			    	right_toggles = 6;

					ssd1306_Fill(Black);
					ssd1306_SetCursor(25, 10);
					ssd1306_WriteString("right Light ON", Font_7x10, White);
					ssd1306_UpdateScreen();
			    	}
			    else if(counter_right==2){

			    	HAL_UART_Transmit(&huart2, "S2_toggles\r\n",12, 10);
			    	right_toggles = 0xEEEEEEE;  // Contador muy grande, hace que haya un parpadeo practicamente infito.
			    	ssd1306_Fill(Black);
			    	ssd1306_SetCursor(25, 10);
			    	ssd1306_WriteString("right Light ON", Font_7x10, White);
			    	ssd1306_UpdateScreen();



			    	}
			     else if (counter_right>=3){

			    	 // si hay una tercera pulsasion se resetean los contadores (se apaga el led)
			    	 HAL_UART_Transmit(&huart2, "S2_off\r\n",8, 10);
			    	 counter_right=0;
			    	 right_toggles = 0;
				     ssd1306_Fill(Black);
				     ssd1306_SetCursor(25, 10);
				     ssd1306_WriteString("right Light OFF", Font_7x10, White);
				     ssd1306_UpdateScreen();

			    	}


			  }

	 }

	 if (GPIO_Pin == S3_Pin){

			if (current_time - last_debounce_time_parking >= DEBOUNCE_TIME) {

				  // Tiempo de reset de 1 segundo, exeptuando el reset despues de dos pulsasiones.
		        if (current_time - last_debounce_time_parking > 1000 && counter_parking < 2) {
		        	counter_parking=0;
		         }





		// se inicia el contador de pulsos
			    counter_parking++;
			    last_debounce_time_parking = current_time;

			    if(counter_parking==1){

	        	    HAL_UART_Transmit(&huart2, "parking_on\r\n", 12, 10);
	        	    parking_toggle = 0xEEEEE;
			    	}

			     else if (counter_parking>=2){
			    	 // si hay una tercera pulsasion se resetean los contadores (se apaga el led)
			    	 HAL_UART_Transmit(&huart2, "parking_off\r\n",13, 10);
			    	 counter_right=0;
			    	 counter_left=0;
			    	 parking_toggle = 0;

			    	}
			  }
	 }
}


void heartbeat(void)
{
	static uint32_t heartbeat_tick = 0;
	if (heartbeat_tick < HAL_GetTick()){
		heartbeat_tick = HAL_GetTick()+ 500;
		HAL_GPIO_TogglePin(D1_GPIO_Port, D1_Pin);
	}
}
void turn_signal_left(void)
{
	static uint32_t turn_toggle_tick = 0;
	if (turn_toggle_tick < HAL_GetTick()){
		if(left_toggles > 0){
		turn_toggle_tick = HAL_GetTick()+ 200;
		HAL_GPIO_TogglePin(D3_GPIO_Port, D3_Pin);
		left_toggles--;
		if(left_toggles==0){

			ssd1306_Fill(Black);
			ssd1306_SetCursor(25, 30);
			ssd1306_WriteString("left Light OFF", Font_7x10, White);
			ssd1306_UpdateScreen();


		}

	}else{
		HAL_GPIO_WritePin(D3_GPIO_Port, D3_Pin,1);
	}

  }
}

void turn_signal_right (void){

	static uint32_t tunr_togle_tick = 0;
	if(tunr_togle_tick  < HAL_GetTick() ){
		if(right_toggles> 0){
			tunr_togle_tick = HAL_GetTick() + 200;
			HAL_GPIO_TogglePin(D4_GPIO_Port, D4_Pin);
			right_toggles--;

			if(right_toggles==0){
				ssd1306_Fill(Black);
				ssd1306_SetCursor(25, 10);
				ssd1306_WriteString("right Light OFF", Font_7x10, White);
				ssd1306_UpdateScreen();

			}



		} else{
			HAL_GPIO_WritePin(D4_GPIO_Port, D4_Pin, 1);

		}

	}
}

void signal_parking_light (void){

	static uint32_t tunr_togle_tick = 0;
	if(tunr_togle_tick  < HAL_GetTick() ){
		if(parking_toggle> 0){
			tunr_togle_tick = HAL_GetTick() + 300;
			HAL_GPIO_TogglePin(D4_GPIO_Port, D4_Pin);
			HAL_GPIO_TogglePin(D3_GPIO_Port, D3_Pin);
			parking_toggle--;

		} else{


		}

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
  MX_I2C1_Init();
  MX_USART2_UART_Init();
  MX_USART3_UART_Init();
  /* USER CODE BEGIN 2 */

  ssd1306_Init();
  ssd1306_SetCursor(25, 30);
  ssd1306_WriteString("Hello World!", Font_7x10, White);
  ssd1306_UpdateScreen();

  ring_buffer_init(&usart2_rb, usart2_buffer, USART2_BUFFER_SIZE);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  printf("Starting...\r\n");
  //HAL_UART_Receive_IT(&huart2, &usart2_rx, 1); // enable interrupt for USART2 Rx
  ATOMIC_SET_BIT(USART2->CR1, USART_CR1_RXNEIE); // usando un funcion mas liviana para reducir memoria
  while (1) {

	  turn_signal_left();
	  turn_signal_right();
	  signal_parking_light();
	  heartbeat();




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
  if (HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_MSI;
  RCC_OscInitStruct.MSIState = RCC_MSI_ON;
  RCC_OscInitStruct.MSICalibrationValue = 0;
  RCC_OscInitStruct.MSIClockRange = RCC_MSIRANGE_6;
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
}

/**
  * @brief I2C1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C1_Init(void)
{

  /* USER CODE BEGIN I2C1_Init 0 */

  /* USER CODE END I2C1_Init 0 */

  /* USER CODE BEGIN I2C1_Init 1 */

  /* USER CODE END I2C1_Init 1 */
  hi2c1.Instance = I2C1;
  hi2c1.Init.Timing = 0x00000E14;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Analogue filter
  */
  if (HAL_I2CEx_ConfigAnalogFilter(&hi2c1, I2C_ANALOGFILTER_ENABLE) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Digital filter
  */
  if (HAL_I2CEx_ConfigDigitalFilter(&hi2c1, 0) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C1_Init 2 */

  /* USER CODE END I2C1_Init 2 */

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
  * @brief USART3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART3_UART_Init(void)
{

  /* USER CODE BEGIN USART3_Init 0 */

  /* USER CODE END USART3_Init 0 */

  /* USER CODE BEGIN USART3_Init 1 */

  /* USER CODE END USART3_Init 1 */
  huart3.Instance = USART3;
  huart3.Init.BaudRate = 115200;
  huart3.Init.WordLength = UART_WORDLENGTH_8B;
  huart3.Init.StopBits = UART_STOPBITS_1;
  huart3.Init.Parity = UART_PARITY_NONE;
  huart3.Init.Mode = UART_MODE_TX_RX;
  huart3.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart3.Init.OverSampling = UART_OVERSAMPLING_16;
  huart3.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart3.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart3) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART3_Init 2 */

  /* USER CODE END USART3_Init 2 */

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
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, D1_Pin|D3_Pin|ROW_1_Pin, GPIO_PIN_SET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, ROW_2_Pin|ROW_4_Pin|ROW_3_Pin, GPIO_PIN_SET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(D4_GPIO_Port, D4_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pins : S1_Pin S2_Pin */
  GPIO_InitStruct.Pin = S1_Pin|S2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : D1_Pin D3_Pin */
  GPIO_InitStruct.Pin = D1_Pin|D3_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : S3_Pin */
  GPIO_InitStruct.Pin = S3_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(S3_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : COLUMN_1_Pin */
  GPIO_InitStruct.Pin = COLUMN_1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_PULLDOWN;
  HAL_GPIO_Init(COLUMN_1_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : COLUMN_4_Pin */
  GPIO_InitStruct.Pin = COLUMN_4_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_PULLDOWN;
  HAL_GPIO_Init(COLUMN_4_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : COLUMN_2_Pin COLUMN_3_Pin */
  GPIO_InitStruct.Pin = COLUMN_2_Pin|COLUMN_3_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_PULLDOWN;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : ROW_1_Pin */
  GPIO_InitStruct.Pin = ROW_1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(ROW_1_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : ROW_2_Pin ROW_4_Pin ROW_3_Pin */
  GPIO_InitStruct.Pin = ROW_2_Pin|ROW_4_Pin|ROW_3_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pin : D4_Pin */
  GPIO_InitStruct.Pin = D4_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(D4_GPIO_Port, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI1_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI1_IRQn);

  HAL_NVIC_SetPriority(EXTI4_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI4_IRQn);

  HAL_NVIC_SetPriority(EXTI9_5_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI9_5_IRQn);

  HAL_NVIC_SetPriority(EXTI15_10_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);

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
