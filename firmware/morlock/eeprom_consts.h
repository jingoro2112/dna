#ifndef EEPROM_CONSTS_H
#define EEPROM_CONSTS_H
/*------------------------------------------------------------------------------*/
/* Copyright: (c) 2013 by Curt Hartung
 * This work is released under the Creating Commons 3.0 license
 * found at http://creativecommons.org/licenses/by-nc-sa/3.0/legalcode
 * and in the LICENCE.txt file included with this distribution
 */

#ifdef _WIN32
#define uchar unsigned char
#define uint unsigned short
#endif

//------------------------------------------------------------------------------
// loaded and saved as a flat structure
struct EEPROMConstants
{
	uchar fireMode; // what fire mode we're in
	uint refireCounter; // minimum time between shots
	uchar eyeOffRefireCounter;
	uchar antiMechanicalDebounce; //  how long a trigger state must remain stable before it is believed
	uchar debounce;
	uchar dwell1;
	uchar dwell2;
	uchar AFACount;
	uchar AFARate;
	uint eyeTransitionLevel; // the point at which the eye is considered to have gone from high to low
	uchar fireHoldoff;
	uchar eyeHoldoff;
	uchar ABSTime;
	uchar ABSAddition;
	uint EyeOffRefireCounter;
	uchar TriggerEyeDisable;
	uchar dimmer;
	uchar accessoryRunTime;
	uchar singleSolenoid;

	uchar locked;

};

#endif