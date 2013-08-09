#ifndef MORLOCK_DEFS_H
#define MORLOCK_DEFS_H
/*------------------------------------------------------------------------------*/
/* Copyright: (c) 2013 by Curt Hartung
 * This work is released under the Creating Commons 3.0 license
 * found at http://creativecommons.org/licenses/by-nc-sa/3.0/legalcode
 * and in the LICENCE.txt file included with this distribution
 */

#ifdef _WIN32
#define uchar unsigned char
#define uint unsigned short
#pragma pack(push, 1)
#endif

#define TIMER_COUNTS_PER_SECOND 6000
#define TIMER_COUNTS_PER_SECOND_X10 60000

#define MS_UNTIL_ENTRY_VALID 1000

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
enum USBCommands
{
	ceCommandIdle = 0,
	ceCommandSetEEPROMConstants,
	ceCommandGetEEPROMConstants
};

#define DEFAULT_SINGLE_SOLENOID 1
#define DEFAULT_FIRE_MODE  ceSemi
#define DEFAULT_BALLS_PER_SECOND_X_10 125
#define DEFAULT_BURST_COUNT 3
#define DEFAULT_MS_TO_ENHANCED_TRIGGER_DISABLE 400
#define DEFAULT_BOLT_HOLDOFF 5
#define DEFAULT_ACCESSORY_RUN_TIME 100
#define DEFAULT_DIMMER 255
#define DEFAULT_ANTI_BOLT_STICK_TIMEOUT 0
#define DEFAULT_ANTI_BOLT_STICK_ADDITION 5
#define DEFAULT_DEBOUNCE 150
#define DEFAULT_REBOUNCE 30
#define DEFAULT_DWELL1 10
#define DEFAULT_DWELL_1_TO_2_HOLDOFF 10
#define DEFAULT_DWELL2 50
#define DEFAULT_MAX_DWELL2 100
#define DEFAULT_EYE_HOLDOFF 1
#define DEFAULT_EYE_HIGH_BLOCKED 1
#define DEFAULT_LOCKED 0
#define DEFAULT_RAMP_ENABLE_COUNT 3
#define DEFAULT_RAMP_CLIMB_RATE 1
#define DEFAULT_RAMP_TOP_MODE ceSemi
#define DEFAULT_RAMP_TIMEOUT 750

//------------------------------------------------------------------------------
// loaded and saved as a flat structure
struct EEPROMConstants
{
	uchar singleSolenoid;

	uchar fireMode; // what fire mode we're in
	uint ballsPerSecondX10; // fixed-point x10 rate of fire
	uchar burstCount; // how many shots are in a burst
	uint enhancedTriggerTimeout;

	uchar boltHoldoff; // (milliseconds)
	
	uchar accessoryRunTime; // milliseconds to run FET2 in single-solenoid mode

	uchar dimmer; // 0-255 how dim to run the LED

	uint ABSTimeout;  // how long before engaging Anti Boltstick (milliseconds)
	uchar ABSAddition; // (milliseconds)

	uchar rebounce; // how long a trigger state must remain stable before it is believed (fast count)
	uchar debounce; // min time between samples after a state change has occured (fast counts)

	uchar dwell1; // (milliseconds)
	uchar dwell1ToDwell2Holdoff; // (milliseconds) time after solenoid1 fire to fire solenoid2, may be negative
	uchar dwell2; // (milliseconds)
	uchar maxDwell2; // (milliseconds)

	uchar eyeHoldoff; // (milliseconds)
	uchar eyeHighBlocked;

	uchar locked;

	uchar rampEnableCount; // shots before ramping starts
	uchar rampClimb; // how many additional shots are added while ramping
	uchar rampTopMode; // when ramping is fully engaged, what mode of fire should be used
	uint rampTimeout; // how long the trigger must be idle for ramping to expire

#ifdef _WIN32
	void installDefaults()
	{
		singleSolenoid = DEFAULT_SINGLE_SOLENOID;
		fireMode = DEFAULT_FIRE_MODE;
		ballsPerSecondX10 = DEFAULT_BALLS_PER_SECOND_X_10;
		burstCount = DEFAULT_BURST_COUNT;
		enhancedTriggerTimeout = DEFAULT_MS_TO_ENHANCED_TRIGGER_DISABLE;
		boltHoldoff = DEFAULT_BOLT_HOLDOFF;
		accessoryRunTime = DEFAULT_ACCESSORY_RUN_TIME;
		dimmer = DEFAULT_DIMMER;
		ABSTimeout = DEFAULT_ANTI_BOLT_STICK_TIMEOUT;
		ABSAddition = DEFAULT_ANTI_BOLT_STICK_ADDITION;
		rebounce = DEFAULT_REBOUNCE;
		debounce = DEFAULT_DEBOUNCE;
		dwell1 = DEFAULT_DWELL1;
		dwell1ToDwell2Holdoff = DEFAULT_DWELL_1_TO_2_HOLDOFF;
		dwell2 = DEFAULT_DWELL2;
		maxDwell2 = DEFAULT_MAX_DWELL2;
		eyeHoldoff = DEFAULT_EYE_HOLDOFF;
		eyeHighBlocked = DEFAULT_EYE_HIGH_BLOCKED;
		locked = DEFAULT_LOCKED;
		rampEnableCount = DEFAULT_RAMP_ENABLE_COUNT;
		rampClimb = DEFAULT_RAMP_CLIMB_RATE;
		rampTopMode = DEFAULT_RAMP_TOP_MODE;
		rampTimeout = DEFAULT_RAMP_TIMEOUT;
	}
#endif
};

#ifdef _WIN32
#pragma pack(pop)
#endif

#endif