/* 
 * Function prototypes for the DRV8424/25 stepper motor driver. Defined here are the functions
 * I would expect to be available for a programmer to call to interface with the driver. T
 *
 *
 */
#pragma once
#include <stdint.h>
#include "stm32g4xx_hal.h" // i think this is the correct file but check when you initialize a project

typedef struct {
	TIM_HandleTypeDef *htim_step;
	uint32_t tim_clk_freq;
	GPIO_TypeDef *IN1_port;
	uint16_t IN1_pin;
    GPIO_TypeDef *IN2_port;
	uint16_t IN2_pin;
    GPIO_TypeDef *IN3_port;
	uint16_t IN3_pin;
    GPIO_TypeDef *IN4_port;
	uint16_t IN4_pin;

	volatile int32_t steps_remaining;
	uint8_t index;
	uint8_t direction;
} ULN2003_Handle;

//error codes to be returned by the functions
#define SUCCESS 0x00
#define ENABLE_ALREADY_LOW_ERROR 0x01
#define ABOVE_MAX_FREQ_ERROR 0x02
#define BELOW_MIN_FREQ_ERROR 0x03

// freq limits (depends on stepper motor being used)
// NOTE: for max freq limit it is optimal to not
// operate at max freq as more noise will be generated
#define MAX_FREQ_LIMIT_HZ 500000
#define MIN_FREQ_LIMIT_HZ 0

// do not edit the pattern size 
#define PATTERN_SIZE 4

// defines the possible directions the motor can go 
#define FORWARD 0
#define BACKWARD 1

// Initializes handle for the driver that holds  all hardware details (pwm channel, pin assignments).
void ULN2003_Init(ULN2003_Handle *hdrv);

// Drive the motor a given number of steps with the direction indicated by the sign of param steps, at the given frequency. 
uint8_t ULN2003_DriveSteps(ULN2003_Handle *hdrv, int32_t steps, uint32_t freq); 

// Stop motor regardless of steps remaining.
uint8_t ULN2003_Stop(ULN2003_Handle *hdrv);

// Those are the most important. After that is implemented and tested, consider adding more like sleep mode, microstepping modes, current limiting, and more.
