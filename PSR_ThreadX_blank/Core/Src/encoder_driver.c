/*
 * encoder_driver.c
 *
 *  Created on: Oct 19, 2025
 *      Author: Ondrej Teren
 */

#include "encoder_driver.h"

#define AROUND 256
#define KP 0.9f

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
	*position = TIM1->CNT % (AROUND - 1);
	ret = tx_mutex_put(&encoder_mutex);
	return ret;
}

UINT encoder_driver_output(uint32_t position)
{
	UINT ret;
    ret = tx_mutex_get(&encoder_mutex, TX_WAIT_FOREVER);
    TIM1->CNT = position % (AROUND - 1);
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

static uint32_t min(uint32_t a, uint32_t b) {
	if(a < b) {
		return a;
	}
	else {
		return b;
	}
}

float P_update(int error) {
    return KP * error;
}

VOID motor_driver_controller(uint32_t target)
{
	uint32_t pos;
	encoder_driver_input(&pos);

	uint32_t r_dist = min(pos - target, pos + AROUND - target);
	uint32_t l_dist = min(target - pos, target + AROUND - pos);

	printf("Motor: POS:%ld TARGET:%ld RD:%ld LD:%ld\n", pos, target, r_dist, l_dist);
	if(pos != target) {
		if(r_dist < l_dist) {
			motor_driver_input_right(P_update(r_dist));
		}
		else {
			motor_driver_input_left(P_update(l_dist));
		}
	}
}

 
