/*
 * toggle_signal.h
 *
 *  Created on: Sep 29, 2024
 *      Author: Cristian
 */


#ifndef TOGGLE_SIGNAL_H
#define TOGGLE_SIGNAL_H

#include "stm32l4xx.h"  // Ajusta esto a tu versión de HAL o tu microcontrolador

// Prototipo de la función que controla el parpadeo
uint8_t toggle_turn_signal(GPIO_TypeDef* GPIO_Port, uint16_t GPIO_Pin, uint32_t *toggles, uint32_t blink_interval);



#endif /* INC_TOGGLE_SIGNAL_H_ */
