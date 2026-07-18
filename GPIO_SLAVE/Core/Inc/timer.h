#ifndef LED_INTERRUPTS_H
#define LED_INTERRUPTS_H

#include <stdint.h>
#include "stm32f303xc.h"

// enable the clocks for desired peripherals (GPIOA, C and E)
void enable_clocks();

//initialise the discovery board I/O (just outputs: inputs are selected by default)
void initialise_board();

void enable_interrupt();

//function to set LEDs to specified bitmask
void set_led(int led_value);


//function to start timer of specified duration
void start_timer_ms();

//function to check if timer has elapsed: returns 0/1
int check_timer(int timer_interval);

//function to delay by specified amount in ms
void delay(int delay_ms);

#endif

