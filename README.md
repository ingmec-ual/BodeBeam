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
 - 5: Encoder A o B (Amarillo)
 - 6: IMU GND
 - 7: IMU VCC
 - 8: IMU SCL -> uC SCL
 - 9: IMU SDA -> uC SDA
 - 10: 
 - 11: IMU INT1 -> PA5
