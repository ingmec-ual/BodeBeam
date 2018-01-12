/*****************************************************************************************
 FILE: main.cpp
 PURPOSE: Main entry point of the BeamBode firmware

Jose Luis Blanco Claraco (C) 2018
Universidad de Almeria
****************************************************************************************/

#include "libclaraquino/uart.h"
#include "libclaraquino/leds.h"
#include "libclaraquino/delays.h"
#include "libclaraquino/millis_timer.h"
#include "libclaraquino/pwm.h"
#include "libclaraquino/gpio.h"

#include <avr/interrupt.h> // sei()

int main(void)
{
	// ================== Setup hardware ==================
	UART::Configure(500000);
	millis_init();

	// OC0A=PIN PB3 (=0x23): pwm output
	gpio_pin_mode(0x23,OUTPUT);
	pwm_init(PWM_TIMER0,PWM_PRESCALER_1);

	int16_t pwm_val = 10;

	const uint8_t PIN_BTN_UP = 0x10;  // A0
	const uint8_t PIN_BTN_DOWN = 0x11;  // A1

	gpio_pin_mode(PIN_BTN_UP, INPUT_PULLUP);
	gpio_pin_mode(PIN_BTN_DOWN, INPUT_PULLUP);

	// Enable interrupts:
	sei();

	UART::WriteString("BeamBode is alive!\r\n");
	flash_led(3,100);

	// ============== Infinite loop ====================
	while(1)
	{
		// ---- Run scheduled tasks ----------
		if (!gpio_pin_read(PIN_BTN_UP))
		{
			++pwm_val;
			if (pwm_val>255) pwm_val= 255;
		}
		if (!gpio_pin_read(PIN_BTN_DOWN))
		{
			--pwm_val;
			if (pwm_val<0) pwm_val= 0;
		}

		pwm_set_duty_cycle(PWM_TIMER0, PWM_PIN_OCnA, pwm_val);


		delay_ms(100);
	}
}
