/*
 * DRV8425.c
 *
 *  Created on: Feb 7, 2026
 *      Author: luckyellasiri
 */

#include "DRV8425.h"
#include <stdio.h>
#include <stdint.h>

#define MAX_STEP_FREQ 500000
#define MIN_STEP_FREQ (hdrv->timer_clk_freq / 65536)

//GPIO initialization structs
GPIO_InitTypeDef GPIO_Enable;
GPIO_InitTypeDef GPIO_Dir;
GPIO_InitTypeDef GPIO_Step;


//global ints
//static DRV8425_Handle *active_drv = NULL;
//static DRV8425_Handle *handle_dir[8];
volatile static DRV8425_Handle *active_drv;

// sets the HAL GPIO pin configurations for the stepper motor
uint8_t DRV8425_Init(DRV8425_Handle *hdrv) {
	/*if (hdrv->htim_step->Instance == TIM1) {
			if (handle_dir[0] == NULL) {
				return HANDLE_IN_USE;
			}
			handle_dir[0] = hdrv;
		} else if (hdrv->htim_step->Instance == TIM2) {
			if (handle_dir[1] == NULL) {
				return HANDLE_IN_USE;
			}
			handle_dir[1] = hdrv;
		} else if (hdrv->htim_step->Instance == TIM3) {
			if (handle_dir[2] == NULL) {
				return HANDLE_IN_USE;
			}
			handle_dir[2] = hdrv;

		} else if (hdrv->htim_step->Instance == TIM4) {
			if (handle_dir[3] == NULL) {
				return HANDLE_IN_USE;
			}
			handle_dir[3] = hdrv;
		} else if (hdrv->htim_step->Instance == TIM5) {
			if (handle_dir[4] == NULL) {
				return HANDLE_IN_USE;
			}
			handle_dir[4] = hdrv;
		} else if (hdrv->htim_step->Instance == TIM6) {
			if (handle_dir[5] == NULL) {
				return HANDLE_IN_USE;
			}
			handle_dir[5] = hdrv;
		} else if (hdrv->htim_step->Instance == TIM7) {
			if (handle_dir[6] == NULL) {
				return HANDLE_IN_USE;
			}
			handle_dir[6] = hdrv;
		} else if (hdrv->htim_step->Instance == TIM8) {
			if (handle_dir[7] == NULL) {
				return HANDLE_IN_USE;
			}
			handle_dir[7] = hdrv;
		} else {
			return INVALID_TIMER;
		}
		*/

	GPIO_Enable.Pin = hdrv->enable_pin;
    GPIO_Enable.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_Enable.Pull = GPIO_NOPULL;
    GPIO_Enable.Speed = GPIO_SPEED_FREQ_MEDIUM;
    HAL_GPIO_Init(hdrv->enable_port, &GPIO_Enable);

    GPIO_Dir.Pin = hdrv->dir_pin;
    GPIO_Enable.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_Enable.Pull = GPIO_NOPULL;
    GPIO_Enable.Speed = GPIO_SPEED_FREQ_MEDIUM;
    HAL_GPIO_Init(hdrv->dir_port, &GPIO_Dir);

    GPIO_Dir.Pin = hdrv->step_pin;
    GPIO_Enable.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_Enable.Pull = GPIO_NOPULL;
    GPIO_Enable.Speed = GPIO_SPEED_FREQ_MEDIUM;
    HAL_GPIO_Init(hdrv->step_port, &GPIO_Step);

    return SUCCESS;
}


uint8_t DRV8425_DriveSteps(DRV8425_Handle *hdrv, int32_t steps, uint32_t freq) {
    //setup & command - does not generate steps itself!!!
    //need to define ints at top of file -> steps_remaining int for incrementing and active_drv address

    //make sure there are steps remaining (steps != 0)
    //make sure freq !=0
    //make sure there's a valid driver like motorX -> so you can access dir/step pins
    if (!hdrv || steps == 0 || freq == 0) return DRIVE_STEPS_ERROR;

    if ((freq > MAX_STEP_FREQ) || (freq < MIN_STEP_FREQ)) {
    	return FREQ_OUT_OF_BOUNDS;
    }

    uint32_t num_steps = 0;
    //determine directions -> pos steps = DIR pin 0 = positive/forward CW, DIR pin 1 = negative/reverse CCW
    if(steps>0){
        HAL_GPIO_WritePin(hdrv->dir_port, hdrv->dir_pin, GPIO_PIN_SET); //dir pin -> 1 for pos steps
        num_steps = steps;
    } else{
        HAL_GPIO_WritePin(hdrv->dir_port, hdrv->dir_pin, GPIO_PIN_RESET); //dir pin -> 0 for negative steps
        num_steps = -steps;
    }
    //STEPS = "steps remaining" - so need absolute value because DIR pin determines direction
    //since we've already written the direction pin, can just negate if steps are negative for steps remaining

    //set active driver for stepping -> so proper hdrv is accessed by HAL_TIM_periodElapsed


    //configure timing
    //take provided frequency (steps/sec) -> timer period for interrupt
    //need to calculate new autoreload value (how many ticks between steps?)

    // formula: timclk / (psc + 1) = tick frequency
    // prescaler value
    uint32_t psc = hdrv->htim_step->Init.Prescaler;
    uint32_t tim_clk = hdrv->timer_clk_freq; //ASSUME this is connected to APB2 bus, if on APB1 will need to update formula
    uint32_t tick_freq = tim_clk / (psc+1); //how many ticks per second based on timer configuration

    // calculate arr (autoreload for interrupt)
    // -> we want arr to equal ticks/step (one interrupt every step)
    //uint32_t arr = (tick_freq / freq) - 1;
    uint32_t arr = 63999;

    uint32_t ccr = 32000 - 1;

    active_drv = hdrv;

    //write that value into timer's auto reload register -> so the interrupt occurs every X ticks
    __HAL_TIM_SET_AUTORELOAD(hdrv->htim_step, arr);
    __HAL_TIM_SET_COMPARE(hdrv->htim_step, hdrv->htim_channel, ccr);
    hdrv->steps_remaining = num_steps;
    //__HAL_TIM_SET_REPETITION_COUNTER(hdrv->htim_step, steps - 1);
    /*
    hdrv->htim_step->Instance->RCR = steps - 1;
    __HAL_TIM_SET_COUNTER(hdrv->htim_step, 0);
    HAL_TIM_GenerateEvent(hdrv->htim_step, TIM_EVENTSOURCE_UPDATE);
    */

    //ENABLE the motor - make driver active
    HAL_GPIO_WritePin(hdrv->enable_port, hdrv->enable_pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_7, GPIO_PIN_SET);
    //start the first interrupt
    HAL_TIM_PWM_Start_IT(hdrv->htim_step, hdrv->htim_channel);

    //signal for success
    return SUCCESS;
}

//interrupt function (configured by drivesteps) -> each interrupt is one STEP pulse (written to STEP pin), decrements remaining step count
//stops timer and disables driver when it runs out of steps
//motor moves to idle state -> could change to Sleep state??
//if drive_steps is called again, the process restarts (direction can change, etc.)

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim){
    //make sure you're accessing the right timer for interrupt

	/*
	if (htim->Instance == TIM1) {
		HAL_TIM_Base_Stop_IT(htim);
		HAL_TIM_PWM_Stop(handle_dir[0]->htim_step, handle_dir[0]->htim_channel);
		HAL_GPIO_WritePin(handle_dir[0]->enable_port, handle_dir[0]->enable_pin, GPIO_PIN_RESET);
	} else if (htim->Instance == TIM2) {
		HAL_TIM_Base_Stop_IT(htim);
		HAL_TIM_PWM_Stop(handle_dir[1]->htim_step, handle_dir[1]->htim_channel);
		HAL_GPIO_WritePin(handle_dir[1]->enable_port, handle_dir[1]->enable_pin, GPIO_PIN_RESET);

	} else if (htim->Instance == TIM3) {
		HAL_TIM_Base_Stop_IT(htim);
		HAL_TIM_PWM_Stop(handle_dir[2]->htim_step, handle_dir[2]->htim_channel);
		HAL_GPIO_WritePin(handle_dir[2]->enable_port, handle_dir[2]->enable_pin, GPIO_PIN_RESET);

	} else if (htim->Instance == TIM4) {
		HAL_TIM_Base_Stop_IT(htim);
		HAL_TIM_PWM_Stop(handle_dir[3]->htim_step, handle_dir[3]->htim_channel);
		HAL_GPIO_WritePin(handle_dir[3]->enable_port, handle_dir[3]->enable_pin, GPIO_PIN_RESET);

	} else if (htim->Instance == TIM5) {
		HAL_TIM_Base_Stop_IT(htim);
		HAL_TIM_PWM_Stop(handle_dir[4]->htim_step, handle_dir[4]->htim_channel);
		HAL_GPIO_WritePin(handle_dir[4]->enable_port, handle_dir[4]->enable_pin, GPIO_PIN_RESET);

	} else if (htim->Instance == TIM6) {
		HAL_TIM_Base_Stop_IT(htim);
		HAL_TIM_PWM_Stop(handle_dir[5]->htim_step, handle_dir[5]->htim_channel);
		HAL_GPIO_WritePin(handle_dir[5]->enable_port, handle_dir[5]->enable_pin, GPIO_PIN_RESET);

	} else if (htim->Instance == TIM7) {
		HAL_TIM_Base_Stop_IT(htim);
		HAL_TIM_PWM_Stop(handle_dir[6]->htim_step, handle_dir[6]->htim_channel);
		HAL_GPIO_WritePin(handle_dir[6]->enable_port, handle_dir[6]->enable_pin, GPIO_PIN_RESET);
	} else {
		HAL_TIM_Base_Stop_IT(htim);
		HAL_TIM_PWM_Stop(handle_dir[7]->htim_step, handle_dir[7]->htim_channel);
		HAL_GPIO_WritePin(handle_dir[7]->enable_port, handle_dir[7]->enable_pin, GPIO_PIN_RESET);
	}
	*/

	if (active_drv->steps_remaining == 0) {
	HAL_TIM_PWM_Stop_IT(active_drv->htim_step, active_drv->htim_channel);
	HAL_GPIO_WritePin(active_drv->enable_port, active_drv->enable_pin, GPIO_PIN_SET);
	active_drv = NULL;
	}

	active_drv->steps_remaining--;

}


// Stop motor regardless of steps remaining. -> TURN OFF ENABLE
uint8_t DRV8425_Stop(DRV8425_Handle *hdrv) {
    if(!hdrv) return HANDLE_ERROR;
    //stop the interrupt
    HAL_TIM_PWM_Stop_IT(hdrv->htim_step, hdrv->htim_channel);
    //cancel remaining steps
    //disable + reset driver
    HAL_GPIO_WritePin(hdrv->enable_port, hdrv->enable_pin, GPIO_PIN_SET);
    active_drv = NULL;

    return 0;
}

