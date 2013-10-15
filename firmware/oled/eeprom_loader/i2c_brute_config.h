#ifndef I2C_BRUTE_CONFIG_H
#define I2C_BRUTE_CONFIG_H
/* Copyright: (c) 2013 by Curt Hartung avr@northarc.com
 * This work is released under the Creating Commons 3.0 license
 * found at http://creativecommons.org/licenses/by-nc-sa/3.0/legalcode
 * and in the LICENCE.txt file included with this distribution
 */

// define the pins, brute-force will take care of the rest

#define I2C_BRUTE_SDA_PORT_LETTER	A
#define I2C_BRUTE_SDA_PIN_NUMBER	6

#define I2C_BRUTE_SCK_PORT_LETTER	A
#define I2C_BRUTE_SCK_PIN_NUMBER	4

#define I2C_BRUTE_DELAY_US			2 // number of microseconds to delay between clock transitions

#endif
