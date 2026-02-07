
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
    GPIO_Enable.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(hdrv->enable_port, &GPIO_Enable);

    GPIO_Dir.Pin = hdrv->dir_pin;
    GPIO_Dir.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_Dir.Pull = GPIO_NOPULL;
    GPIO_Dir.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(hdrv->dir_port, &GPIO_Dir);

    GPIO_Step.Pin = hdrv->step_pin;
    GPIO_Step.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_Step.Pull = GPIO_NOPULL;
    GPIO_Step.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(hdrv->step_port, &GPIO_Step);

    enable_state = ENABLE_LOW;
    steps_remaining = 0;
    direction = FORWARD;
}


uint8_t DRV8425_DriveSteps(DRV_8425_Handle *hdrv, int32_t steps, uint32_t freq) {
    //setup & command - does not generate steps itself!!!
   
    //make sure there are steps remaining (steps != 0)
    //make sure freq !=0
    //make sure there's a valid driver like motorX -> so you can access dir/step pins
    if(!hdrv || steps == 0 || freq == 0) return 1;

    //determine directions -> pos steps = DIR pin 0 = positive/forward CW, DIR pin 1 = negative/reverse CCW
    if(steps>0){
        direction = FORWARD;
        HAL_GPIO_WritePin(hdrv->dir_port, hdrv->dir_pin, GPIO_PIN_SET); //dir pin -> 1 for pos steps
    } else{
        direction = BACKWARD;
        HAL_GPIO_WritePin(hdrv->dir_port, hdrv->dir_pin, GPIO_PIN_RESET); //dir pin -> 0 for negative steps
    }
    //STEPS = "steps remaining" - so need absolute value because DIR pin determines direction
    //since we've already written the direction pin, can just negate if steps are negative for steps remaining
    if(steps < 0){
        steps_remaining = -steps;
    }else{
        steps_remaining = steps;
    }

    //set active driver for stepping -> so proper hdrv is accessed by HAL_TIM_periodElapsed
    active_drv = hdrv;

    //configure timing
    //take provided frequency (steps/sec) -> timer period for interrupt
    //need to calculate new autoreload value (how many ticks between steps?)
    
    // formula: timclk / (psc + 1) = tick frequency
    // prescaler value
    uint32_t psc = hdrv->htim_step->Init.Prescaler;
    // uint32_t tim_clk = HAL_RCC_GetPCLK1Freq(); //ASSUME this is connected to APB2 bus, if on APB1 will need to update formula 
    uint32_t tick_freq = hdrv->tim_clk_freq / (psc+1); //how many ticks per second based on user provided clock freq

    // calculate arr (autoreload for interrupt)
    // -> we want arr to equal ticks/step (one interrupt every step)
    uint32_t arr = (tick_freq / freq) - 1;


    //write that value into timer's auto reload register -> so the interrupt occurs every X ticks
    __HAL_TIM_SET_AUTORELOAD(hdrv->htim_step, arr);

    //ENABLE the motor - make driver active
    HAL_GPIO_WritePin(hdrv->enable_port, hdrv->enable_pin, GPIO_PIN_SET);

    //start the first interrupt
    HAL_TIM_Base_Start_IT(hdrv->htim_step);

    //signal for success
    return 0;
}

//interrupt function (configured by drivesteps) -> each interrupt is one STEP pulse (written to STEP pin), decrements remaining step count
//stops timer and disables driver when it runs out of steps
//motor moves to idle state -> could change to Sleep state??
//if drive_steps is called again, the process restarts (direction can change, etc.)

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim){
    
    //if no more steps -> stop interrupt, disable & reset driver
    if(steps_remaining == 0){
        HAL_TIM_Base_Stop_IT(htim);
        HAL_GPIO_WritePin(active_drv->enable_port, active_drv->enable_pin, GPIO_PIN_RESET);
        enable_state = ENABLE_LOW;
        active_drv = NULL;
    }

    if(steps_remaining > 0) {
        //ONE STEP!
        HAL_GPIO_WritePin(active_drv->step_port, active_drv->step_pin, GPIO_PIN_SET);
        //tWH(STEP) ≥ 970 ns (in datasheet)
        //TODO: ADD FLAG FOR DELAY between step set and reset - need to think abt how to implement this
        HAL_GPIO_WritePin(active_drv->step_port, active_drv->step_pin, GPIO_PIN_RESET);
    }

    steps_remaining--;
}


// Stop motor regardless of steps remaining. -> TURN OFF ENABLE
uint8_t DRV8425_Stop(DRV8425_Handle *hdrv) {

    //TODO: implement stop flag

    if(!hdrv) return 1;
    
    if(enable_state == ENABLE_LOW) return ENABLE_ALREADY_LOW_ERROR;
    
    //stop the interrupt
    HAL_TIM_Base_Stop_IT(hdrv->htim_step);
    //cancel remaining steps
    steps_remaining = 0;
    //disable + reset driver
    active_drv = NULL;
    HAL_GPIO_WritePin(hdrv->enable_port, hdrv->enable_pin, GPIO_PIN_RESET);

    return 0;
}
