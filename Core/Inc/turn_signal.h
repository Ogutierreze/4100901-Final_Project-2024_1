/*
 * turn_signal.h
 *
 *  Created on: Sep 30, 2024
 *      Author: Cristian
 */

#ifndef INC_TURN_SIGNAL_H_
#define INC_TURN_SIGNAL_H_


#include "stm32l4xx_hal.h"  // Asegúrate de que esta es la versión correcta de tu HAL

// Función para manejar el botón y los toggles (izquierda o derecha)
void handle_turn_signal(GPIO_TypeDef* GPIO_Port, uint16_t GPIO_Pin, uint32_t* counter, uint32_t* toggles, uint32_t blink_interval, char* uart_msg_on, char* uart_msg_off);

// Función para manejar las luces de estacionamiento (ambas luces a la vez)
void handle_parking_lights(GPIO_TypeDef* GPIO_Port_Left, uint16_t GPIO_Pin_Left, GPIO_TypeDef* GPIO_Port_Right, uint16_t GPIO_Pin_Right, uint32_t* counter, uint32_t* toggles, uint32_t blink_interval, char* uart_msg_on, char* uart_msg_off);




#endif /* INC_TURN_SIGNAL_H_ */
