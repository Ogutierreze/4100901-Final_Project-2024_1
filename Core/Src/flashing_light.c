/*
 * flashing_light.c
 *
 *  Created on: Sep 13, 2024
 *      Author: Cristian
 */

#include "flashing_light.h"
#include "main.h"


void flashing_signal(GPIO_TypeDef* GPIO_Port, uint16_t GPIO_Pin, uint32_t interval_ms, uint8_t *toggles_count) {
    static uint32_t tunr_togle_tick = 0;

    // Obtenemos el tiempo actual
    uint32_t current_tick = HAL_GetTick();

    if (current_tick >= tunr_togle_tick) {
        if (*toggles_count > 0) {
            // Actualiza el tiempo para el siguiente parpadeo basado en el intervalo deseado
            tunr_togle_tick = current_tick + interval_ms;

            // Cambiar el estado del pin especificado
            HAL_GPIO_TogglePin(GPIO_Port, GPIO_Pin);

            // Reducir el contador de parpadeos
            (*toggles_count)--;
        } else {
            // Si no quedan parpadeos, asegurarse de que el pin esté apagado
            HAL_GPIO_WritePin(GPIO_Port, GPIO_Pin, GPIO_PIN_RESET);
        }
    }
}
