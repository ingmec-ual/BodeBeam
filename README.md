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
 - UP: PA0
 - DOWN: PA1

* PWM:
 - OC0A=PIN PB3  --> PWMH
