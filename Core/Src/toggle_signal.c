/*
 * toggle_signal.c
 *
 *  Created on: Sep 29, 2024
 *      Author: Cristian
 */

#include "toggle_signal.h"

uint8_t toggle_turn_signal(GPIO_TypeDef* GPIO_Port, uint16_t GPIO_Pin, uint32_t *toggles, uint32_t blink_interval)
{
    static uint32_t toggle_tick = 0;

    if (toggle_tick < HAL_GetTick()) {
        if (*toggles > 0) {
            toggle_tick = HAL_GetTick() + blink_interval;  // Establece el intervalo para el parpadeo
            HAL_GPIO_TogglePin(GPIO_Port, GPIO_Pin);       // Alterna el estado del pin del LED
            (*toggles)--;                                  // Decrementa el contador de toggles

            if (*toggles == 0) {
                return 0;  // Si no quedan más toggles, retornar 0
            }
        }
    }
    return 1;  // Si aún quedan toggles, retornar 1
}
