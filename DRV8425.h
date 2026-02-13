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
	//add freq in handle to avoid hardware config access missteps
	uint32_t tim_clk_freq;
	GPIO_TypeDef *dir_port;
	uint16_t dir_pin;
	GPIO_TypeDef *enable_port;
	uint16_t enable_pin;
	GPIO_TypeDef *step_port;
	uint16_t step_pin;

	volatile int32_t steps_remaining;
	uint8_t index;
	uint8_t direction;
	uint8_t enable_state;

	//TODO: stop flag that indicates the instant the stop button is pressed (driving stops immediately)
	//TODO: add condition in stop function??
} DRV8425_Handle;

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

// enable pin states
#define ENABLE_HIGH 0x01
#define ENABLE_LOW 0x00

// direction states
#define FORWARD 0x01
#define BACKWARD 0x00

// Initializes handle for the driver that holds  all hardware details (pwm channel, pin assignments).
void DRV8425_Init(DRV8425_Handle *hdrv);

// Drive the motor a given number of steps with the direction indicated by the sign of param steps, at the given frequency. 
uint8_t DRV8425_DriveSteps(DRV8425_Handle *hdrv, int32_t steps, uint32_t freq); 

// Stop motor regardless of steps remaining.
uint8_t DRV8425_Stop(DRV8425_Handle *hdrv);

// Those are the most important. After that is implemented and tested, consider adding more like sleep mode, microstepping modes, current limiting, and more.
