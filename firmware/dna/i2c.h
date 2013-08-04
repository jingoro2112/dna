#ifndef I2C_H
#define I2C_H
/*------------------------------------------------------------------------------*/
/* Copyright: (c) 2013 by Curt Hartung
 * This work is released under the Creating Commons 3.0 license
 * found at http://creativecommons.org/licenses/by-nc-sa/3.0/legalcode
 * and in the LICENCE.txt file included with this distribution
 */

#include <avr/io.h>

#if defined ( _AVR_IOMX8_H_ )
#include "i2c_atmega.h"
#elif defined ( _AVR_IOTNX4_H_ )
#include "i2c_attiny.h"
#endif

#endif
