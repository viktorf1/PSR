/*
 * encoder_driver.h
 *
 *  Created on: Oct 19, 2025
 *      Author: Ondrej Teren
 */

#ifndef INC_ENCODER_DRIVER_H_
#define INC_ENCODER_DRIVER_H_

#include "main.h"
#include "tx_api.h"

UINT encoder_driver_initialize();
UINT encoder_driver_input(uint32_t *position);
UINT encoder_driver_output(uint32_t position);

UINT motor_driver_initialize();
UINT motor_driver_input_left(uint32_t value);
UINT motor_driver_input_right(uint32_t value);
VOID motor_driver_controller(uint32_t target);

#endif /* INC_ENCODER_DRIVER_H_ */
