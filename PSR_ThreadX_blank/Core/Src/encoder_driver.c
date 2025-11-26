/*
 * encoder_driver.c
 *
 *  Created on: Oct 19, 2025
 *      Author: Ondrej Teren
 */

#include "encoder_driver.h"
TX_MUTEX encoder_mutex;
TX_MUTEX motor_mutex;

UINT encoder_driver_initialize()
{
	UINT ret;
	ret = tx_mutex_create(&encoder_mutex, "Encoder Mutex", TX_NO_INHERIT);
	return ret;
}

UINT encoder_driver_input(uint32_t *position)
{
	UINT ret;
	ret = tx_mutex_get(&encoder_mutex, TX_WAIT_FOREVER);
	*position = TIM1->CNT;
	ret = tx_mutex_put(&encoder_mutex);
	return ret;
}

UINT encoder_driver_output(uint32_t position)
{
	UINT ret;
    ret = tx_mutex_get(&encoder_mutex, TX_WAIT_FOREVER);
    TIM1->CNT = position;
    ret = tx_mutex_put(&encoder_mutex);
    return ret;
}

UINT motor_driver_initialize()
{
	UINT ret;
	ret = tx_mutex_create(&motor_mutex, "Motor Mutex", TX_NO_INHERIT);
	return ret;
}

UINT motor_driver_input_left(uint32_t value)
{
	UINT ret;
	if (value > 500) value = 500;
	ret = tx_mutex_get(&motor_mutex, TX_WAIT_FOREVER);
	TIM2->CCR3 = value;
	TIM2->CCR4 = 0;
	ret = tx_mutex_put(&motor_mutex);
	return ret;
}

UINT motor_driver_input_right(uint32_t value)
{
	UINT ret;
	if (value > 500) value = 500;
	ret = tx_mutex_get(&motor_mutex, TX_WAIT_FOREVER);
	TIM2->CCR3 = 0;
	TIM2->CCR4 = value;
	ret = tx_mutex_put(&motor_mutex);
	return ret;
}

 