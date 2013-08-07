#include <dna.h>
#include <usb.h>
#include <a2d.h>

#include <avr/interrupt.h>
#include <avr/eeprom.h>
#include <util/delay.h>

#include "morlock_defs.h"

#ifndef true
#define true 1
#endif
#ifndef false
#define false 0
#endif

// some macros to make program flow a bit easier, also facilitates
// stubbing
#define eyeEnable()  (PORTA |= 0b00000000)
#define eyeDisable() (PORTA &= 0b11111111)
#define readTrigger() (PORTA & 0b00000000)
#define setLedOn()  (PORTA &= 0b01111111)
#define setLedOff() (PORTA |= 0b10000000)
#define fet1On()
#define fet1Off()
#define fet2On()
#define fet2Off()

#define LIGHT_ON_FOR					  400
#define LIGHT_OFF_FOR					  500
#define LIGHT_ON_FOR_FAST				  150
#define LIGHT_OFF_FOR_FAST				  200

//------------------------------------------------------------------------------
// a little maintenance but worth it for the memory effeciency, the
// compiler also handles this quite well.
struct Bits
{
	uchar b0:1;
	uchar b1:1;
	uchar b2:1;
	uchar b3:1;
	uchar b4:1;
	uchar b5:1;
	uchar b6:1;
	uchar b7:1;
} volatile bits[10];

uint refireTime;
volatile uint refireBox;

// I can't think of a pre-compiler way to properly manage this
// hands-off, if I think of some sort of enum/typedef/macro-geddon I'll
// add it
#define eyeBlocked				(bits[0].b0) // is the eye blocked
#define eyeHighBlocked			(bits[0].b1) // is blocked high, or low?
#define triggerState			(bits[0].b2) // high is depressed
#define triggerStateChangeValid	(bits[0].b3) // did the state change last long enough to 'count'?
#define startFireCycle			(bits[0].b4) // signal to the main loop to begine a single shot cycle
#define inProgramMode			(bits[0].b5)
#define arToolate				(bits[0].b6) // did the autoresponse timer time out?
#define a2dWatchingEye			(bits[0].b7) // is the a2d mux-ed to the eye channel and compare level set appropriately?
#define samplingVoltage			(bits[1].b0) // is the a2d being used to sample voltage? if so do not toggle LED!
#define blinkOn					(bits[1].b1) // while blinking, is the light on?
#define ledOn					(bits[1].b2) // should the LED be on right now? this is filtered through dimmer and voltage check
#define useEye					(bits[1].b3) // is the eye being used
#define eyeFault				(bits[1].b4) // has the eye been detected as faulty
#define selectingRegister		(bits[1].b5) // program mode

volatile uint msToAutoresponseTriggerDisable;
volatile uint shotsInString;
volatile uint millisecondCountBox;
volatile uint millisecondCountBox2;
volatile uint millisecondCountBox3;

volatile uchar accessoryRunTime;
uint scheduleShotRate;
volatile uint scheduleShotBox;
volatile uint antiBoltstickTimeout;

uchar timesToBlinkLight;
volatile uint blinkBox;
volatile uchar dimmerBox;
volatile uchar debounceBox;
volatile uchar burstCount;

uchar usbCommand = ceCommandIdle;
uchar eepromLoadPointer;
volatile struct EEPROMConstants consts;

// program mode
volatile uint msUntilEntryValid;
volatile uchar currentEntry;

uchar rampModeReplaced;
volatile uint rampTimeoutBox;
uchar rampLevel;
uchar triggerTimer;
uchar currentFireMode;

//------------------------------------------------------------------------------
// source up the profile
void loadEEPROMConstants()
{
	unsigned int i;
	for( i=0; i<sizeof(consts); i++ )
	{
		((uchar*)&consts)[i] = eeprom_read_byte( (uchar*)i );
	}

	refireTime = TIMER_COUNTS_PER_SECOND_X10 / consts.ballsPerSecondX10;
	currentFireMode = consts.fireMode;
}

//------------------------------------------------------------------------------
void saveEEPROMConstants()
{
	unsigned int i;
	for( i=0; i<sizeof(consts); i++ )
	{
		eeprom_write_byte( (uchar*)i, *(((uchar*)&consts) + i) );
		eeprom_busy_wait();
	}
}

//------------------------------------------------------------------------------
unsigned char dnaUsbInputSetup( unsigned char *data, unsigned char len )
{
	if ( usbCommand != ceCommandIdle )
	{
		return 0; // must be part of a multi-command, pass through
	}

	usbCommand = data[0];
	switch( usbCommand )
	{
		case ceCommandSetEEPROMConstants:
		{
			eepromLoadPointer = 0;
			break;
		}

		case ceCommandGetEEPROMConstants:
		{
			dnaUsbQueueData( (unsigned char *)&consts, sizeof(consts) );
			usbCommand = ceCommandIdle;
			break;
		}

		default:
			usbCommand = ceCommandIdle; // yeek
			break;
	}

	return 1;
}

//------------------------------------------------------------------------------
void dnaUsbInputStream( unsigned char *data, unsigned char len )
{
	switch( usbCommand )
	{
		case ceCommandSetEEPROMConstants:
		{
			unsigned char i;
			for( i=0; i<len; i++ )
			{
				((unsigned char *)&consts)[eepromLoadPointer++] = data[i];
			}

			if ( eepromLoadPointer >= sizeof(consts) )
			{
				usbCommand = ceCommandIdle;
				saveEEPROMConstants();
			}
			break;
		}

		default:
		{
			usbCommand = ceCommandIdle;
			break;
		}
	}
}

//------------------------------------------------------------------------------
void cycleSingleSolenoid()
{
	refireBox = refireTime;
	if ( useEye )
	{
		if ( !eyeBlocked )
		{
			millisecondCountBox = 400;
			while( millisecondCountBox )
			{
				// break the count if the trigger is released on an
				// empty chamber, or the ball fell within 100ms of a
				// scheduled shot, or we are in semi-auto
				if ( !triggerState
					 && ((millisecondCountBox < 300) || currentFireMode == ceSemi) )
				{
					startFireCycle = false;
					return;
				}

				if ( eyeBlocked ) // that's what we were waiting for, a ball to drop
				{
					break;
				}
			}

			if ( millisecondCountBox < 375 ) // took more than 25 milliseconds, dont' want to short-cycle the gun
			{
				refireBox = refireTime;
			}
		}

		millisecondCountBox = consts.eyeHoldoff; // wait for ball to 'seat'
		while( millisecondCountBox );
	}

	ledOn = true;

	fet1On();

	if ( consts.accessoryRunTime )
	{
		accessoryRunTime = consts.accessoryRunTime;
		fet2On();
	}

	millisecondCountBox = consts.dwell1;

	if ( !antiBoltstickTimeout && consts.ABSTime )
	{
		antiBoltstickTimeout = consts.ABSTime;
		millisecondCountBox += consts.ABSAddition;
	}

	while( millisecondCountBox );

	fet1Off();

	if ( useEye )
	{
		// okay the bolt is on its way forward or already forward,
		// give it some time to unblock, if it takes longer than
		// that then something is wrong.
		millisecondCountBox = 100;
		while ( eyeBlocked && millisecondCountBox );

		// wait the holdoff to make sure the bolt is on its way back,
		// and we're not seeing the space between the ball and
		// the bolt.
		millisecondCountBox = consts.fireHoldoff;
		while( millisecondCountBox );

		// if there is an error, then wait for the ROF timer,
		// otherwise ignore it and only wait the eye holdoff
	}

	while( refireBox );
}

//------------------------------------------------------------------------------
void cycleDoubleSolenoid()
{
	millisecondCountBox = consts.dwell1;
	millisecondCountBox2 = consts.dwell1ToDwell2Holdoff;
	millisecondCountBox3 = 0;

	fet1On();

	while( millisecondCountBox )
	{
		if ( !millisecondCountBox2 && (currentFireMode != ceSniper) )
		{
			fet2On();
			millisecondCountBox3 = consts.dwell2;
		}
	}

	fet1Off();

	while( !arToolate // don't engage sniper mode if you are firing quickly
		   && currentFireMode == ceSniper 
		   && triggerState );
	
	while( millisecondCountBox2 ); // wait for holdoff

	if ( useEye )
	{
		millisecondCountBox = consts.maxDwell2;
		
		while( millisecondCountBox && eyeBlocked ); // wait for eye to unblock indicating the bolt has opened

		if ( !millisecondCountBox )
		{
			// took too long, eye must be busted
			eyeFault = true;
		}
		else
		{
			eyeFault = false;
			
			while( millisecondCountBox && !eyeFault ); // now wait for paint to drop

			if ( !millisecondCountBox )
			{
				// took too long, eye must be damaged
				eyeFault = true;
			}
			else
			{
				millisecondCountBox = consts.eyeHoldoff; // wait for ball to 'seat'
				while( millisecondCountBox );

				eyeFault = false;
			}
		}
	}
	else 
	{
		if ( !millisecondCountBox3 )
		{
			fet2On();
			millisecondCountBox3 = consts.dwell2;
		}

		while( millisecondCountBox3 ); // wait for pulse, then check eye
	}

	if ( currentFireMode == ceCocker ) // cool mode if you hate your battery
	{
		while( triggerState && millisecondCountBox );
	}

	fet2Off();

	refireBox = refireTime - (consts.dwell1ToDwell2Holdoff + consts.dwell2); // load box as if everything happened instantly
}

//------------------------------------------------------------------------------
int __attribute__((noreturn)) main(void)
{
	// set up I/O
	DDRA = 0b10000000;
	PORTA = 0b10000000; // all off

//	DDRB = 0b00000000;
//	PORTB = 0b00000000;

	dnaUsbInit();
	loadEEPROMConstants();

	// set up timer 0 to tick exactly 6000 per second:
	// 12000000 / (8 * 250) = 6000
	// accomplished by using timer0 in waveform generation mode 2
	TCCR0B = 1<<CS01; // set 8-bit timer prescaler to div/8
	OCR0A = 250;
	TCCR0A = 1<<WGM01; // mode 2, reset counter when it reaches OCROA
	TIMSK0 = 1<<OCIE0A; // fire off an interrupt every time it matches 250, thus dividing by exactly 2000 (overflow would work too)
	
	a2dSetChannel( 6 );
	a2dSetPrescaler( A2D_PRESCALE_16 ); 
	a2dEnableInterrupt(); // latch in the value as an interrupt rather than polling

	sei();

	shotsInString = 0;
	currentEntry = 0;
	rampTimeoutBox = 1; // let this timeout immediately so the defaults will be instaled

	_delay_ms(10); // let the state settle

	if( triggerState && !consts.locked )
	{
		currentFireMode = 1;
		saveEEPROMConstants(); // prorgam mode defaults the gun to semi
		
		timesToBlinkLight = 2;
		inProgramMode = true;
	}

	for(;;)
	{
		// spin until a fire condition is triggered from the ISR
		while( !startFireCycle )
		{
			if ( !inProgramMode	)
			{
				ledOn = eyeBlocked;
			}
		}

		if ( currentFireMode != ceFullAuto )
		{
			startFireCycle = false;
		}

		// bursting? if so count it down
		if ( burstCount && --burstCount )
		{
			startFireCycle = true;
		}

		// setup complete, cycle the marker
		if ( consts.singleSolenoid )
		{
			cycleSingleSolenoid();
		}
		else
		{
			cycleDoubleSolenoid();
		}

		while( refireBox ); // wait until the counter clears us

		// if ramping has hit this top rate its basically full-auto as
		// long as the trigger keeps being cycled at some [slow] rate
		if ( currentFireMode == ceRamp
			 && scheduleShotRate < refireTime )
		{
			startFireCycle = true;
		}
	}
}

//------------------------------------------------------------------------------
// sample the eye asychonously
ISR( ADC_vect )
{
	if ( !a2dWatchingEye )
	{
		return;
	}

	eyeDisable(); // done with emitter

	// latch it in
	if ( a2dReadResultNoPoll() > consts.eyeTransitionLevel )
	{
		eyeBlocked = eyeHighBlocked;
	}
	else
	{
		eyeBlocked = !eyeHighBlocked;
	}	
}

//------------------------------------------------------------------------------
// Entered 6000 times per second
// if the marker is in programming mode, or USB is being used, some of
// the extra logic might push this ISR into the next tick, that's ok,
// it doesn't matter if a tick (or three!) is lost now and again, since
// when the gun is cycling none of that logic will be engaged.
ISR( TIM0_COMPA_vect )
{
	usbPoll();

	// timers at the top have a very high resolution
	if ( refireBox )
	{
		refireBox--;
	}

	if ( scheduleShotBox )
	{
		if ( !--scheduleShotBox && rampTimeoutBox && (currentFireMode == ceRamp) )
		{
			startFireCycle = true;
			scheduleShotBox = scheduleShotRate;
		}
	}

	if ( debounceBox ) // programmable debounce, sample ever X milliseconds
	{
		debounceBox--;
	}
	else if ( triggerState != readTrigger() )
	{
		// debounce check part 2, make sure the state change lasts long enough
		if ( !triggerStateChangeValid )
		{
			debounceBox = consts.antiMechanicalDebounce;
			triggerStateChangeValid = true; // next time around it counts
		}
		else
		{
			triggerState = !readTrigger();
			debounceBox = consts.debounce;

			if ( triggerState ) // been pressed
			{
				if ( inProgramMode )
				{
					msUntilEntryValid = MS_UNTIL_ENTRY_VALID;
					currentEntry++;
				}
				else
				{
					startFireCycle = true;
					shotsInString++;

					if ( currentFireMode == ceRamp
						 && shotsInString >= consts.rampEnableCount )
					{
						scheduleShotRate = ((consts.msToAutoresponseTriggerDisable - msToAutoresponseTriggerDisable) /
										   rampLevel) * 6;
						scheduleShotBox = scheduleShotRate;
						rampLevel += consts.rampRate;
						
						if ( scheduleShotRate <= refireTime )
						{
							currentFireMode = consts.rampTopMode; // we're there, go crazy
						}
					}
					
					rampTimeoutBox = consts.rampTimeout; // trigger has been depressed, reset the "do what you're doing" timeout
					
					// after this many milliseconds autoresponse will not fire
					msToAutoresponseTriggerDisable = consts.msToAutoresponseTriggerDisable;
					arToolate = false;

					if ( currentFireMode == ceBurst )
					{
						burstCount = consts.burstCount;
					}
				}
			}
			else if ( !inProgramMode ) // been released 
			{
				if ( (currentFireMode == ceAutoresponse) && !arToolate )
				{
					startFireCycle = true;
				}
				else
				{
					startFireCycle = false;
				}
			}
		}
	}
	else
	{
		triggerStateChangeValid = false;
	}

	// duty cycle for LED, effecting a dimmer
	if ( !samplingVoltage )
	{
		if ( ledOn )
		{
			if ( dimmerBox++ > 8 )
			{
				dimmerBox = 0;
			}
			
			if ( consts.dimmer > dimmerBox )
			{
				setLedOn();
			}
			else
			{
				setLedOff();
			}
		}
		else
		{
			setLedOff();
		}
	}

	static uchar millisecondPrescaler = 1;
	if ( --millisecondPrescaler )
	{
		return;
	}
	millisecondPrescaler = 6;

	// ------------------------------------------------------------------------------------------
	// exactly 1 ms per tick
	// ------------------------------------------------------------------------------------------
		
	if ( a2dWatchingEye )
	{
		eyeEnable(); // turn on emitter, wait until the end of this routine to kick off the sampling
	}
	
	if ( msToAutoresponseTriggerDisable )
	{
		msToAutoresponseTriggerDisable--;
		arToolate = true;
	}

	if ( antiBoltstickTimeout )
	{
		antiBoltstickTimeout--;
	}

	if ( millisecondCountBox )
	{
		millisecondCountBox--;
	}
	if ( millisecondCountBox2 )
	{
		millisecondCountBox2--;
	}
	if ( millisecondCountBox3 )
	{
		millisecondCountBox3--;
	}

	if ( rampTimeoutBox )
	{
		if ( !--rampTimeoutBox )
		{
			shotsInString = 0;
			rampLevel = 2;
			currentFireMode = consts.fireMode;
			scheduleShotRate = 0xFFFF;
		}
	}

	// fet2 being used to run a hopper? turn it off now
	if ( accessoryRunTime )
	{
		if ( !--accessoryRunTime )
		{
			fet2Off();
		}
	}

	// handle program mode activity
	if ( !triggerState && inProgramMode )	// trigger up in program mode?
	{
		if ( msUntilEntryValid )	// waiting for idle trigger? (signifying an entry has been made)
		{
			if ( !--msUntilEntryValid ) // this the one?
			{
				if ( selectingRegister )
				{
					selectingRegister = false;
//					ubAddress = ubCurrentEntry;

//					if ( currentEntry < ceLastEEPROMLocation )
					{
//						ReadEEPROM(); // read current value
//						ubAddress--; // undo auto-increment
//						timesToBlinkLight = ubData; // flash it
					}
//					else // error
					{
						timesToBlinkLight = 4;
						selectingRegister = true;
					}
				}
				else // not selecting a register, use the value
				{
					selectingRegister = true;

					timesToBlinkLight = 2;

/*
					if ( ubAddress == ceFireMode )
					{
						bInProgramMode = false;
					}

					ubData = ubCurrentEntry;
					WriteEEPROM(); // commit the value

					GetConstantsByEEPROM(); // read them all back

					ubData = 0;
*/
				}

				currentEntry = 0;
			}
		}
	}


	
	// if the LED is being told to blink, handle that here
	if ( timesToBlinkLight )
	{
		if ( blinkOn )
		{
			if ( !--blinkBox )
			{
				blinkOn = false;
				ledOn = true;
				blinkBox = LIGHT_OFF_FOR;
			}
		}
		else if ( !--blinkBox )
		{
			blinkOn = true;
			ledOn = false;
			blinkBox = LIGHT_ON_FOR;
			
			if ( !--timesToBlinkLight )
			{
				ledOn = inProgramMode;
			}
		}
	}

	// should have been plenty of CPU cycles to let the emitter rise, start conversion!
	if ( a2dWatchingEye )
	{
		a2dStartConversion(); 
	}
}