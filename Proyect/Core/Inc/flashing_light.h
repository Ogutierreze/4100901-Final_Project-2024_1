
/*
 * flashing_light.h
 *
 *  Created on: Sep 13, 2024
 *      Author: Cristian
 */


#ifndef INC_FLASHING_LIGHT_H_
#define INC_FLASHING_LIGHT_H_

#include "stm32l4xx_hal.h"
#include <stdint.h>
void flashing_signal(GPIO_TypeDef* GPIO_Port, uint16_t GPIO_Pin, uint32_t interval_ms, uint8_t *toggles_count);

#endif /* INC_FLASHING_LIGHT_H_ */
