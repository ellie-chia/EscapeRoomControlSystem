#include "serial.h"
#include "timer.h"

#include "stm32f303xc.h"

// We store the pointers to the GPIO and USART that are used
//  for a specific serial port. To add another serial port
//  you need to select the appropriate values.
struct _SerialPort {
	USART_TypeDef *UART;
	GPIO_TypeDef *GPIO;
	volatile uint32_t MaskAPB2ENR;	// mask to enable RCC APB2 bus registers
	volatile uint32_t MaskAPB1ENR;	// mask to enable RCC APB1 bus registers
	volatile uint32_t MaskAHBENR;	// mask to enable RCC AHB bus registers
	volatile uint32_t SerialPinModeValue;
	volatile uint32_t SerialPinSpeedValue;
	volatile uint32_t SerialPinAlternatePinValueLow;
	volatile uint32_t SerialPinAlternatePinValueHigh;
	void (*completion_function)(uint32_t);
};



// instantiate the serial port parameters
//   note: the complexity is hidden in the c file
SerialPort USART1_PORT = {USART1,
		GPIOC,
		RCC_APB2ENR_USART1EN, // bit to enable for APB2 bus
		0x00,	// bit to enable for APB1 bus
		RCC_AHBENR_GPIOCEN, // bit to enable for AHB bus
		0xA00,
		0xF00,
		0x770000,  // for USART1 PC10 and 11, this is in the AFR low register
		0x00, // no change to the high alternate function register
		0x00 // default function pointer is NULL
		};

SerialPort USART2_PORT = {
    USART2,
    GPIOA,
    0x00,                    // No APB2 enable for USART2 (it's on APB1)
    RCC_APB1ENR_USART2EN,    // Enable USART2 clock on APB1 bus
    RCC_AHBENR_GPIOAEN,      // Enable GPIOA clock on AHB bus
    0xA800,                  // GPIOA MODER for pins PA2 and PA3 as Alternate Function (AF)
                             // 0xA800 in binary: 1010 1000 0000 0000 (pins 2 and 3 set to AF mode)
    0xF000,                  // GPIOA OSPEEDR for pins PA2 and PA3 set to high speed
    0x7700,                  // GPIOA AFRL (AFR[0]) for PA2 and PA3 set to AF7 (USART2)
                             // Each pin has 4 bits in AFRL: 0x7 << (4 * pin number)
    0x00,                    // AFRH (AFR[1]) no pins above 7 used
    0x00                     // default completion function pointer NULL
};

void finished_transmission(uint32_t bytes_sent){
}

// InitialiseSerial - Initialise the serial port
// Input: baudRate is from an enumerated set
void SerialInitialise(uint32_t baudRate, SerialPort *serial_port, void (*completion_function)(uint32_t)) {

	serial_port->completion_function = completion_function;

	// enable clock power, system configuration clock and GPIOC
	// common to all UARTs
	RCC->APB1ENR |= RCC_APB1ENR_PWREN;
	RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN;

	// enable the GPIO which is on the AHB bus
	RCC->AHBENR |= serial_port->MaskAHBENR;

	// set pin mode to alternate function for the specific GPIO pins
	// Clear only the required pins for the selected USART
	if (serial_port->GPIO == GPIOA) {
	    // PA2 (TX), PA3 (RX)
	    serial_port->GPIO->MODER &= ~((0x3 << (2 * 2)) | (0x3 << (2 * 3)));
	} else if (serial_port->GPIO == GPIOC) {
	    // PC10 (TX), PC11 (RX)
	    serial_port->GPIO->MODER &= ~((0x3 << (2 * 10)) | (0x3 << (2 * 11)));
	}

	// Apply configured mode
	serial_port->GPIO->MODER |= serial_port->SerialPinModeValue;

	// enable high speed clock for specific GPIO pins
	serial_port->GPIO->OSPEEDR = serial_port->SerialPinSpeedValue;

	// set alternate function to enable USART to external pins
	serial_port->GPIO->AFR[0] |= serial_port->SerialPinAlternatePinValueLow;
	serial_port->GPIO->AFR[1] |= serial_port->SerialPinAlternatePinValueHigh;

	// enable the device based on the bits defined in the serial port definition
	RCC->APB1ENR |= serial_port->MaskAPB1ENR;
	RCC->APB2ENR |= serial_port->MaskAPB2ENR;

	// Get a pointer to the 16 bits of the BRR register that we want to change
	uint16_t *baud_rate_config = (uint16_t*)&serial_port->UART->BRR; // only 16 bits used!

	// Baud rate calculation from datasheet
	switch(baudRate){
	case BAUD_9600:
		// NEED TO FIX THIS !
		*baud_rate_config = 417;  // 115200 at 8MHz
		break;
	case BAUD_19200:
		// NEED TO FIX THIS !
		*baud_rate_config = 417;  // 115200 at 8MHz
		break;
	case BAUD_38400:
		// NEED TO FIX THIS !
		*baud_rate_config = 417;  // 115200 at 8MHz
		break;
	case BAUD_57600:
		// NEED TO FIX THIS !
		*baud_rate_config = 417;  // 115200 at 8MHz
		break;
	case BAUD_115200:
		*baud_rate_config = 417;  // 115200 at 8MHz
		break;
	}


	// enable serial port for tx and rx
	serial_port->UART->CR1 |= USART_CR1_TE | USART_CR1_RE | USART_CR1_UE;

}

void SerialInitialiseReceiver(void) {
    SerialInitialise(BAUD_115200, &USART2_PORT, &finished_transmission);

    // Enable RX interrupt
    USART2_PORT.UART->CR1 |= USART_CR1_RXNEIE;

    // Enable USART2 interrupt in NVIC
    NVIC_EnableIRQ(USART2_IRQn);
}


#define RX_BUFFER_SIZE 64
volatile char rx_buffer[RX_BUFFER_SIZE];
volatile uint8_t message_ready = 0;
volatile uint32_t rx_index = 0;

void USART2_IRQHandler(void) {

	set_led(0b10101010);
    if (USART2->ISR & USART_ISR_RXNE) {
        char received = USART2->RDR;  // Read received byte, clears RXNE flag

        if (rx_index < RX_BUFFER_SIZE - 1) {
            rx_buffer[rx_index++] = received;
            if (received == '\n') {   // End of message
                rx_buffer[rx_index] = '\0';  // Null-terminate string
                rx_index = 0;
                message_ready = 1;  // Signal main loop a message is ready
            }
        } else {
            rx_index = 0;  // Buffer overflow, reset index
        }
    }
}


void SerialOutputChar(uint8_t data, SerialPort *serial_port) {
    while ((serial_port->UART->ISR & USART_ISR_TXE) == 0);  // Wait for TX buffer empty
    serial_port->UART->TDR = data;
    while ((serial_port->UART->ISR & USART_ISR_TC) == 0);   // Wait for transmission complete
}


void SerialOutputString(uint8_t *pt, SerialPort *serial_port) {

	uint32_t counter = 0;
	while(*pt) {
		SerialOutputChar(*pt, serial_port);
		counter++;
		pt++;
	}

	serial_port->completion_function(counter);
}
