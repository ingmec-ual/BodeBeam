﻿/*****************************************************************************************
 FILE: main.cpp
 PURPOSE: Main entry point of the BeamBode firmware

Jose Luis Blanco Claraco (C) 2018
Universidad de Almeria
****************************************************************************************/

#include <libclaraquino/uart.h>
#include <libclaraquino/leds.h>
#include <libclaraquino/delays.h>
#include <libclaraquino/millis_timer.h>
#include <libclaraquino/pwm.h>
#include <libclaraquino/gpio.h>
#include <libclaraquino/mod_quad_encoder.h>
#include <libclaraquino/modules/imu_lsm9ds1/SparkFunLSM9DS1.h>
#include <libclaraquino/modules/lcd/LiquidCrystal_I2C.h>

#include <stdio.h>  // sprintf()

#include <avr/interrupt.h> // sei()

// ===========  PIN CONFIGURATION ===================
const uint8_t PIN_BTN_UP     = 0x10;    // A0
const uint8_t PIN_BTN_DOWN   = 0x11;  // A1
const uint8_t PIN_BTN_OK     = 0x12;  // A2
const uint8_t PIN_BTN_CANCEL = 0x13;  // A3

const uint8_t PIN_ENCODER_A   = 0x42; // INT0=PD2
const uint8_t ENCODER_TICKS_PER_REV = 16;
// ==================================================

const uint32_t PERIOD_ESTIM_FREQ_MS_TH = 250*10;

struct XYZd
{
	XYZd() : x(0),y(0),z(0) {}
	double x,y,z;
	void keep_max_with(const XYZd &o)
	{
		if (o.x>x) x=o.x;
		if (o.y>y) y=o.y;
		if (o.z>z) z=o.z;
	}
	void operator += (const XYZd &o)
	{
		x+=o.x;
		y+=o.y;
		z+=o.z;
	}
	void operator *= (const double k)
	{
		x*=k;
		y*=k;
		z*=k;
	}
};

void ejecutar_modo_manual();
void ejecutar_barrido_USB();

LSM9DS1 imu;
LiquidCrystal_I2C lcd(0x3F,16,2);


volatile uint32_t tim2_sample_time = 0;
ISR(TIMER2_COMPA_vect)
{
	tim2_sample_time = millis();
}

void setup_tim2_interrupts()
{
	// Timer2: 8bits,
	// prescaler = 1024;
	// 1 overflow with compare = (1+n): prescaler* (1+n) / FREQ_CPU = period
	// Fs = 20e6/(2*1024*(1+OCR2A))
	// 5ms period compare value:
	OCR2A = 75; // Fs = 128.5 Hz

	// Mode 2: WGM21=1 (clear on compare)
	TCCR2B = (1<<WGM21) | (4 /*prescaler*/);
}

void enable_tim2_interrupts()
{
	TIMSK2 |= (1<<OCIE2A);
}
void disable_tim2_interrupts()
{
	TIMSK2 &= ~(1<<OCIE2A);
}

void read_imu_and_avrg_peak_values(XYZd &imu_pk)
{
	// Read IMU:
	const uint8_t imu_samples = imu.getFIFOSamples();

	// Find peak values:
	uint16_t imu_ax_pk = 0, imu_ay_pk = 0 , imu_az_pk = 0;
	for(uint8_t ii = 0; ii < imu_samples ; ii++)
	{
		imu.readAccel();
		const uint16_t ax_abs = imu.ax>0 ? imu.ax : -imu.ax;
		const uint16_t ay_abs = imu.ay>0 ? imu.ay : -imu.ay;
		const uint16_t az_abs = imu.az>0 ? imu.az : -imu.az;
		if (ax_abs>imu_ax_pk) imu_ax_pk=ax_abs;
		if (ay_abs>imu_ay_pk) imu_ay_pk=ay_abs;
		if (az_abs>imu_az_pk) imu_az_pk=az_abs;
	}
	imu_pk.x = 9.81*imu.calcAccel(imu_ax_pk);
	imu_pk.y = 9.81*imu.calcAccel(imu_ay_pk);
	imu_pk.z = 9.81*(imu.calcAccel(imu_az_pk) - 1.0 /* remove gravity */);
	if (imu_pk.z<0) imu_pk.z=-imu_pk.z;
}


int main(void)
{
	// ================== Setup hardware ==================
	UART::Configure(500000);
	millis_init();

	setup_tim2_interrupts();

	UART::WriteString("% Hi there! BeamBode is alive ;-)\r\n");
	flash_led(3,100);

	// OC0A=PIN PB3 (=0x23): pwm output
	gpio_pin_mode(0x23,OUTPUT);
	pwm_init(PWM_TIMER0,PWM_PRESCALER_1);

	gpio_pin_mode(PIN_BTN_UP, INPUT_PULLUP);
	gpio_pin_mode(PIN_BTN_DOWN, INPUT_PULLUP);
	gpio_pin_mode(PIN_BTN_OK, INPUT_PULLUP);
	gpio_pin_mode(PIN_BTN_CANCEL, INPUT_PULLUP);

	gpio_pin_mode(PIN_ENCODER_A, INPUT_PULLUP);

	mod_quad_encoder_init(0,PIN_ENCODER_A,0);

	// Enable interrupts:
	sei();

	// LCD must be initialized *after* sei()
	lcd.begin();

	// Welcome screen:
	lcd.setCursor(0,0);
	lcd.write("=== Bode Beam ===");
	lcd.setCursor(0,1);
	lcd.write("    UAL (C) 2018");
	delay_ms(1000);
	lcd.clear();


	// Setup IMU:
	imu.settings.device.commInterface = IMU_MODE_I2C;
	imu.settings.device.mAddress = LSM9DS1_M_ADDR(1);  // SDO_M pulled-up
	imu.settings.device.agAddress = LSM9DS1_AG_ADDR(1);// SDO_AG pulled-up

	// accel sample rate can be 1-6
	// 1 = 10 Hz    4 = 238 Hz
	// 2 = 50 Hz    5 = 476 Hz
	// 3 = 119 Hz   6 = 952 Hz
	imu.settings.accel.sampleRate = 6;
//	imu.settings.accel.bandwidth = A_ABW_105; // Anti aliasing filter= 105 Hz

	// accel scale can be 2, 4, 8, or 16 (g)
	imu.settings.accel.scale = 16;
	lcd.clear();
	lcd.setCursor(0,0);
	lcd.write("Calib. IMU...");
	delay_ms(100);
	if (!imu.begin())
	{
		lcd.setCursor(0,1);
		lcd.write("IMU error!");
		while (1) {}
	}
	// Do self-calibration to remove gravity vector.
	imu.calibrate(true);

	imu.setAccelODR(XL_ODR_50);

	lcd.setCursor(0,1);
	lcd.write("Hecho!");
	delay_ms(250);

	// ====== Main menu ============
	const char * menu_entries[] = {
		"1. Modo manual  ",
		"2. Barrido freq."
	  //"                "
	};
	using func_modo = void (*)();
	const func_modo menu_funcs[] = {
		&ejecutar_modo_manual,
		&ejecutar_barrido_USB,
	};
	const uint8_t num_menu_entries = sizeof(menu_entries)/sizeof(menu_entries[0]);

	int8_t selected_mnu = 0;

	for (;;)
	{
		lcd.setCursor(0,0);
		lcd.write("Escoja y OK:");
		
		lcd.setCursor(0,1);
		lcd.write(menu_entries[selected_mnu]);

		if (!gpio_pin_read(PIN_BTN_UP))
		{
			selected_mnu--;
			if (selected_mnu<0)
				selected_mnu=num_menu_entries-1;
		}
		if (!gpio_pin_read(PIN_BTN_DOWN))
		{
			selected_mnu++;
			if (selected_mnu>=num_menu_entries)
				selected_mnu=0;
		}
		if (!gpio_pin_read(PIN_BTN_CANCEL))
		{
			selected_mnu=0;
		}
		if (!gpio_pin_read(PIN_BTN_OK))
		{
			(menu_funcs[selected_mnu])();
			lcd.clear();
		}
		delay_ms(200);
	}

} // end main()

uint32_t tim_last_freq_estim = 0;
uint32_t last_freq_estim_encoder = 0;
double   freq = .0; // Estimated rotor freq
	
// average accelerations:
XYZd     imu_avr_pk, imu_avr_pk_tmp;
uint8_t imu_avr_cnt = 0;
uint8_t PERIOD_ESTIMATE_IMU_AVERAGE = 40;

void ejecutar_modo_manual()
{
	PERIOD_ESTIMATE_IMU_AVERAGE = 40;

	lcd.clear();
	lcd.write("Modo manual...");
	delay_ms(1500);
	lcd.clear();

	imu.enableFIFO(true);
	imu.setFIFO(FIFO_CONT, 0x1F);

	int16_t pwm_val = 0;
	char str[100];

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
		if (!gpio_pin_read(PIN_BTN_CANCEL))
		{
			return;
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

			const int32_t Aenc = int32_t(encoder_now)-int32_t(last_freq_estim_encoder);
			const double K = ENCODER_TICKS_PER_REV*PERIOD_ESTIM_FREQ_MS_TH*0.0001;
			freq = double(Aenc)/K;

			tim_last_freq_estim = t_now;
			last_freq_estim_encoder = encoder_now;
		}

		// Read IMU:
		XYZd imu_pk;
		read_imu_and_avrg_peak_values(imu_pk);

		// Average peak values:
		imu_avr_pk_tmp.keep_max_with(imu_pk);
		if (++imu_avr_cnt>PERIOD_ESTIMATE_IMU_AVERAGE)
		{
			imu_avr_pk = imu_avr_pk_tmp;
			imu_avr_pk_tmp= XYZd();
			imu_avr_cnt=0;
		}
		// LCD output ==============
			
		// calc PWM as percentage:
		{
			const float pwm_pc = (pwm_val *100) / 255.0f;
			int pwm_unit = (int)pwm_pc;
			//int pwm_cents = pwm_pc*uint16_t(10) - uint16_t(pwm_unit)*10;
			//sprintf(str_pwm,"P=%3d.%01d%%",pwm_unit,pwm_cents);
			sprintf(str,"P=%3d%%",pwm_unit);
		}
		lcd.setCursor(0,0);
		lcd.write(str);

		{
			int freq_unit = (int)freq;
			int freq_cents = freq*uint32_t(10) - uint32_t(freq_unit)*10;
			sprintf(str,"F=%02d.%01dHz  ",freq_unit, freq_cents);
		}
		lcd.setCursor(7,0);
		lcd.write(str);

		// Accel:
		{
			int32_t a_unit = (int32_t)imu_avr_pk.z;
			int32_t a_cents = imu_avr_pk.z*int32_t(100) - int32_t(a_unit)*100;
			sprintf(str,"Apk=%3ld.%02ld m/s2  ",a_unit,a_cents);
		}
		lcd.setCursor(0,1);
		lcd.write(str);
			
		// USB output mode?
		if (!gpio_pin_read(PIN_BTN_OK))
		{
			lcd.clear();
			lcd.setCursor(0,0);
			lcd.write("*Grabando USB...*");
			lcd.setCursor(0,1);
			lcd.write(" x para terminar");

			//imu.enableFIFO(true);
			//imu.setFIFO(FIFO_CONT,0x1F);
			imu.enableFIFO(false);
			imu.setFIFO(FIFO_OFF,0);

			enable_tim2_interrupts();

			uint32_t last_sample_tim = 0;
			uint8_t cnt_check_esc_btn = 0;
			XYZd imu_accs;
			for (;;)
			{
				uint32_t t_new;
				cli();
				t_new = tim2_sample_time;
				sei();
				// Do we have a new sample?
				if (last_sample_tim!=t_new)
				{
					last_sample_tim = t_new;
					imu.readAccel();
					imu_accs.x = 9.81*imu.calcAccel(imu.ax);
					imu_accs.y = 9.81*imu.calcAccel(imu.ay);
					imu_accs.z = 9.81*(imu.calcAccel(imu.az) - 1.0 ); // remove gravity
					int32_t ax_cents = imu_accs.x*1000;
					int32_t ay_cents = imu_accs.y*1000;
					int32_t az_cents = imu_accs.z*1000;
					sprintf(str,"%ld %ld %ld %ld\r\n", t_new,ax_cents,ay_cents,az_cents);
					UART::WriteString(str);
				}

				if (++cnt_check_esc_btn==0)
				{
					if (!gpio_pin_read(PIN_BTN_CANCEL))
					break;
				}
			}
			disable_tim2_interrupts();
			imu.enableFIFO(true);
			imu.setFIFO(FIFO_CONT, 0x1F);

			lcd.clear();
			lcd.write("Terminando ");
			lcd.setCursor(0,1);
			lcd.write("modo USB...");
			delay_ms(1500);
			lcd.clear();

		} // end modo captura USB

		delay_ms(25);
	}
} // end ejecutar_modo_manual()


void ejecutar_barrido_USB()
{
	imu.enableFIFO(true);
	imu.setFIFO(FIFO_CONT, 0x1F);

	PERIOD_ESTIMATE_IMU_AVERAGE = 40;
	int post_avrg_period_cnts = 0;
	int16_t pwm_val = 150;

	// Give some time to stabilize:
	pwm_set_duty_cycle(PWM_TIMER0, PWM_PIN_OCnA, pwm_val);
	delay_ms(1000);

	char str[100];

	while (pwm_val>25)
	{
		if (!gpio_pin_read(PIN_BTN_CANCEL))
		{
			return;
		}

		// Set PWM output:
		pwm_set_duty_cycle(PWM_TIMER0, PWM_PIN_OCnA, pwm_val);
		delay_ms(100);

		// Rotor freq estimation:
		const uint32_t t_now = millis();
		if (t_now-tim_last_freq_estim>PERIOD_ESTIM_FREQ_MS_TH)
		{
			cli();
			const uint32_t encoder_now = ENC_STATUS[0].COUNTER;
			sei();

			const int32_t Aenc = int32_t(encoder_now)-int32_t(last_freq_estim_encoder);
			const double K = ENCODER_TICKS_PER_REV*PERIOD_ESTIM_FREQ_MS_TH*0.0001;
			freq = double(Aenc)/K;

			tim_last_freq_estim = t_now;
			last_freq_estim_encoder = encoder_now;
		}

		// Convert to m/s2
		XYZd imu_pk;
		read_imu_and_avrg_peak_values(imu_pk);

		// Average peak values:
		imu_avr_pk_tmp.keep_max_with(imu_pk);
		bool new_bode_data = false;
		if (++imu_avr_cnt>PERIOD_ESTIMATE_IMU_AVERAGE)
		{
			imu_avr_pk = imu_avr_pk_tmp;
			imu_avr_pk_tmp= XYZd();
			imu_avr_cnt=0;

			// make sure we discard the first period of averaging, just in case we pass 
			// through a resonance, which trash the data:
			if (++post_avrg_period_cnts>=2)
			{
				post_avrg_period_cnts = 0;
				new_bode_data=true;
			}
		}
		// LCD output ==============
		
		// calc PWM as percentage:
		{
			const float pwm_pc = (pwm_val *100) / 255.0f;
			int pwm_unit = (int)pwm_pc;
			sprintf(str,"P=%3d%%",pwm_unit);
		}
		lcd.setCursor(0,0);
		lcd.write(str);

		{
			int freq_unit = (int)freq;
			int freq_cents = freq*uint32_t(10) - uint32_t(freq_unit)*10;
			sprintf(str,"F=%02d.%01dHz  ",freq_unit, freq_cents);
		}
		lcd.setCursor(7,0);
		lcd.write(str);

		// Accel:
		{
			int32_t a_unit = (int32_t)imu_avr_pk.z;
			int32_t a_cents = imu_avr_pk.z*int32_t(100) - int32_t(a_unit)*100;
			sprintf(str,"Apk=%3ld.%02ld m/s2  ",a_unit,a_cents);
		}
		lcd.setCursor(0,1);
		lcd.write(str);
		
		// Dump Bode info to USB:
		if (new_bode_data)
		{
			// Move forward:
			pwm_val-=2;

			uint32_t xpk = imu_avr_pk.x*1000;
			uint32_t ypk = imu_avr_pk.y*1000;
			uint32_t zpk = imu_avr_pk.z*1000;
			uint32_t freq_mils = freq*1000;

			sprintf(str,"%ld %ld %ld %ld\r\n", freq_mils,xpk,ypk,zpk);
			UART::WriteString(str);
		}
	}

	// Stop!
	pwm_set_duty_cycle(PWM_TIMER0, PWM_PIN_OCnA, 0);

} // end ejecutar_barrido_USB()

