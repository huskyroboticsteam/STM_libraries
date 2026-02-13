/*
 * DRV8425.c
 *
 *  Created on: Feb 7, 2026
 *      Author: luckyellasiri
 */

#include "DRV8425.h"
#include <stdio.h>
#include <stdint.h>

//GPIO initialization structs
GPIO_InitTypeDef GPIO_Enable;
GPIO_InitTypeDef GPIO_Dir;
GPIO_InitTypeDef GPIO_Step;


//global ints
static DRV8425_Handle *active_drv = NULL;

// sets the HAL GPIO pin configurations for the stepper motor
void DRV8425_Init(DRV8425_Handle *hdrv) {
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

    hdrv->enable_state = ENABLE_LOW;
    hdrv->direction = FORWARD;
}


void DRV8425_DriveSteps(DRV8425_Handle *hdrv, int32_t steps, uint32_t freq) {
    //setup & command - does not generate steps itself!!!
    //need to define ints at top of file -> steps_remaining int for incrementing and active_drv address

    //make sure there are steps remaining (steps != 0)
    //make sure freq !=0
    //make sure there's a valid driver like motorX -> so you can access dir/step pins
    if (!hdrv || steps == 0 || freq == 0) return;

    if ((freq > MAX_STEP_FREQ) || (freq < MIN_STEP_FREQ)) {
    	return;
    }

    //determine directions -> pos steps = DIR pin 0 = positive/forward CW, DIR pin 1 = negative/reverse CCW
    if(steps>0){
        HAL_GPIO_WritePin(hdrv->dir_port, hdrv->dir_pin, GPIO_PIN_SET); //dir pin -> 1 for pos steps
    } else{
        HAL_GPIO_WritePin(hdrv->dir_port, hdrv->dir_pin, GPIO_PIN_RESET); //dir pin -> 0 for negative steps
        steps = -steps;
    }
    //STEPS = "steps remaining" - so need absolute value because DIR pin determines direction
    //since we've already written the direction pin, can just negate if steps are negative for steps remaining
    // if(steps < 0){
    //     steps_remaining = -steps;

    // }else{
    //     steps_remaining = steps;
    // }

    //set active driver for stepping -> so proper hdrv is accessed by HAL_TIM_periodElapsed
    active_drv = hdrv;



    //configure timing
    //take provided frequency (steps/sec) -> timer period for interrupt
    //need to calculate new autoreload value (how many ticks between steps?)

    // formula: timclk / (psc + 1) = tick frequency
    // prescaler value
    uint32_t psc = hdrv->htim_step->Init.Prescaler;
    uint32_t tim_clk = hdrv->tim_clk_freq; //ASSUME this is connected to APB2 bus, if on APB1 will need to update formula
    uint32_t tick_freq = tim_clk / (psc+1); //how many ticks per second based on timer configuration

    // calculate arr (autoreload for interrupt)
    // -> we want arr to equal ticks/step (one interrupt every step)
    uint32_t arr = tick_freq / freq - 1;
    uint32_t ccr = 970 / (1 / tim_clk);

    //write that value into timer's auto reload register -> so the interrupt occurs every X ticks
    __HAL_TIM_SET_AUTORELOAD(hdrv->htim_step, arr);
    __HAL_TIM_SET_COMPARE(hdrv->htim_step, ccr);
    __HAL_TIM_SET_REPETITION(hdrv->htim_step, steps - 1);
    __HAL_TIM_SET_COUNTER(&htim1, 0);
    __HAL_TIM_GENERATE_EVENT(&htim1, TIM_EVENTSOURCE_UPDATE);

    //ENABLE the motor - make driver active
    HAL_GPIO_WritePin(hdrv->enable_port, hdrv->enable_pin, GPIO_PIN_SET);

    //start the first interrupt
    HAL_TIM_PWM_Start(hdrv->htim_step, hdrv->htim_channel);
    HAL_TIM_Base_Start_IT(hdrv->htim_step);

    //signal for success
    return 0;
}

//interrupt function (configured by drivesteps) -> each interrupt is one STEP pulse (written to STEP pin), decrements remaining step count
//stops timer and disables driver when it runs out of steps
//motor moves to idle state -> could change to Sleep state??
//if drive_steps is called again, the process restarts (direction can change, etc.)

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim){
    //make sure you're accessing the right timer for interrupt
    if(!active_drv) return; //could return named error
    if(htim != active_drv->htim_step) return; //could return named error

    //if no more steps -> stop interrupt, disable & reset driver
    if (htim->Instance == TIM1) {
        HAL_TIM_Base_Stop_IT(htim);
        HAL_TIM_PWM_Stop(active_drv->htim_step, active_drv>htim_channel);
        HAL_GPIO_WritePin(active_drv->enable_port, active_drv->enable_pin, GPIO_PIN_RESET);
        active_drv = NULL;
        return;
    }

}


// Stop motor regardless of steps remaining. -> TURN OFF ENABLE
uint8_t DRV8425_Stop(DRV8425_Handle *hdrv) {
    if(!hdrv) return 1;
    //stop the interrupt
    HAL_TIM_Base_Stop_IT(hdrv->htim_step);
    //cancel remaining steps
    steps_remaining = 0;
    //disable + reset driver
    active_drv = NULL;
    HAL_GPIO_WritePin(hdrv->enable_port, hdrv->enable_pin, GPIO_PIN_RESET);

    return 0;
}

