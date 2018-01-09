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

#include <avr/interrupt.h> // sei()

int main(void)
{
	// ================== Setup hardware ==================
	UART::Configure(500000);
	millis_init();

	// Enable interrupts:
	sei();

	UART::WriteString("BeamBode is alive!\r\n");
	flash_led(3,100);

	// ============== Infinite loop ====================
	while(1)
	{
		// ---- Run scheduled tasks ----------

	}
}
