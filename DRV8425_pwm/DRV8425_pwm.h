/*
 * DRV8425.h
 *
 *  Created on: Jan 22, 2026
 *      Author: luckyellasiri
 */

#ifndef INC_DRV8425_H_
#define INC_DRV8425_H_

#include <stdint.h>
#include "stm32g4xx_hal.h" // i think this is the correct file but check when you initialize a project
typedef struct {
	TIM_HandleTypeDef *htim_step;
	uint32_t htim_channel;
	uint32_t timer_clk_freq;
	GPIO_TypeDef *dir_port;
	uint16_t dir_pin;
	GPIO_TypeDef *enable_port;
	uint16_t enable_pin;
    GPIO_TypeDef *step_port;
    uint16_t step_pin;

	uint8_t direction;
	uint8_t enable_state;
} DRV8425_Handle;

#define MAX_STEP_FREQ 500000
#define MIN_STEP_FREQ (timer_clk_freq) ((timer_clk_freq) / 65536)

#define FORWARD 1
#define BACKWARD 0

#define SUCCESS 0
#define FREQ_OUT_OF_BOUNDS 1


// Initializes handle for the driver that holds  all hardware details (pwm channel, pin assignments).
void DRV8425_Init(DRV8425_Handle *hdrv);

// Drive the motor a given number of steps with the direction indicated by the sign of param steps, at the given frequency.
uint8_t DRV8425_DriveSteps(DRV8425_Handle *hdrv, int32_t steps, uint32_t freq);

// Stop motor regardless of steps remaining.
uint8_t DRV8425_Stop(DRV8425_Handle *hdrv);


#endif /* INC_DRV8425_H_ */
