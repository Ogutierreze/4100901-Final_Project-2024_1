/*
 * verify_password.h
 *
 *  Created on: Sep 29, 2024
 *      Author: Cristian
 */

#ifndef PASSWORD_VALIDATOR_H
#define PASSWORD_VALIDATOR_H
#include "ring_buffer.h"

#include <stdint.h>

// Prototipo de la función de validación
// Retorna 1 si la contraseña es correcta, 0 si es incorrecta.
uint8_t validate_password(const char *correct_password, ring_buffer_t *keyboard_ring_buffer);

#endif
