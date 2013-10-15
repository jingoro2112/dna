#ifndef MORLOCK_DEFS_H
#define MORLOCK_DEFS_H
/*------------------------------------------------------------------------------
 * Copyright: (c) 2013 by Curt Hartung avr@northarc.com
 * This work is released under the Creating Commons 3.0 license
 * found at http://creativecommons.org/licenses/by-nc-sa/3.0/legalcode
 * and in the LICENCE.txt file included with this distribution
 */

// must define this for whatever OS it's running on
#ifdef _WIN32
#include <winsock2.h>
#include <windows.h>
#include "../dna/dna_types.h"
#pragma pack(push, 1)

#elif !defined(AVR)
#include <arpa/inet.h>
#endif

#ifndef AVR
#include "../../util/str.hpp"
#include <stddef.h>
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
	ceCommandGetUtilityString,
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

#ifndef AVR

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

	//------------------------------------------------------------------------------
	static int offsetOf( const char* memberName )
	{
		Cstr compare( memberName );
		
		if ( compare == "singleSolenoid" ) return offsetof( EEPROMConstants, singleSolenoid );
		else if ( compare == "fireMode" ) return offsetof( EEPROMConstants, fireMode );
		else if ( compare == "ballsPerSecondX10" ) return offsetof( EEPROMConstants, ballsPerSecondX10 );
		else if ( compare == "burstCount" ) return offsetof( EEPROMConstants, burstCount );
		else if ( compare == "enhancedTriggerTimeout" ) return offsetof( EEPROMConstants, enhancedTriggerTimeout );
		else if ( compare == "boltHoldoff" ) return offsetof( EEPROMConstants, boltHoldoff );
		else if ( compare == "accessoryRunTime" ) return offsetof( EEPROMConstants, accessoryRunTime );
		else if ( compare == "dimmer" ) return offsetof( EEPROMConstants, dimmer );
		else if ( compare == "ABSTimeout" ) return offsetof( EEPROMConstants, ABSTimeout );
		else if ( compare == "ABSAddition" ) return offsetof( EEPROMConstants, ABSAddition );
		else if ( compare == "rebounce" ) return offsetof( EEPROMConstants, rebounce );
		else if ( compare == "debounce" ) return offsetof( EEPROMConstants, debounce );
		else if ( compare == "dwell1" ) return offsetof( EEPROMConstants, dwell1 );
		else if ( compare == "dwell2Holdoff" ) return offsetof( EEPROMConstants, dwell2Holdoff );
		else if ( compare == "dwell2" ) return offsetof( EEPROMConstants, dwell2 );
		else if ( compare == "maxDwell2" ) return offsetof( EEPROMConstants, maxDwell2 );
		else if ( compare == "eyeEnabled" ) return offsetof( EEPROMConstants, eyeEnabled );
		else if ( compare == "eyeHoldoff" ) return offsetof( EEPROMConstants, eyeHoldoff );
		else if ( compare == "eyeHighBlocked" ) return offsetof( EEPROMConstants, eyeHighBlocked );
		else if ( compare == "eyeDetectLevel" ) return offsetof( EEPROMConstants, eyeDetectLevel );
		else if ( compare == "eyeDetectHoldoff" ) return offsetof( EEPROMConstants, eyeDetectHoldoff );
		else if ( compare == "eyeStrong" ) return offsetof( EEPROMConstants, eyeStrong );
		else if ( compare == "locked" ) return offsetof( EEPROMConstants, locked );
		else if ( compare == "rampEnableCount" ) return offsetof( EEPROMConstants, rampEnableCount );
		else if ( compare == "rampClimb" ) return offsetof( EEPROMConstants, rampClimb );
		else if ( compare == "rampTopMode" ) return offsetof( EEPROMConstants, rampTopMode );
		else if ( compare == "rampTimeout" ) return offsetof( EEPROMConstants, rampTimeout );
		else if ( compare == "shortCyclePreventionInterval" ) return offsetof( EEPROMConstants, shortCyclePreventionInterval );
		else if ( compare == "eyeLevel" ) return offsetof( EEPROMConstants, eyeLevel );
		else if ( compare == "version" ) return offsetof( EEPROMConstants, version );
		else return -1;
	}

#endif
};

#ifdef _WIN32
#pragma pack(pop)
#endif

#endif