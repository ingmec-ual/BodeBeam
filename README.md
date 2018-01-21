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

* uSD card:
 - MISO (PB6)
 - MOSI (PB5)
 - SCK (PB7)
 - CS: PA6

* Bornera:
 - 1: Motor +
 - 2: Motor -
 - 3: Encoder Vcc (Azul)
 - 4: Encoder GND (Verde)
 - 5: Encoder A (Amarillo)
 - 6: IMU GND
 - 7: IMU VCC
 - 8: IMU SCK ("SCL") -> PB7
 - 9: IMU MOSI SDI ("SDA") -> PB5
 - 10: IMU MISO SDO_A/G -> PB6
 - 11: IMU CS_AG -> PA5

* IMU: LSM9DS1
 - SCL/SPC => SDI SCK SPI (PB7)
 - SDA/SDI: SPI MOSI (PB5)
 - SDO_A/G: SPI MISO (PB6)
 - CS_A/G: =0 enable SPI  => PA5
 - CS_M: (not used)
