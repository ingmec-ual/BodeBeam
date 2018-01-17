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
#include "libclaraquino/mod_quad_encoder.h"
#include "libclaraquino/modules/imu_lsm9ds1/SparkFunLSM9DS1.h"

#include <stdio.h>  // sprintf()

#include <avr/interrupt.h> // sei()

// ===========  PIN CONFIGURATION ===================
const uint8_t PIN_BTN_UP = 0x10;    // A0
const uint8_t PIN_BTN_DOWN = 0x11;  // A1
const uint8_t PIN_ENCODER_A = 0x42; // INT0=PD2
const uint8_t PIN_IMU_CS_AG = 0x15; // A5
const uint8_t PIN_IMU_CS_M = 0x16;  // A6

const uint8_t ENCODER_TICKS_PER_REV = 16;
// ==================================================

const uint32_t PERIOD_ESTIM_FREQ_MS_TH = 300*10;


int main(void)
{
	// ================== Setup hardware ==================
	UART::Configure(500000);
	millis_init();

	// OC0A=PIN PB3 (=0x23): pwm output
	gpio_pin_mode(0x23,OUTPUT);
	pwm_init(PWM_TIMER0,PWM_PRESCALER_1);

	int16_t pwm_val = 10;

	gpio_pin_mode(PIN_BTN_UP, INPUT_PULLUP);
	gpio_pin_mode(PIN_BTN_DOWN, INPUT_PULLUP);

	gpio_pin_mode(PIN_ENCODER_A, INPUT_PULLUP);

	mod_quad_encoder_init(0,PIN_ENCODER_A,0);


	LSM9DS1 imu;
	imu.settings.device.commInterface = IMU_MODE_SPI;
	imu.settings.device.mAddress = PIN_IMU_CS_M;
	imu.settings.device.agAddress = PIN_IMU_CS_AG;

	imu.begin();

	// Enable interrupts:
	sei();

	UART::WriteString("Hi there! BeamBode is alive ;-)\r\n");
	flash_led(3,100);

	uint32_t tim_last_freq_estim = 0;
	uint32_t last_freq_estim_encoder = 0;
	float   freq = .0f; // Estimated rotor freq

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

		// Set PWM output:
		pwm_set_duty_cycle(PWM_TIMER0, PWM_PIN_OCnA, pwm_val);

		// Rotor freq estimation:
		const uint32_t t_now = millis();
		if (t_now-tim_last_freq_estim>PERIOD_ESTIM_FREQ_MS_TH)
		{
			cli();
			const uint32_t encoder_now = ENC_STATUS[0].COUNTER;
			sei();

			freq = (encoder_now-last_freq_estim_encoder)/float(ENCODER_TICKS_PER_REV*PERIOD_ESTIM_FREQ_MS_TH*0.0001f);

			tim_last_freq_estim = t_now;
			last_freq_estim_encoder = encoder_now;
		}

		// Read IMU:
		imu.readAccel();
		


		// USB output:
		char str[60];
		int freq_unit = freq;
		int freq_cents = freq*uint32_t(1000) - uint32_t(freq_unit)*1000;
		
		sprintf(str,"%ld %i %li %d.%03d %d\r\n",t_now, pwm_val, ENC_STATUS[0].COUNTER, freq_unit, freq_cents,imu.az);
		UART::WriteString(str);

		delay_ms(40);
	}
}
