/*
 * password_validator.c
 *
 *  Created on: Sep 29, 2024
 *      Author: Cristian
 */
#include "password_validator.h"
#include "ssd1306_fonts.h"
#include "ssd1306.h"
#include "ring_buffer.h"
#include <string.h>

uint8_t validate_password(const char *correct_password, ring_buffer_t *keyboard_ring_buffer) {
    uint8_t byte = 0;
    uint8_t id_incorrect = 0;
    uint8_t password_length = sizeof(correct_password);

    // Leer del buffer y comparar con la clave correcta
    for (uint8_t idx = 0; idx < password_length-1; idx++) {
        if (ring_buffer_read(keyboard_ring_buffer, &byte) != 0) {
            if (byte != correct_password[idx]) {
                id_incorrect = 1;  // Marcar como incorrecto si no coincide
                break;
            }else{id_incorrect=0;}

        } else {
            id_incorrect = 1;  // Si no hay suficientes caracteres en el buffer
            break;
        }
    }

    if (id_incorrect == 0) {
        // Contraseña correcta


        return 1;
    } else {
        // Contraseña incorrecta

        return 0;
    }
}
