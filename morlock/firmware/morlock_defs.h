/* Copyright: (c) 2013 by Curt Hartung avr@northarc.com
 * This work is released under the Creating Commons 3.0 license
 * found at http://creativecommons.org/licenses/by-nc-sa/3.0/legalcode
 * and in the LICENCE.txt file included with this distribution
 */
#ifndef MORLOCK_DEFS_H
#define MORLOCK_DEFS_H

// must define this for whatever OS it's running on
#ifdef _WIN32
#include <winsock2.h>
#include <windows.h>
#include "../../sdk/firmware/dna/dna_types.h"
#pragma pack(push, 1)

#else
//#include <arpa/inet.h>
//#define uin8 unsigned char
//#define uint unsigned short
#endif

// this calculation is a single constant which will give us the number
// of counts to wait, given the BPS desired
#define ROF_TIMER_NUMERATOR_X10 468750 // (12mHz / 256(timer 1 prescaler)) * 10
#define MS_UNTIL_ENTRY_VALID 1000
#define RATE_OF_FIRE_COUNTDOWN_TOP 1000

#define MORLOCK_CODE_VERSION 1
#define MORLOCK_APP_VERSION "1.01"

//------------------------------------------------------------------------------
enum FireModes
{
	ceSemi =1,
	ceAutoresponse,
	ceBurst,
	ceFullAuto,
	ceRamp,
	ceCocker,
	ceSniper,
};

//------------------------------------------------------------------------------
enum ProgrammingSelectState
{
	ceStateSelectingRegister = 0,
	ceStateSelectingDeltaMethod,
	ceStateSelectingValue,
};

//------------------------------------------------------------------------------
enum ProgrammingRegister
{
	ceRegisterFireMode = 1,
	ceRegisterFireRate,
	ceRegisterEyeToggle,
	ceRegisterDwell1,
	ceRegisterDwell2,

	ceRegisterLast
};

//------------------------------------------------------------------------------
enum USBCommands
{
	ceCommandIdle = 0,
	ceCommandSetEEPROMConstants,
	ceCommandGetEEPROMConstants,
	ceCommandRNASend,

	ceCommandGetEEPROM,
};

#ifdef __AVR_ARCH__
#define CONSTS consts.
#else
#define CONSTS
#endif

#define INSTALL_MORLOCK_DEFAULTS \
	CONSTS singleSolenoid = 1;\
	CONSTS fireMode = ceSemi;\
	CONSTS ballsPerSecondX10 = 125;\
	CONSTS burstCount = 3;\
	CONSTS enhancedTriggerTimeout = 400;\
	CONSTS boltHoldoff = 5;\
	CONSTS accessoryRunTime = 100;\
	CONSTS dimmer = 8;\
	CONSTS ABSTimeout = 0;\
	CONSTS ABSAddition = 5;\
	CONSTS rebounce = 15;\
	CONSTS debounce = 40;\
	CONSTS dwell1 = 8;\
	CONSTS dwell2Holdoff = 8;\
	CONSTS dwell2 = 50;\
	CONSTS maxDwell2 = 150;\
	CONSTS eyeHoldoff = 1;\
	CONSTS eyeHighBlocked = 0;\
	CONSTS eyeEnabled = 0;\
	CONSTS eyeDetectLevel = 127;\
	CONSTS eyeDetectHoldoff = 10;\
	CONSTS eyeStrong = 0;\
	CONSTS locked = 0;\
	CONSTS rampEnableCount = 3;\
	CONSTS rampClimb = 1;\
	CONSTS rampTopMode = ceSemi;\
	CONSTS rampTimeout = 750;\
	CONSTS shortCyclePreventionInterval = 5;\

//------------------------------------------------------------------------------
// loaded and saved as a flat structure
struct EEPROMConstants
{
	uint8 singleSolenoid;

	uint8 fireMode; // what fire mode we're in
	uint16 ballsPerSecondX10; // fixed-point x10 rate of fire
	uint8 burstCount; // how many shots are in a burst
	uint16 enhancedTriggerTimeout;

	uint8 boltHoldoff; // (milliseconds)
	
	uint8 accessoryRunTime; // milliseconds to run FET2 in single-solenoid mode

	uint8 dimmer; // 0-255 how dim to run the LED

	uint16 ABSTimeout;  // how long before engaging Anti Boltstick (milliseconds)
	uint8 ABSAddition; // (milliseconds)

	uint8 rebounce; // how long a trigger state must remain stable before it is believed (fast count)
	uint8 debounce; // min time between samples after a state change has occured (fast counts)

	uint8 dwell1; // (milliseconds)
	uint8 dwell2Holdoff; // (milliseconds) time after solenoid1 fire to fire solenoid2
	uint8 dwell2; // (milliseconds)
	uint8 maxDwell2; // (milliseconds)

	uint8 eyeEnabled;
	uint8 eyeHoldoff; // (milliseconds)
	uint8 eyeHighBlocked;
	uint8 eyeDetectLevel;
	uint8 eyeDetectHoldoff;
	uint8 eyeStrong;

	uint8 locked;

	uint8 rampEnableCount; // shots before ramping starts
	uint8 rampClimb; // how many additional shots are added while ramping
	uint8 rampTopMode; // when ramping is fully engaged, what mode of fire should be used
	uint16 rampTimeout; // how long the trigger must be idle for ramping to expire

	uint8 shortCyclePreventionInterval; // the anmount of time the gun always waits at the end of a fire cycle

	// return data
	uint8 eyeLevel;
	uint8 version;

#ifndef __AVR_ARCH__
	void installDefaults()
	{
		INSTALL_MORLOCK_DEFAULTS;
	}

	//------------------------------------------------------------------------------
	void swap( unsigned short* us )
	{
		*us = (*us>>8) | (*us<<8);
	}
	
	//------------------------------------------------------------------------------
	// set the byte-order properly for values that are in host OS
	// format but need to go to the AVR 
	void transposeValues()
	{
		if ( htons(0x1234) != (uint16)0x1234 )
		{
			// little-endian machines are already in the right order
			return;
		}
		swap( &ballsPerSecondX10 );
		swap( &enhancedTriggerTimeout );
		swap( &ABSTimeout );
		swap( &rampTimeout );
	}

#endif
};

#ifdef _WIN32
#pragma pack(pop)
#endif

#endif