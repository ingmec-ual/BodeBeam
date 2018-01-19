# BodeBeam
HW y SW para práctica de vibraciones mecánicas


# Conexiones:

* Encoder motor ([Pololu](https://www.pololu.com/product/2822)):

		Color	Function
		Red	motor power (connects to one motor terminal)
		Black	motor power (connects to the other motor terminal)
		Green	encoder GND
		Blue	encoder Vcc (3.5 – 20 V)
		Yellow	encoder A output
		White	encoder B output

* LCD:
 - SDA: PC1
 - SCL: PC0
 - Power 5V and GND

* Botones:
 - UP:   PA0
 - DOWN: PA1
 - OK:   PA2
 - BACK: PA3

* PWM:
 - OC0A=PIN PB3  --> PWMH

* Encoder:
 - INT0=PD2

* Bornera:
 - 1: Motor +
 - 2: Motor -
 - 3: Encoder Vcc (Azul)
 - 4: Encoder GND (Verde)
 - 5: Encoder A (Amarillo)
 - 6: IMU GND
 - 7: IMU VCC
 - 8: IMU SCK ("SCL")
 - 9: IMU MOSI SDI ("SDA")
 - 10: IMU MISO SDO_A/G
 - 11: IMU CS_AG

* IMU: LSM9DS1
 - SCL/SPC => SDI SCK SPI
 - SDA/SDI: SPI MOSI
 - SDO_A/G: SPI MISO
 - CS_A/G: =0 enable SPI  => PA5
 - CS_M: (not used) => PA6
