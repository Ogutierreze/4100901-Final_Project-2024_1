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
#include "turn_signal.h"
/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>

#include "ssd1306.h"
#include "ssd1306_fonts.h"
#include "password_validator.h"

#include "ring_buffer.h"
#include "keypad.h"
#define DEBOUNCE_TIME 200
uint8_t data_usart2;
uint8_t newline[] = "\r\n";

#define BUFFER_CAPACITY 10
uint8_t keyboard_buffer_memory[BUFFER_CAPACITY];
ring_buffer_t keyboard_ring_buffer;

#define USART2_BUFFER_SIZE 10
uint8_t usart2_buffer[USART2_BUFFER_SIZE];
ring_buffer_t usart2_rb;
uint8_t usart2_rx;

#define MAX_PASSWORD_LENGTH 10
char current_password[MAX_PASSWORD_LENGTH] = "12345";
uint8_t changing_password = 0;
char new_password[MAX_PASSWORD_LENGTH];
uint8_t new_password_index = 0;

uint8_t first_key_pressed = 0;
uint8_t cursor_x_position = 10;  // Control de la posición del cursor horizontal
uint8_t cursor_y_position = 30;  // Línea en la que aparecerán las teclas
uint8_t max_cursor_x_position = 80;

#define MAX_DISPLAY_CHARS 20 // Ajusta este valor según el tamaño de la pantalla y el tamaño del texto

// Buffer para almacenar la secuencia de teclas
static char display_buffer[MAX_DISPLAY_CHARS + 1]; // +1 para el terminador nulo
static uint8_t buffer_index = 0; // Asegúrate de que esta definición sea única

// Variables para la posición actual del cursor en la pantalla
static uint8_t cursor_x = 10;
static uint8_t cursor_y = 30;
uint16_t left_toggles=0;
uint8_t incorrect_password_toggles = 6;  // Parpadeos para contraseña incorrecta
uint32_t incorrect_password_interval = 125;  // 125 ms entre parpadeos
uint16_t left_toggles2=0;
uint8_t authorized_access=0;
uint8_t flag_parking;
uint8_t validate_uart=2;
uint8_t key_pressed=0xFF;
uint8_t detected_d=0;
uint8_t detected_a=0;
uint8_t detected_b=0;



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
	  ring_buffer_write(&usart2_rb, usart2_rx);
	  }

	  // put the data received in buffer
	  HAL_UART_Receive_IT(&huart2, &usart2_rx, 1); // enable interrupt to continue receiving
	  ATOMIC_SET_BIT(USART2->CR1, USART_CR1_RXNEIE); // usando un funcion mas liviana para reducir memoria
  }



// Contraseña actual, inicialmente es la contraseña por defecto "12345"


void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    key_pressed = keypad_scan(GPIO_Pin);

    // Si el sistema tiene acceso autorizado
    if (authorized_access == 1) {
        handle_all_buttons(GPIO_Pin);
    }

    if (key_pressed != 0xFF) {

        // Resetear secuencia
        if (key_pressed == '*') {
            ring_buffer_reset(&keyboard_ring_buffer);
            memset(display_buffer, 0, sizeof(display_buffer)); // Limpiar el buffer de pantalla
            buffer_index = 0; // Reiniciar el índice del buffer

            ssd1306_Fill(Black);
            ssd1306_SetCursor(10, 20);
            ssd1306_WriteString("Secuencia reiniciada", Font_6x8, White);
            ssd1306_UpdateScreen();
            HAL_UART_Transmit(&huart2, (uint8_t*)"Secuencia reiniciada\n\r", 22, 10);
            return;
        }

        if (key_pressed == 'A') {
            if (authorized_access == 1 && !changing_password) {
                // Cambiar a estado de cambio de contraseña
                changing_password = 1;
                new_password_index = 0;  // Reiniciar índice de la nueva contraseña

                // Mostrar mensaje de cambio de contraseña en la pantalla
                ssd1306_Fill(Black);
                ssd1306_SetCursor(10, 10);
                ssd1306_WriteString("Cambio de", Font_6x8, White);
                ssd1306_SetCursor(10, 20);
                ssd1306_WriteString("contrasena", Font_6x8, White);
                ssd1306_SetCursor(10, 40);
                ssd1306_WriteString("Ingrese nueva", Font_6x8, White);
                ssd1306_SetCursor(10, 50);
                ssd1306_WriteString("y presione #", Font_6x8, White);
                ssd1306_UpdateScreen();

                return;
            }
        }

        if (key_pressed == 'B') {
            detected_b = 1;
        }

        // Si estamos en el estado de cambiar la contraseña
        if (changing_password) {
            if (key_pressed == '#' ) {
                // Cuando el usuario presiona '#', validar la contraseña ingresada
                if (new_password_index > 0) {
                    strncpy(current_password, new_password, MAX_PASSWORD_LENGTH);  // Guardar la nueva contraseña

                    // Mostrar mensaje de éxito en la pantalla
                    ssd1306_Fill(Black);
                    ssd1306_SetCursor(10, 20);
                    ssd1306_WriteString("Contrasena", Font_6x8, White);
                    ssd1306_SetCursor(10, 30);
                    ssd1306_WriteString("cambiada!", Font_6x8, White);
                    ssd1306_UpdateScreen();
                    HAL_UART_Transmit(&huart2, (uint8_t*)"Contrasena cambiada\n\r", 21, 10);

                    // Resetear el estado de cambio de contraseña
                    changing_password = 0;

                    ssd1306_UpdateScreen();
                }
                return;
            } else if (key_pressed != '*' && key_pressed != '#' && new_password_index < MAX_PASSWORD_LENGTH) {
                // Almacenar la tecla como parte de la nueva contraseña
                new_password[new_password_index++] = key_pressed;
                new_password[new_password_index] = '\0'; // Null-terminar el string

                // Mostrar los asteriscos mientras se ingresa la nueva contraseña
                ssd1306_Fill(Black);
                ssd1306_SetCursor(10, 30);
                ssd1306_WriteString("Nueva clave:", Font_6x8, White);
                ssd1306_SetCursor(10, 50);
                ssd1306_WriteString("********", Font_6x8, White);  // Mostrar asteriscos
                ssd1306_UpdateScreen();
            }
            return;
        }

        // Escribir la tecla en el ring buffer si no es el menú
        if (key_pressed != '#' && key_pressed != 'D' && key_pressed != 'A' && key_pressed != 'B') {
            ring_buffer_write(&keyboard_ring_buffer, key_pressed);

            if (buffer_index < MAX_DISPLAY_CHARS) {
                display_buffer[buffer_index++] = key_pressed;
                display_buffer[buffer_index] = '\0'; // Null-terminar el buffer

                ssd1306_Fill(Black);
                ssd1306_SetCursor(10, 30);
                ssd1306_WriteString(display_buffer, Font_6x8, White);
                ssd1306_UpdateScreen();
                HAL_UART_Transmit(&huart2, &key_pressed, 1, 10);
            }
            return;
        }

        // Validar la contraseña con la actual (que es "12345" por defecto o la nueva si se ha cambiado)
        const char* my_id2 = current_password;  // Usamos la contraseña actual
        uint8_t result = validate_password(my_id2, &keyboard_ring_buffer);

        if (result == 1) {
            HAL_UART_Transmit(&huart2, (uint8_t*)"Contrasena Correcta\n\r", 21, 10);
            HAL_UART_Transmit(&huart2, (uint8_t*)"Iniciando...\n\r", 14, 10);
            // Contraseña correcta
            ssd1306_Fill(Black);
            ssd1306_SetCursor(10, 20);
            ssd1306_WriteString("Acceso autorizado", Font_6x8, White);
            ssd1306_UpdateScreen();
            authorized_access = 1;
        } else {
            ssd1306_Fill(Black);
            ssd1306_SetCursor(10, 20);
            ssd1306_WriteString("Incorrecto", Font_6x8, White);
            ssd1306_UpdateScreen();
            HAL_UART_Transmit(&huart2, (uint8_t*)"Incorrecto\n\r", 12, 10);
            left_toggles = 6;
        }

        // Reiniciar buffer y display
        ring_buffer_reset(&keyboard_ring_buffer);
        memset(display_buffer, 0, sizeof(display_buffer));
        buffer_index = 0;
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
	if(parking_toggle==0){
		static uint32_t turn_toggle_tick = 0;
		if (turn_toggle_tick < HAL_GetTick()){
			if(left_toggles > 0){
			turn_toggle_tick = HAL_GetTick()+ 200;
			HAL_GPIO_TogglePin(D3_GPIO_Port, D3_Pin);
			left_toggles--;
			if(left_toggles==0){

				ssd1306_Fill(Black);
				ssd1306_SetCursor(10, 10);
				ssd1306_WriteString("left Light OFF", Font_7x10, White);
				ssd1306_UpdateScreen();
				if(right_toggles!=0){
					ssd1306_SetCursor(10, 40);
					ssd1306_WriteString("Right Light ON", Font_7x10, White);
					ssd1306_UpdateScreen();


				}else{
					ssd1306_SetCursor(10, 40);
					ssd1306_WriteString("Right Light OFF", Font_7x10, White);
					ssd1306_UpdateScreen();


				}


			}else{
				ssd1306_Fill(Black);
				ssd1306_SetCursor(10, 10);
				ssd1306_WriteString("left Light ON", Font_7x10, White);
				ssd1306_SetCursor(10, 40);
				ssd1306_WriteString("Right Light OFF", Font_7x10, White);
				ssd1306_UpdateScreen();
			}

		}else{
	//		HAL_GPIO_WritePin(D3_GPIO_Port, D3_Pin,1);
		}

	  }

	}

}

void turn_signal_right (void){
	if(parking_toggle==0){
		static uint32_t tunr_togle_tick = 0;
		if(tunr_togle_tick  < HAL_GetTick() ){
			if(right_toggles> 0){
				tunr_togle_tick = HAL_GetTick() + 200;
				HAL_GPIO_TogglePin(D4_GPIO_Port, D4_Pin);
				right_toggles--;

				if(right_toggles==0){

					ssd1306_Fill(Black);
					ssd1306_SetCursor(10, 40);
					ssd1306_WriteString("Right Light OFF", Font_7x10, White);
					ssd1306_UpdateScreen();
					if(right_toggles!=0){
						ssd1306_SetCursor(10, 10);
						ssd1306_WriteString("light Light ON", Font_7x10, White);
						ssd1306_UpdateScreen();


					}else{
						ssd1306_SetCursor(10, 10);
						ssd1306_WriteString("light Light OFF", Font_7x10, White);
						ssd1306_UpdateScreen();


					}


				}else{
					ssd1306_Fill(Black);
					ssd1306_SetCursor(10, 10);
					ssd1306_WriteString("left Light OFF", Font_7x10, White);
					ssd1306_SetCursor(10, 40);
					ssd1306_WriteString("Right Light ON", Font_7x10, White);
					ssd1306_UpdateScreen();
				}

				}



			} else{
	//			HAL_GPIO_WritePin(D4_GPIO_Port, D4_Pin, 1);

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

			if(parking_toggle==0){


			}else{
				ssd1306_Fill(Black);
				ssd1306_SetCursor(0, 25);
				ssd1306_WriteString("Parking Light ON", Font_7x10, White);
				ssd1306_UpdateScreen();
			}


		}
	}
}




void handle_all_buttons(uint16_t GPIO_Pin) {
    // Lógica para apagar la luz derecha si se presiona S1 (luz izquierda)
    if (GPIO_Pin == S1_Pin) {
    	if(flag_parking==0){
            if (right_toggles > 0) {
                right_toggles = 0;  // Apaga la luz derecha si está parpadeando
                HAL_GPIO_WritePin(D4_GPIO_Port, D4_Pin, 1);  // Apaga el LED derecho
                HAL_UART_Transmit(&huart2, "Right light off (due to S1)\r\n", 30, 10);

            }
            handle_turn_signal(D3_GPIO_Port, D3_Pin, &counter_left, &left_toggles, 200, "S1\r\n", "S1_off\r\n");


    	}

    }

    // Lógica para apagar la luz izquierda si se presiona S2 (luz derecha)
    if (GPIO_Pin == S2_Pin) {
    	if(flag_parking==0){
            if (left_toggles > 0) {
                left_toggles = 0;  // Apaga la luz izquierda si está parpadeando
                HAL_GPIO_WritePin(D3_GPIO_Port, D3_Pin, 1);  // Apaga el LED izquierdo
                HAL_UART_Transmit(&huart2, (uint8_t *)"Left light off (due to S2)\r\n", 29, 10);
            }
            handle_turn_signal(D4_GPIO_Port, D4_Pin, &counter_right, &right_toggles, 200, "S2\r\n", "S2_off\r\n");

    	}


    }

    // Lógica para apagar ambas luces si se presiona S3 (luces de parqueo)
    if (GPIO_Pin == S3_Pin) {
        if (left_toggles > 0 || right_toggles > 0) {
            left_toggles = 0;  // Apaga la luz izquierda si está encendida
            right_toggles = 0;  // Apaga la luz derecha si está encendida
            HAL_GPIO_WritePin(D3_GPIO_Port, D3_Pin, 1);  // Apaga el LED izquierdo
            HAL_GPIO_WritePin(D4_GPIO_Port, D4_Pin, 1);  // Apaga el LED derecho
            HAL_UART_Transmit(&huart2, (uint8_t *)"Both lights off (due to S3)\r\n", 31, 10);
        }

        handle_parking_lights(D3_GPIO_Port, D3_Pin, D4_GPIO_Port, D4_Pin, &counter_parking, &parking_toggle, 300, "Parking ON\r\n", "Parking OFF\r\n");

    }
}


// Función para manejar el botón y los toggles (izquierda o derecha)
void handle_turn_signal(GPIO_TypeDef* GPIO_Port, uint16_t GPIO_Pin, uint32_t* counter, uint32_t* toggles, uint32_t blink_interval, char* uart_msg_on, char* uart_msg_off) {
    uint32_t current_time = HAL_GetTick();

    if (current_time - last_debounce_time_left >= DEBOUNCE_TIME) {

        if (current_time - last_debounce_time_left > 1000 && *counter < 2) {
            *counter = 0;  // Reinicia el contador si ha pasado suficiente tiempo sin una segunda pulsación
        }

        (*counter)++;
        last_debounce_time_left = current_time;

        if (*counter == 1) {
            *toggles = 6;  // 6 parpadeos
            HAL_UART_Transmit(&huart2, uart_msg_on, strlen(uart_msg_on), 10);
        } else if (*counter == 2) {
            *toggles = 0xEEEEEEE;  // Parpadeo infinito
            HAL_UART_Transmit(&huart2, uart_msg_on, strlen(uart_msg_on), 10);
        } else if (*counter >= 3) {
            *counter = 0;
            *toggles = 0;  // Apagar la luz
            HAL_UART_Transmit(&huart2, uart_msg_off, strlen(uart_msg_off), 10);
            HAL_GPIO_WritePin(GPIO_Port, GPIO_Pin, 1);
        }
    }
}

// Función para manejar las luces de estacionamiento (ambas luces a la vez)
void handle_parking_lights(GPIO_TypeDef* GPIO_Port_Left, uint16_t GPIO_Pin_Left, GPIO_TypeDef* GPIO_Port_Right, uint16_t GPIO_Pin_Right, uint32_t* counter, uint32_t* toggles, uint32_t blink_interval, char* uart_msg_on, char* uart_msg_off) {
    uint32_t current_time = HAL_GetTick();

    if (current_time - last_debounce_time_parking >= DEBOUNCE_TIME) {
        (*counter)++;
        last_debounce_time_parking = current_time;

        if (*counter == 1) {
            *toggles = 0xEEEE;  // Parpadeo continuo para las luces de estacionamiento
            HAL_UART_Transmit(&huart2, uart_msg_on, strlen(uart_msg_on), 10);
        } else if (*counter >= 2) {
			ssd1306_Fill(Black);
			ssd1306_SetCursor(0, 25);
			ssd1306_WriteString("parking Light OFF", Font_7x10, White);
			ssd1306_UpdateScreen();
            *toggles = 0;  // Apagar ambas luces
            HAL_UART_Transmit(&huart2, uart_msg_off, strlen(uart_msg_off), 10);
            *counter = 0;
            HAL_GPIO_WritePin(GPIO_Port_Right, GPIO_Pin_Right, 1);
            HAL_GPIO_WritePin(GPIO_Port_Left, GPIO_Pin_Left, 1);



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
  ssd1306_SetCursor(0, 10);
  ssd1306_UpdateScreen();
  ssd1306_WriteString("Bienvenido", Font_6x8, White);
  ssd1306_SetCursor(0, 30);
  ssd1306_UpdateScreen();
  ssd1306_WriteString("Ingrese la ", Font_6x8, White);
  ssd1306_SetCursor(0, 50);
  ssd1306_WriteString("Contraseña", Font_6x8, White);
  ssd1306_UpdateScreen();


  ring_buffer_init(&keyboard_ring_buffer, keyboard_buffer_memory, BUFFER_CAPACITY);
  ring_buffer_init(&usart2_rb, usart2_buffer, USART2_BUFFER_SIZE);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  printf("Starting...\r\n");
  //HAL_UART_Receive_IT(&huart2, &usart2_rx, 1); // enable interrupt for USART2 Rx
  ATOMIC_SET_BIT(USART2->CR1, USART_CR1_RXNEIE); // usando un funcion mas liviana para reducir memoria
  while (1) {

	  const char my_id1[4] = "open";
	  if(ring_buffer_is_empty(&usart2_rb)==0){
		  validate_uart=validate_password(my_id1,&usart2_rb);
		  if(validate_uart==1 ){

	          HAL_UART_Transmit(&huart2, "acceso permitido\n\r",18 , 10);

		  }else if(validate_uart==0){
	          HAL_UART_Transmit(&huart2, "acceso denegado\n\r",17 , 10);

		  }
	  }





      if(detected_b!=0){
      	authorized_access=0;
        ssd1306_Fill(Black);
        ssd1306_WriteString("Bienvenido", Font_6x8, White);
        ssd1306_SetCursor(0, 30);
        ssd1306_UpdateScreen();
        ssd1306_WriteString("Ingrese la ", Font_6x8, White);
        ssd1306_SetCursor(0, 50);
        ssd1306_WriteString("Contraseña", Font_6x8, White);
        ssd1306_UpdateScreen();
        detected_b=0;


      }


	  if(authorized_access==1){




          heartbeat();
    	  turn_signal_left();
    	  turn_signal_right();
    	  signal_parking_light();
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
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, D1_Pin|D3_Pin|ROW_1_Pin, GPIO_PIN_SET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, ROW_2_Pin|ROW_4_Pin|ROW_3_Pin|D4_Pin, GPIO_PIN_SET);

  /*Configure GPIO pin : B1_Pin */
  GPIO_InitStruct.Pin = B1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(B1_GPIO_Port, &GPIO_InitStruct);

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
  HAL_NVIC_SetPriority(EXTI0_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI0_IRQn);

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
#endif /* USE_FULL_ASSERT */
