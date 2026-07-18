#include <stdint.h>
#include "stm32f303xc.h"
#include "timer.h"

// store a pointer to the function that is called when a button is pressed
// set a default value of NULL so that it won't be called until the function pointer is defined
// button press is currently set up to accept one integer argument

// enable the clocks for desired peripherals (GPIOA, C and E)
void enable_clocks() {
	RCC->AHBENR |= RCC_AHBENR_GPIOAEN | RCC_AHBENR_GPIOCEN | RCC_AHBENR_GPIOEEN;

	//enable timer clock
	RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;
}


//initialise the discovery board I/O (just outputs: inputs are selected by default)
void initialise_board() {
	// get a pointer to the second half word of the MODER register (for outputs pe8-15)
	uint16_t *led_output_registers = ((uint16_t *)&(GPIOE->MODER)) + 1;
	*led_output_registers = 0x5555;
}



void set_led(int led_value){

	uint8_t *led_register = ((uint8_t*)&(GPIOE->ODR)) + 1; //access LED register

	*led_register = led_value; //set LED register
}


void start_timer_ms(void) {
    // 8 MHz (or whatever your TIM2 clock is) / (PSC+1) → 1 kHz
    TIM2->PSC = 48000 - 1;
    TIM2->EGR = TIM_EGR_UG;     // force prescaler & ARR to shadow
    TIM2->CNT = 0;              // reset count
    TIM2->CR1 |= TIM_CR1_CEN;   // enable timer
}

int check_timer(int timer_interval){
	//if timer elapsed, return 1
	if (TIM2->CNT >= timer_interval){
		return 1;
	}

	//else, return 0
	return 0;

}


void delay(int delay_ms){
	//start timer
	start_timer_ms();

	//loop and check if timer has elapsed
	while (1){
		if (check_timer(delay_ms)){
			return;
		}
	}
}


