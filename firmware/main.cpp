/*****************************************************************************************
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

int main(void)
{
	// ================== Setup hardware ==================
	UART::Configure(500000);
	millis_init();

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


void ejecutar_modo_manual()
{
	imu.enableFIFO(true);
	imu.setFIFO(FIFO_CONT, 0x1F);

	uint32_t tim_last_freq_estim = 0;
	uint32_t last_freq_estim_encoder = 0;
	double   freq = .0; // Estimated rotor freq
		
	// average accelerations:
	XYZd     imu_avr_pk, imu_avr_pk_tmp;
	uint8_t imu_avr_cnt = 0;
	const uint8_t IMU_AVR_MAX = 40;

	int16_t pwm_val = 0;

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
		// Convert to m/s2
		XYZd imu_pk;
		imu_pk.x = 9.81*imu.calcAccel(imu_ax_pk);
		imu_pk.y = 9.81*imu.calcAccel(imu_ay_pk);
		imu_pk.z = 9.81*(imu.calcAccel(imu_az_pk) - 1.0 /* remove gravity */);
		if (imu_pk.z<0) imu_pk.z=-imu_pk.z;

		// Average peak values:
		imu_avr_pk_tmp.keep_max_with(imu_pk);
		if (++imu_avr_cnt>IMU_AVR_MAX)
		{
			imu_avr_pk = imu_avr_pk_tmp;
			imu_avr_pk_tmp= XYZd();
			imu_avr_cnt=0;
		}
		// LCD output ==============
			
		// calc PWM as percentage:
		char str_pwm[16];
		{
			const float pwm_pc = (pwm_val *100) / 255.0f;
			int pwm_unit = (int)pwm_pc;
			//int pwm_cents = pwm_pc*uint16_t(10) - uint16_t(pwm_unit)*10;
			//sprintf(str_pwm,"P=%3d.%01d%%",pwm_unit,pwm_cents);
			sprintf(str_pwm,"P=%3d%%",pwm_unit);
		}
		lcd.setCursor(0,0);
		lcd.write(str_pwm);

		char str_freq[16];
		{
			int freq_unit = (int)freq;
			int freq_cents = freq*uint32_t(10) - uint32_t(freq_unit)*10;
			sprintf(str_freq,"F=%02d.%01dHz",freq_unit, freq_cents);
		}
		lcd.setCursor(7,0);
		lcd.write(str_freq);

		// Accel:
		char str_accel[16];
		{
			int32_t a_unit = (int32_t)imu_avr_pk.z;
			int32_t a_cents = imu_avr_pk.z*int32_t(100) - int32_t(a_unit)*100;
			sprintf(str_accel,"Apk=%3ld.%02ld m/s2  ",a_unit,a_cents);
		}
		lcd.setCursor(0,1);
		lcd.write(str_accel);
			
		// USB output:
		#if 0
		UART::WriteString(str);
		#endif

		delay_ms(25);
	}
} // end ejecutar_modo_manual()


void ejecutar_barrido_USB()
{

} // end ejecutar_barrido_USB()

