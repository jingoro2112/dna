#include "i2c.h"
/* Copyright: (c) 2013 by Curt Hartung
 * This work is released under the Creating Commons 3.0 license
 * found at http://creativecommons.org/licenses/by-nc-sa/3.0/legalcode
 * and in the LICENCE.txt file included with this distribution
 */

#if defined ( _AVR_IOMX8_H_ )
	#include "i2c_atmega.c"
#elif defined ( _AVR_IOTNX4_H_ )
	#include "i2c_attiny.c"
#endif

