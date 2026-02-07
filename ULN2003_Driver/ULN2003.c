/*
 * ULN2003.c
 *
 *  Created on: Feb 3, 2026
 *      Author: lyellasiri001 & mflessa
 */

#include "ULN2003.h"
#include <stdio.h>
#include <stdint.h>

//GPIO initialization structs
GPIO_InitTypeDef GPIO_IN1;
GPIO_InitTypeDef GPIO_IN2;
GPIO_InitTypeDef GPIO_IN3;
GPIO_InitTypeDef GPIO_IN4;

//global ints
static ULN2003_Handle *active_drv = NULL;
static uint8_t pattern[PATTERN_SIZE] = {0x0C, 0x06, 0x03, 0x09};

// sets the HAL GPIO pin configurations for the stepper motor
void ULN2003_Init(ULN2003_Handle *hdrv) {
    GPIO_IN1.Pin = hdrv->IN1_pin;
    GPIO_IN1.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_IN1.Pull = GPIO_NOPULL;
    GPIO_IN1.Speed = GPIO_SPEED_FREQ_MEDIUM;
    HAL_GPIO_Init(hdrv->IN1_port, &GPIO_IN1);

    GPIO_IN2.Pin = hdrv->IN2_pin;
    GPIO_IN2.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_IN2.Pull = GPIO_NOPULL;
    GPIO_IN2.Speed = GPIO_SPEED_FREQ_MEDIUM;
    HAL_GPIO_Init(hdrv->IN2_port, &GPIO_IN2);

    GPIO_IN3.Pin = hdrv->IN3_pin;
    GPIO_IN3.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_IN3.Pull = GPIO_NOPULL;
    GPIO_IN3.Speed = GPIO_SPEED_FREQ_MEDIUM;
    HAL_GPIO_Init(hdrv->IN3_port, &GPIO_IN3);

    GPIO_IN4.Pin = hdrv->IN4_pin;
    GPIO_IN4.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_IN4.Pull = GPIO_NOPULL;
    GPIO_IN4.Speed = GPIO_SPEED_FREQ_MEDIUM;
    HAL_GPIO_Init(hdrv->IN4_port, &GPIO_IN4);

}

uint8_t ULN2003_DriveSteps(ULN2003_Handle *hdrv, int32_t steps, uint32_t freq) {
    //setup & command - does not generate steps itself!!!
    //need to define ints at top of file -> steps_remaining int for incrementing and active_drv address

    //make sure there are steps remaining (steps != 0)
    //make sure freq !=0
    //make sure there's a valid driver like motorX -> so you can access dir/step pins
    if (!hdrv || steps == 0 || freq == 0) return 1;

    //STEPS = "steps remaining" - so need absolute value because DIR pin determines direction
    //since we've already written the direction pin, can just negate if steps are negative for steps remaining
    if(steps < 0){
        direction = BACKWARD;
        steps_remaining = -steps;
    }else{
        steps_remaining = steps;
    }

    //set active driver for stepping -> so proper hdrv is accedded by HAL_TIM_periodElapsed
    active_drv = hdrv;

    // formula: timclk / (psc + 1) = tick frequency
       // prescaler value
    uint32_t psc = hdrv->htim_step->Init.Prescaler;
    //uint32_t tim_clk = HAL_RCC_GetPCLK2Freq(); //ASSUME this is connected to APB2 bus, if on APB1 will need to update formula
    uint32_t tick_freq = hdrv->tim_clk_freq / (psc+1); //how many ticks per second based on timer configuration

       // calculate arr (autoreload for interrupt)
       // -> we want arr to equal ticks/step (one interrupt every step)
    uint32_t arr = (tick_freq / freq) - 1;

       //write that value into timer's auto reload register -> so the interrupt occurs every X ticks
    __HAL_TIM_SET_AUTORELOAD(hdrv->htim_step, arr);

    HAL_TIM_Base_Start_IT(hdrv->htim_step);

    //configure timing
    //take provided frequency (steps/sec) -> timer period for interrupt
    //need to calculate new autoreload value (how many ticks between steps?)

    //signal for success
    return 0;
}

//interrupt function (configured by drivesteps) -> each interrupt is one STEP pulse (written to STEP pin), decrements remaining step count
//stops timer and disables driver when it runs out of steps
//motor moves to idle state -> could change to Sleep state??
//if drive_steps is called again, the process restarts (direction can change, etc.)

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim){
    //make sure you're accessing the right timer for interrupt

    //if no more steps -> stop interrupt, disable & reset driver
    if(steps_remaining == 0){
        HAL_TIM_Base_Stop_IT(htim);
        active_drv = NULL;
    }


    if (steps_remaining > 0) {
            HAL_GPIO_WritePin(active_drv->IN1_port, active_drv->IN1_pin, pattern[index] & 0x01);
            HAL_GPIO_WritePin(active_drv->IN2_port, active_drv->IN2_pin, pattern[index] & 0x02);
            HAL_GPIO_WritePin(active_drv->IN3_port, active_drv->IN3_pin, pattern[index] & 0x04);
            HAL_GPIO_WritePin(active_drv->IN4_port, active_drv->IN4_pin, pattern[index] & 0x08);

            if (direction == FORWARD) {
                index = (index + 1 + PATTERN_SIZE) % PATTERN_SIZE;
            } else {
                index = (index - 1 + PATTERN_SIZE) % PATTERN_SIZE;
            }
    }

    steps_remaining--;
}


// Stop motor regardless of steps remaining. -> TURN OFF ENABLE
uint8_t ULN2003_Stop(ULN2003_Handle *hdrv) {
    if(!hdrv) return 1;
    //stop the interrupt
    HAL_TIM_Base_Stop_IT(hdrv->htim_step);
    //cancel remaining steps
    steps_remaining = 0;
    //disable + reset driver
    active_drv = NULL;
    return 0;
}


