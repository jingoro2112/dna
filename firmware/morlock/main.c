#include <dna.h>
#include <usb.h>
#include <a2d.h>
#include <i2c.h>
#include <24c512.h>

#include <avr/interrupt.h>
#include <avr/eeprom.h>

#include <util/delay.h>

#ifndef true
#define true 1
#endif
#ifndef false
#define false 0
#endif

#define TRIGGER	(PORTA & 0b00000100)
#define DIP		(PORTA & 0b00000100)
#define LED		(PORTA & 0b00000100)
#define EYE		(PINA & 0b00000100)
#define FET_1	(PORTA & 0b00000100)
#define FET_2	(PORTA & 0b00000100)

// some macros to make program flow a bit easier, also facilitates
// stubbing
#define eyeEnable()  (PORTA |= 0b00000000)
#define eyeDisable() (PORTA &= 0b11111111)
#define readTrigger() (PORTA & 0b00000000)
#define setLedOn()  (PORTA &= 0b11111111)
#define setLedOff() (PORTA |= 0b00000000)
#define fet1On()
#define fet1Off()
#define fet2On()
#define fet2Off()

#define LIGHT_ON_FOR					  40
#define LIGHT_OFF_FOR					  50
#define LIGHT_ON_FOR_FAST				  15
#define LIGHT_OFF_FOR_FAST				  20

//------------------------------------------------------------------------------
enum // FireModes
{
	ceENSemi =0,   // 1 semi auto, standard
	ceENAutoresponse, // 2 autoresponse
	ceENFA,        // 3 FA
	ceENSmooth,    // 4 smooth ramping
	ceENSteady,    // 5 assist ramping
	ceENFast,      // 6 fast ramping
	ceENSuperTriple, // 7 WDP triple
	ceENPSP,       // 8 PSP mode 3 shots then ramp
	ceENPSP2,      // 9 PSP2
	ceENPSP3,      // 10 triple
	ceENNXL,       // 11 NXL code 3 shots then FA
	ceFAFast,      // 12 FA off break then fast ramping
	ceENUNSAFE,    // 13 Unsafe
	ceBONXL        // 14 breakout then NXL
};

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

volatile uint refireCounter;
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
#define autoShot				(bits[0].b6)
#define a2dWatchingEye			(bits[0].b7) // is the a2d mux-ed to the eye channel and compare level set appropriately?

#define samplingVoltage			(bits[1].b0) // is the a2d being used to sample voltage? if so do not toggle LED!
#define blinkOn					(bits[1].b1) // while blinking, is the light on?
#define ledOn					(bits[1].b2) // should the LED be on right now? this is filtered through dimmer and voltage check
#define useEye					(bits[1].b3) // is the eye being used
#define eyeFault				(bits[1].b4) // has the eye been detected as faulty

volatile uint lastTriggerStateTransition; // how long in the past a trigger state changed
volatile uint countsToReactiveTriggerDisable;
volatile uchar AFARateBox;
uchar shotStringCount;
volatile uint countBox;
uchar accessoryRunTime;

uchar timesToBlinkLight;
uchar blinkBox;
uchar dimmerBox;
uchar debounceBox;
uchar burstCount;


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
	
} volatile consts;

#define FAST_TIMER_COUNTS_PER_SECOND 5859
#define FAST_TIMER_ISR_PRESCALE 23

// these are approximations
#define msToFastTimerCounts( ms ) ((ms) * 6) 
#define msToSlowTimerCounts( ms ) (((ms) * 255) / 1000)

//------------------------------------------------------------------------------
// source up the profile
void loadEEPROMConstants()
{
	unsigned int i;
	for( i=0; i<sizeof(consts); i++ )
	{
		*(((uchar*)&consts) + i) = eeprom_read_byte( (uchar*)i );
	}
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
void cycleSingleSolenoid()
{
	if ( useEye )
	{
		if ( !eyeBlocked )
		{
			countBox = msToFastTimerCounts( 400 );
			while( countBox )
			{
				// break the count if the trigger is released on an empty
				// chamber, but at the same time allow enhanced trigger
				// modes to function within a ~400ms window (AR and
				// Turbo)
				if ( !triggerState
					 && ((countBox < msToFastTimerCounts(350)) || consts.fireMode == ceENSemi) )
				{
					startFireCycle = false;
					return;
				}

				if ( eyeBlocked ) // that's what we were waiting for, a ball to drop
				{
					break;
				}
			}
		}
		// else the eye was blocked indicating nothing special needed
		// to happen
			
		countBox = consts.eyeHoldoff; // wait for ball to 'seat'
		while( countBox );
	}

	ledOn = true;

	fet1On();

	if ( consts.accessoryRunTime )
	{
		accessoryRunTime = consts.accessoryRunTime;
		fet2On();
	}

//	uiCountBox = ubDwell1;

#ifdef MORLOCK
	if ( !uiABSTimeBox && ubABSTime != 1 )
		uiCountBox += ubABSAddition;

	if ( ubABSTime > 1 )
	{
		uiABSTimeBox = 1250;
	}
#endif

//	while( uiCountBox );

	fet1Off();

/*
	if ( bUseEye )
	{
		// okay the bolt is on its way forward or already forward,
		// give it some time to unblock, if it takes longer than
		// that then something is wrong.
		uiCountBox = 100;
		while ( bEyeBlocked && uiCountBox );

		// wait the holdoff to make sure the bolt is on its way back,
		// and we're not seeing the space between the ball and
		// the bolt.
		uiCountBox = ubFireHoldoff; 
		while( uiCountBox );

		// if there is an error, then wait for the ROF timer,
		// otherwise ignore it and only wait the eye holdoff
	}

	while( ubRefireBox );
*/
	
}

//------------------------------------------------------------------------------
void cycleDoubleSolenoid()
{
/*
	uiCountBox = ubDwell1; 
	output_high( FET_1 );
	while( uiCountBox );
	output_low( FET_1 );

	if ( bSniperMode && !bTriggerReleaseTooLate ) // drop out of sniper if two shots are fired quickly
	{
		while ( !bTriggerState ); // trigger released?
	}

	uiRefireBox = uiRefireCounter - ubDwell1; // when can we fire again

	uiCountBox = ubFireHoldoff; 
	while( uiCountBox );

	if ( bUseEye )
	{
		bEyeError = false; // default to working on closed-bolt

		if ( !bEyeBlocked ) // bolt missing or eye faulty
			bEyeError = true;
	}

	output_high( FET_2 );	// open bolt

	if ( bUseEye && !bEyeError )
	{
		uiCountBox = BOLT_HELD_FOR;
		while( uiCountBox && bEyeBlocked ); // wait for bolt to open

		if ( !uiCountBox )
		{
			bEyeError = true;
		}
		else // now wait for paint to drop
		{
			while( uiCountBox && !bEyeBlocked ); // wait for bolt to open

			if ( !uiCountBox )
			{
				bEyeError = true;
			}
		}
	}
	else
	{
		uiCountBox = ubDwell2;
		while( uiCountBox );	// wait for pulse, then check eye
	}

	if ( bCockerMode ) // cool mode if you hate your battery
	{
		uiRefireHold = uiRefireBox;  // stop time

		do
		{
			uiCountBox = BOLT_HELD_FOR;
			while ( !bTriggerState && uiCountBox ); // wait for trigger to release

		} while ( !bCockerDontWaitForever && !bTriggerState );

		uiRefireBox = uiRefireHold;  // start time
	}


	output_low( FET_2 ); // close bolt
*/
}

unsigned char d[8];

//------------------------------------------------------------------------------
unsigned char dnaUsbInputSetup( unsigned char *data, unsigned char len )
{
	switch( data[0] )
	{
		case 1:
		{
			d[0] = 0xA1;
			d[1] = 0xB2;
			d[2] = 0xC1;
			d[3] = 0xD1;
			write24c512( 0xA0, 100, d, 4 );
			write24c512( 0xA0, 101, d+1, 1 );
			write24c512( 0xA0, 102, d+2, 1 );
			write24c512( 0xA0, 103, d+3, 1 );
			break;
		}

		case 2:
		{
			d[0] = 1;
			d[1] = 2;
			d[2] = 3;
			d[3] = 4;
//			read24c512( 0xA0, 100, d, 4 );
//			read24c512( 0xA0, 101, d+1, 1 );
//			read24c512( 0xA0, 102, d+2, 1 );
//			read24c512( 0xA0, 103, d+3, 1 );
			dnaUsbQueueData( d, 4 );

			break;
		}
	}


	return 0;
}

//------------------------------------------------------------------------------
void dnaUsbInputStream( unsigned char *data, unsigned char len )
{
}


//------------------------------------------------------------------------------
int __attribute__((noreturn)) main(void)
{
	dnaUsbInit();
	i2cInit(10);

//	DDRA = 0b10000000;
//	PORTA = 0b00000001;
	
//	TCCR0B = 1<<CS01; // set 8-bit timer prescaler to /8 for 5859.375 intterupts per second @12mHz
//	TIMSK0 = 1<<TOIE0; // fire off an interrupt every time it overflows, this is our tick (~170 microseconds per)

	sei();

	for(;;)
	{
		usbPoll();
	}
	/*
		while ( i2cStartWrite(0xA0) );
		i2cWrite( 0 );
		i2cWrite( 100 );

		i2cStartRead( 0xA0 );

		i2cReadByte();
		i2cReadByte();
		i2cReadByte();
		
//		if ( !i2cWrite( 0x55 ) )
//		{
//			PORTA &= 0b01111111;
//		}
//		else
//		{
//			PORTA |= 0b10000000;
//		}


//		i2cWrite( 0x23 );

		i2cStop();

		

//		read24c512( 0xA0, 100, d, 4 );

		_delay_ms(5);

				

//		usbPoll();
		
	}

*/











	
	loadEEPROMConstants();
	
	a2dSetChannel( 6 );
	a2dSetPrescaler( A2D_PRESCALE_16 ); 
	a2dEnableInterrupt(); // latch in the value as an interrupt rather than polling


	sei();

	_delay_ms(10); // let the state set up

	if( triggerState && !consts.locked )
	{
		consts.fireMode = 1;
		timesToBlinkLight = 2;
		inProgramMode = true;
	}

	for(;;)
	{
		while( !startFireCycle )
		{
			if ( !inProgramMode	)
			{
				ledOn = eyeBlocked;
			}
		}

//		countsToReactiveTriggerDisable = msToSlowTimerCounts( 500 );

		autoShot = false;

		if ( consts.fireMode != ceFAFast
			 && consts.fireMode != ceENFA
			 && consts.fireMode != ceBONXL )
		{
			startFireCycle = false;
		}

		// some enhanced fire modes
		if ( shotStringCount > consts.AFACount )
		{
			if ( AFARateBox )
			{
				if ( consts.fireMode == ceENSmooth )
				{
					debounceBox = 1;
				}

				if ( consts.fireMode == ceENFast || consts.fireMode == ceENPSP )
				{
					autoShot = true;
				}
			}

			if ( consts.fireMode == ceENNXL )
			{
				if ( !readTrigger() )
				{
					startFireCycle = true;
				}
			}

			if ( lastTriggerStateTransition
				 && (consts.fireMode == ceENPSP2 || consts.fireMode == ceENUNSAFE) )
			{
				startFireCycle = true;
			}
		}

		// bursting? if so count it down
		if ( burstCount && --burstCount )
		{
			startFireCycle = true;
		}

		refireBox = consts.eyeOffRefireCounter;
		AFARateBox = consts.AFARate;

		// setup should be complete now, we know we are firing, so now
		// its time to cycle the marker
		if ( consts.singleSolenoid )
		{
			cycleSingleSolenoid();
		}
		else
		{
			cycleDoubleSolenoid();
		}

		while( refireBox ); // wait until the counter clears us
	}
}

//------------------------------------------------------------------------------
// sample the eye asychonously
ISR( ADC_vect, ISR_NOBLOCK )
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
// this is entered 5859.375 times per second, to make sure any
// rate-of-fire calculations are performed properly I am truncating
// that for compuatational purposes.

// If the gun is cycling, then the programming stuff will never be
// entered, in programming mode some of the extra logic might push this
// ISR into the next tick, thats ok, it doesn't matter if a tick (or
// three!) is lost. 
ISR( TIM0_OVF_vect, ISR_NOBLOCK )
{
//	usbPoll();

	// timers at the top have a very high resolution
	if ( refireBox )
	{
		refireBox--;
	}

	if ( countBox )
	{
		countBox--;
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
			triggerState = !readTrigger();
			
			debounceBox = consts.antiMechanicalDebounce;
			triggerStateChangeValid = true; // next time around it counts

//			lastTriggerStateTransition = msToSlowTimerCounts( 750 );

			

		}
		else
		{
			
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

	// use a prescaler to get a more 'human' amount of time, which can
	// be measured with an 8-bit byte: 5859/23 = ~255 counts per second
	static uchar s_tim0Prescaler;
	if ( --s_tim0Prescaler )
	{
		return;
	}
	
	if ( a2dWatchingEye )
	{
		eyeEnable(); // turn on emitter, wait until the end of this routine to kick off the sampling
	}
	
	s_tim0Prescaler = FAST_TIMER_ISR_PRESCALE;

	// check+dec all the subcounters that slave from the primary
	if ( lastTriggerStateTransition )
	{
		lastTriggerStateTransition--;
	}
	
	if ( countsToReactiveTriggerDisable )
	{
		countsToReactiveTriggerDisable--;
	}

	if ( AFARateBox )
	{
		AFARateBox--;
	}

	// fet2 being used to run a hopper? turn it off now
	if ( accessoryRunTime )
	{
		if ( !--accessoryRunTime )
		{
			fet2Off();
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