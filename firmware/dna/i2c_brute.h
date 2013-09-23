#ifndef I2C_BRUTE_H
#define I2C_BRUTE_H
/*------------------------------------------------------------------------------*
 * Copyright: (c) 2013 by Curt Hartung avr@northarc.com
 * This work is released under the Creating Commons 3.0 license
 * found at http://creativecommons.org/licenses/by-nc-sa/3.0/legalcode
 * and in the LICENCE.txt file included with this distribution
 */

#include <avr/io.h>
#include <i2c_brute_config.h>
#include "i2c_interface.h"

#define I2C_BRUTE_CONCAT(x, y)  x##y
#define I2C_BRUTE_D_PORT(port)  I2C_BRUTE_CONCAT( PORT, port )
#define I2C_BRUTE_D_PIN(port)  I2C_BRUTE_CONCAT( PIN, port )
#define I2C_BRUTE_D_DDR(port)  I2C_BRUTE_CONCAT( DDR, port )

#define I2C_BRUTE_SDA_PORT  I2C_BRUTE_D_PORT( I2C_BRUTE_SDA_PORT_LETTER )
#define I2C_BRUTE_SDA_PIN  I2C_BRUTE_D_PIN( I2C_BRUTE_SDA_PORT_LETTER )
#define I2C_BRUTE_SDA_DDR  I2C_BRUTE_D_DDR( I2C_BRUTE_SDA_PORT_LETTER )

#define I2C_BRUTE_SCK_PORT  I2C_BRUTE_D_PORT( I2C_BRUTE_SCK_PORT_LETTER )
#define I2C_BRUTE_SCK_PIN  I2C_BRUTE_D_PIN( I2C_BRUTE_SCK_PORT_LETTER )
#define I2C_BRUTE_SCK_DDR  I2C_BRUTE_D_DDR( I2C_BRUTE_SCK_PORT_LETTER )

#define i2cBruteSDAHigh() (I2C_BRUTE_SDA_DDR &= ~(1<<I2C_BRUTE_SDA_PIN_NUMBER))
#define i2cBruteSDALow() (I2C_BRUTE_SDA_DDR |= (1<<I2C_BRUTE_SDA_PIN_NUMBER))
#define i2cBruteIsSDAHigh() (I2C_BRUTE_SDA_PIN & (1<<I2C_BRUTE_SDA_PIN_NUMBER))
#define i2cBruteIsSDALow() (!i2cBruteIsSDAHigh())

#define i2cBruteSCKHigh() (I2C_BRUTE_SCK_DDR &= ~(1<<I2C_BRUTE_SCK_PIN_NUMBER))
#define i2cBruteSCKLow() (I2C_BRUTE_SCK_DDR |= (1<<I2C_BRUTE_SCK_PIN_NUMBER))
#define i2cBruteIsSCKHigh() (I2C_BRUTE_SCK_PIN & (1<<I2C_BRUTE_SCK_PIN_NUMBER))
#define i2cBruteIsSCKLow() (!(I2C_BRUTE_SCK_PIN & (1<<I2C_BRUTE_SCK_PIN_NUMBER)))

#endif
