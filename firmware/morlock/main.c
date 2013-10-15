/*------------------------------------------------------------------------------*
 * Copyright: (c) 2013 by Curt Hartung avr@northarc.com
 * This work is released under the Creating Commons 3.0 license
 * found at http://creativecommons.org/licenses/by-nc-sa/3.0/legalcode
 * and in the LICENCE.txt file included with this distribution
 */

#include <dna.h>
#include <usb.h>
#include <rna.h>
#include <galloc.h>

#include <avr/interrupt.h>
#include <avr/eeprom.h>
#include <util/delay.h>
#include <rna_packet.h>

#include "morlock_defs.h"

#ifndef true
#define true 1
#endif
#ifndef false
#define false 0
#endif

#define annunciatorOn() (PORTA |= 0b00000100)  // A2
#define annunciatorOff() (PORTA &= 0b11111011)  // A2
#define annunciatorToggle() (PORTA ^= 0b00000100)  // A2
#define isAnnunciatorOn() (PINA & 0b00000100) // A2
#define strongEyeEnable() (PORTA &= 0b10111111)  // A6
#define strongEyeDisable() (PORTA |= 0b01000000)  // A6
#define weakEyeEnable() (PORTA &= 0b11011111)  // A5
#define weakEyeDisable() (PORTA |= 0b00100000)  // A5

#define readTrigger() (PINA & 0b00010000)  // A4
// eye A1
#define fet1On() (PORTA |= 0b0000001)  // A0
#define fet1Off() (PORTA &= 0b1111110)  // A0
#define fet2On() (PORTA |= 0b0001000)  // A3
#define fet2Off() (PORTA &= 0b1110111)  // A3

#define setupA() (DDRA = 0b10001101); (PORTA = 0b00010100);
#define setupB() (DDRB = 0b00000000); (PORTB = 0b00000000);

#define LIGHT_ON_FOR					  400
#define LIGHT_OFF_FOR					  500
#define LIGHT_ON_FOR_FAST				  150
#define LIGHT_OFF_FOR_FAST				  200

//------------------------------------------------------------------------------
// a little maintenance but worth it for the memory effeciency, the
// compiler also handles this quite well.
#define NUMBER_OF_BIT_ENTRIES 2
struct Bits
{
	volatile uint8 b0:1;
	volatile uint8 b1:1;
	volatile uint8 b2:1;
	volatile uint8 b3:1;
	volatile uint8 b4:1;
	volatile uint8 b5:1;
	volatile uint8 b6:1;
	volatile uint8 b7:1;
} volatile bits[NUMBER_OF_BIT_ENTRIES];

// I can't think of a pre-compiler way to properly manage this
// hands-off, if I think of some sort of enum/typedef/macro-geddon I'll
// add it
#define eyeBlocked				(bits[0].b0) // is the eye blocked
#define eepromConstantsDirty	(bits[0].b1) // EEPROM constants need saving, this is amortized
#define triggerState			(bits[0].b2) // current state of the trigger (low is depressed)
#define triggerStateChangeValid	(bits[0].b3) // did the state change last long enough to 'count'?
#define startFireCycle			(bits[0].b4) // signal to the main loop to begine a single shot cycle
#define inProgramMode			(bits[0].b5)
#define a2dWatchingOther		(bits[0].b6) // is the a2d meant to be sampling the eye?
#define eyeFault				(bits[0].b7) // has the eye been detected as faulty

#define rnaPacketAvail			(bits[1].b0) //
#define blinkOn					(bits[1].b1) // while blinking, is the light on?
#define isLedOn					(bits[1].b2) // should the LED be on right now? this is filtered through dimmer and voltage check
//#define
#define hasButtonBoard			(bits[1].b4) // 
#define hasOLED					(bits[1].b5) //
//#define
#define sampleEye				(bits[1].b7) // should the eye be sampled at all?


volatile uint16 enhancedTriggerTimeout;
volatile uint8 shotsInString;
volatile uint8 millisecondCountBox;
volatile uint8 millisecondCountBox2;
volatile uint8 millisecondCountBox3;
volatile uint16 millisecondCountBoxLong;

uint8 accessoryRunTime;
uint16 antiBoltstickTimeout;

uint8 timesToBlinkLight;
uint16 blinkBox;
uint8 dimmerBox;
uint8 debounceBox;
uint16 triggerWavelength;
uint16 triggerWavelengthBox;
volatile uint8 burstCount;

uint8 usbCommand;

uint8 usbRNATo;
uint8 usbRNAPacket[132];
uint8 usbRNAPacketPos;
uint8 usbRNAPacketExpected;

uint8 eepromLoadPointer;
volatile struct EEPROMConstants consts;

// program mode
uint8 programSelectState;
uint8 currentEntry;
uint8 selectedRegister;
uint8 plusMinusDelta;
uint8 selectedValue;

volatile uint16 rampTimeoutBox;
volatile uint16 rampLevel;
volatile uint8 triggerTimer;
volatile uint8 currentFireMode;
volatile uint16 scheduleShotRate;
volatile uint16 scheduleShotBox;

//------------------------------------------------------------------------------
void digitizeEye()
{
	if ( consts.eyeStrong )
	{
		strongEyeEnable();
	}
	else
	{
		weakEyeEnable();
	}

	// wait for the detector to rise
	for( unsigned char d = 0; d<consts.eyeDetectHoldoff; d++ )
	{
		_delay_us( 5 );
	}

	ADCSRA |= (1<<ADSC); // start a conversion
	while( ADCSRA & (1<<ADSC) );

	consts.eyeLevel = ADCH;
	if ( consts.eyeLevel > consts.eyeDetectLevel ) // clock it in
	{
		eyeBlocked = consts.eyeHighBlocked ? 1 : 0;
	}
	else
	{
		eyeBlocked = consts.eyeHighBlocked ? 0 : 1;
	}

	weakEyeDisable();
	strongEyeDisable();
}

//------------------------------------------------------------------------------
void loadEEPROMConstants()
{
	for( unsigned int i=0; i<sizeof(consts); i++ )
	{
		((uint8*)&consts)[i] = eeprom_read_byte( (uint8*)i );
	}

	// do a one-time 32-bit calculation to extract the exact timer
	// count for refire. this is good enough in 16-bit but as long as
	// we have the space what the heck
	uint16 refireRegister = ((unsigned long)ROF_TIMER_NUMERATOR_X10 / consts.ballsPerSecondX10) - 1;
	OCR1AH = refireRegister >> 8;
	OCR1AL = refireRegister;
	currentFireMode = consts.fireMode;

	weakEyeDisable();
	strongEyeDisable();
	if ( consts.eyeStrong )
	{
		DDRA &= 0b11011111; // turn off weak driver
		DDRA |= 0b01000000; // turn on strong driver
	}
	else
	{
		DDRA &= 0b10111111; // turn off strong driver
		DDRA |= 0b00100000; // turn on weak driver
	}
}

//------------------------------------------------------------------------------
void saveEEPROMConstants()
{
	for( unsigned int i=0; i<sizeof(consts); i++ )
	{
		eeprom_write_byte( (uint8*)i, *(((uint8*)&consts) + i) );
		eeprom_busy_wait();
	}
	loadEEPROMConstants(); // be sure to do any translations that occur on load
}

//------------------------------------------------------------------------------
void dnaUsbCommand( unsigned char command, unsigned char data[5] )
{
	if ( command == ceCommandGetEEPROMConstants )
	{
		digitizeEye();
		consts.version = MORLOCK_CODE_VERSION;
		dnaUsbQueueData( (unsigned char *)&consts, sizeof(consts) );
	}

	usbCommand = ceCommandIdle;
}

//------------------------------------------------------------------------------
unsigned char dnaUsbInputSetup( unsigned char totalSize, unsigned char *data, unsigned char len )
{
	// if we are not processing a command, get one, otherwise interpret
	// as a continuation
	if ( usbCommand == ceCommandIdle )
	{
		usbCommand = data[0];

		if ( usbCommand == ceCommandRNASend )
		{
			usbRNATo = data[1];
			usbRNAPacketExpected = data[2];
			usbRNAPacketPos = 0;
			return 3;
		}
		
		return 1; // consumed command
	}

	return 0; // consumed nothing
}

//------------------------------------------------------------------------------
void dnaUsbInputStream( unsigned char *data, unsigned char len )
{
	if ( usbCommand == ceCommandIdle ) // fast-fail
	{
		return;
	}
	else if ( usbCommand == ceCommandSetEEPROMConstants )
	{
		unsigned char i;
		for( i=0; i<len; i++ )
		{
			((unsigned char *)&consts)[eepromLoadPointer++] = data[i];
		}

		if ( eepromLoadPointer >= sizeof(consts) )
		{
			eepromConstantsDirty = true;
			eepromLoadPointer = 0;
			usbCommand = ceCommandIdle;
		}
	}
	else if ( usbCommand == ceCommandRNASend )
	{
		for( unsigned int i=0; i<len && (usbRNAPacketPos < usbRNAPacketExpected) ; i++ )
		{
			usbRNAPacket[usbRNAPacketPos++] = data[i];
		}

		if ( usbRNAPacketPos >= usbRNAPacketExpected )
		{
			usbCommand = ceCommandIdle;
			rnaPacketAvail = true;
		}
	}
	else
	{
		// unknown command, set to idle and hope for a recovery
		usbCommand = ceCommandIdle;
	}
}

//------------------------------------------------------------------------------
void cycleSingleSolenoid()
{
	if ( consts.eyeEnabled )
	{
		digitizeEye(); // check it

		if ( !eyeBlocked )
		{
			millisecondCountBoxLong = 400;
			while( millisecondCountBoxLong )
			{
				// break the count if the trigger is released on an
				// empty chamber, or the ball fell within 100ms of a
				// scheduled shot, or we are in semi-auto
				if ( triggerState
					 && ((millisecondCountBoxLong < 300) || currentFireMode == ceSemi) )
				{
					startFireCycle = false;
					return;
				}

				if ( eyeBlocked ) // that's what we were waiting for, a ball to drop
				{
					break;
				}
			}

			millisecondCountBox = consts.eyeHoldoff; // wait for ball to 'seat'
			while( millisecondCountBox );
		}
		
		// else: assume enough time has passed, no need to wait for the eyeHoldoff
	}

	fet1On();

	if ( consts.accessoryRunTime )
	{
		accessoryRunTime = consts.accessoryRunTime;
		fet2On();
	}

	millisecondCountBox = consts.dwell1;

	if ( !antiBoltstickTimeout && consts.ABSTimeout )
	{
		antiBoltstickTimeout = consts.ABSTimeout;
		millisecondCountBox += consts.ABSAddition;
	}

	while( millisecondCountBox );

	fet1Off();

	if ( consts.eyeEnabled )
	{
		// wait the holdoff to make sure the bolt is on its way back,
		// and we're not seeing the space between the ball and
		// the bolt, which would short-cycle the gun thinking the bolt
		// was another ball
		millisecondCountBox = consts.boltHoldoff;
		while( millisecondCountBox );
	}
}

//------------------------------------------------------------------------------
void cycleDoubleSolenoid()
{
	millisecondCountBox = consts.dwell1;
	millisecondCountBox2 = consts.dwell2Holdoff;
	millisecondCountBox3 = 0;

	fet1On(); // fire!

	while( millisecondCountBox )
	{
		// if programmed to start the bolt back BEFORE the fire
		// solenoid has actually completed.. well okay do that, but
		// make sure we handle the sniper-mode corner case, namely
		// DON'T honor the "start before fire complete" if in sniper
		// mode and the fire rate is low enough
		if ( !millisecondCountBox2 &&
			 ( (currentFireMode != ceSniper)
			  || (currentFireMode == ceSniper && (triggerWavelength < 1000))) )
		{
			fet2On();
			millisecondCountBox3 = consts.dwell2;
		}
	}

	fet1Off();

	while( millisecondCountBox2 ); // wait for holdoff

	// if the bolt has not already started back, and we are in sniper
	// mode and not firing too quickly, wait for the trigger to be
	// released before cycling the bolt
	if ( !millisecondCountBox3 )
	{
		if ( (currentFireMode == ceSniper) && (triggerWavelength >= 1000) )
		{
			while( !triggerState );
		}
	}

	fet2On(); // start bolt opening
	if ( !millisecondCountBox3 )
	{
		millisecondCountBox3 = consts.dwell2;
	}

	if ( consts.eyeEnabled )
	{
		while( millisecondCountBox3 && eyeBlocked ); // wait for eye to unblock indicating the bolt has opened

		if ( !millisecondCountBox3 )
		{
			// took too long, eye must be busted
			eyeFault = true;
		}
		else
		{
			eyeFault = false;

			millisecondCountBoxLong = 400;
			while( millisecondCountBoxLong && !eyeBlocked ); // now wait for paint to drop

			if ( !millisecondCountBoxLong )
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
		while( millisecondCountBox3 ); // wait for pulse, then check eye
	}

	// cool mode if you hate your battery
	if ( currentFireMode == ceCocker && (triggerWavelength >= 1000) ) 
	{
		millisecondCountBoxLong = 750;
		while( !triggerState && millisecondCountBoxLong );
	}

	fet2Off();
}


//------------------------------------------------------------------------------
unsigned char rnaInputSetup( unsigned char *data, unsigned char dataLen, unsigned char from, unsigned char packetLen )
{
	usbRNAPacketPos = 0;
	usbRNAPacketExpected = packetLen;
	return 0;
}

//------------------------------------------------------------------------------
void rnaInputStream( unsigned char *data, unsigned char dataLen )
{
	for( unsigned char i=0; i<dataLen; i++ )
	{
		usbRNAPacket[usbRNAPacketPos++] = data[i];
	}

	if ( usbRNAPacketPos < usbRNAPacketExpected )
	{
		return;
	}

	if ( *usbRNAPacket == RNATypeConsoleString )
	{
//		dprint( (char *)usbRNAPacket + 1, usbRNAPacketExpected - 1  );
	}
}

//------------------------------------------------------------------------------
void setupVariables()
{
	for( unsigned char c = 0; c<NUMBER_OF_BIT_ENTRIES; c++ )
	{
		*(((unsigned char*)&bits) + c) = 0;
	}

	usbCommand = ceCommandIdle;
	rampTimeoutBox = 1; // let this timeout immediately so the defaults will be instaled
}

//------------------------------------------------------------------------------
int __attribute__((OS_main)) main(void)
{
	setupA();
	setupB();
	setupVariables();

	dnaUsbInit();

//	INSTALL_MORLOCK_DEFAULTS;
//	saveEEPROMConstants();

	loadEEPROMConstants();

	// 12000000 / (64 * 94) = 1994.680 interrupts per second
	// accomplished by using timer0 in waveform generation mode 2
	TCCR0B = 1<<CS01 | 1<<CS00; // set 8-bit timer prescaler to div/64
	OCR0A = 94;
	TCCR0A = 1<<WGM01; 
	TIMSK0 = 1<<OCIE0A; // mode 2, reset counter when it reaches OCROA

	TCCR1B = 1<<WGM12 | 1<<CS12;// CTC for OCR1A, clock select

	// set up A2D
	ADMUX = 1<<MUX0; // A1, Vcc ref
	ADCSRA = 1<<ADEN | 1<<ADPS2; // enable A2D, x16
	DIDR0 = ADC1D; // disable all digital function on A1
	ADCSRB = 1<<ADLAR; // knock off lower two bits, implementation is not that accurate

	PRR = 1<<PRUSI; // not using USI (yet)
	
	rnaInit();

	sei();


	_delay_ms(2); // let state settle, and make sure housekeeping ISR runs

	if( !triggerState && !consts.locked )
	{
		timesToBlinkLight = 1;
		inProgramMode = true;
	}

	for(;;)
	{
		PRR |= 1<<PRTIM1; // power down timer, don't waste power

		// spin until a fire condition is triggered from the ISR
		while( !startFireCycle )
		{
			sampleEye = false;

			if ( !millisecondCountBox && consts.eyeEnabled )
			{
				millisecondCountBox--;
				digitizeEye();
				isLedOn = eyeBlocked ? true : false;
			}

			if ( eepromConstantsDirty )
			{
				isLedOn = true;
				setLedOn();
				eepromConstantsDirty = false;
				saveEEPROMConstants();
				isLedOn = false;
			}

			if ( rnaPacketAvail )
			{
				isLedOn = true;
				setLedOn();
				rnaPacketAvail = false;
				_delay_ms( 50 );
				rnaSend( usbRNATo, usbRNAPacket, usbRNAPacketExpected );
				isLedOn = false;
			}
		}

		sampleEye = true;

		// set up the timer
		PRR &= ~(1<<PRTIM1); // timer back on
		TCNT1 = 0; // reset the timer
		TIFR1 |= 1<<OCF1A; // reset compare match

		if ( shotsInString < consts.rampEnableCount )
		{
			shotsInString++;
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

		// bursting? if so count it down
		if ( burstCount && --burstCount )
		{
			startFireCycle = true;
		}
		else if ( currentFireMode != ceFullAuto )
		{
			startFireCycle = false;
		}

		isLedOn = false;

		// make sure at least this much time elapses at the end of a fire cycle
		millisecondCountBox = consts.shortCyclePreventionInterval;
		while( millisecondCountBox );

		while( !(TIFR1 & 1<<OCF1A) ); // wait for end of fire cycle

		if ( currentFireMode == ceRamp && startFireCycle && (consts.rampTopMode != ceSemi) ) // officially hit top rate, blow guts out
		{
			// a shot was scheduled at the maximum rate it could be, go
			// ahead and shift up to whatever top mode the user wanted
			currentFireMode = consts.rampTopMode;
			scheduleShotBox = 0;
			scheduleShotRate = 0;
		}
	}
}

//------------------------------------------------------------------------------
// entered 1994.7 times per second
ISR( TIM0_COMPA_vect, ISR_NOBLOCK )
{
	if ( scheduleShotBox )
	{
		if ( !--scheduleShotBox  && (currentFireMode == ceRamp) && enhancedTriggerTimeout )
		{
			startFireCycle = true;
			scheduleShotBox = scheduleShotRate;
		}
	}

	if ( !isAnnunciatorOn() )
	{
		if ( debounceBox ) // programmable debounce, sample every X milliseconds
		{
			debounceBox--;
		}
		else if ( (triggerState && !readTrigger()) || (!triggerState && readTrigger()) )
		{
			// rebounce check part 2, make sure the state change lasts long enough
			if ( !triggerStateChangeValid && consts.rebounce )
			{
				debounceBox = consts.rebounce;
				triggerStateChangeValid = true; // next time around it counts
			}
			else
			{
				debounceBox = consts.debounce;

				triggerState = readTrigger() ? true : false;

				if ( !triggerState ) // been pressed
				{
					triggerWavelength = triggerWavelengthBox;
					triggerWavelengthBox = 0;

					if ( inProgramMode )
					{
						millisecondCountBoxLong = MS_UNTIL_ENTRY_VALID;
						currentEntry++;
					}
					else
					{
						// ramping and in a qualified-long-enough string?
						if ( (currentFireMode == ceRamp) && (shotsInString >= consts.rampEnableCount) )
						{
							// RAMPING

							// get the current rate and schedule a shot for
							// 50% faster in the future base, increasing
							// per ramp climb
							scheduleShotRate = triggerWavelength - (triggerWavelength / rampLevel);
							rampLevel += consts.rampClimb;

							if ( !scheduleShotBox || (scheduleShotBox > scheduleShotRate) )
							{
								scheduleShotBox = scheduleShotRate;
							}
						}

						startFireCycle = true;

						// trigger has been depressed, reset the "do what you're doing" timeout
						rampTimeoutBox = consts.rampTimeout; 

						// after this many milliseconds Enhanced will not fire
						enhancedTriggerTimeout = consts.enhancedTriggerTimeout;

						if ( currentFireMode == ceBurst )
						{
							burstCount = consts.burstCount;
						}
					}
				}
				else if ( !inProgramMode ) // been released 
				{
					if ( (currentFireMode == ceAutoresponse) && enhancedTriggerTimeout )
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
	}

	// duty cycle for LED, effecting a dimmer
	if ( isLedOn )
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

	//--------------------------------------------------------------------------------
	// everything below here has a ~1ms resulotion
	static uint8 s_ms;
	if ( ++s_ms & 0x01 )
	{
		return;
	}

	annunciatorToggle();
	
	if ( triggerWavelengthBox < 1000 )
	{
		triggerWavelengthBox++;
	}
	
	if ( enhancedTriggerTimeout )
	{
		enhancedTriggerTimeout--;
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

	if ( antiBoltstickTimeout )
	{
		antiBoltstickTimeout--;
	}

	if ( rampTimeoutBox )
	{
		if ( !--rampTimeoutBox )
		{
			shotsInString = 0;
			rampLevel = 2;
			currentFireMode = consts.fireMode;
			scheduleShotBox = 0;
			scheduleShotRate = 0;
		}
	}
	
	// fet2 being used to run a hopper? turn it off now
	if ( accessoryRunTime && !--accessoryRunTime )
	{
		fet2Off();
	}

	// handle program mode activity
	if ( millisecondCountBoxLong ) // waiting for idle trigger? (signifying an entry has been made)
	{
		if ( !--millisecondCountBoxLong && triggerState && inProgramMode ) // this the one?
		{
			if ( programSelectState == ceStateSelectingRegister )
			{
				if ( currentEntry >= ceRegisterLast )
				{
					timesToBlinkLight = 4;
				}
				else
				{
					programSelectState++;
					selectedRegister = currentEntry;
					if ( selectedRegister == ceRegisterFireMode )
					{
						programSelectState++; // for fire mode, jump right to set
					}
					else if ( selectedRegister == ceRegisterEyeToggle )
					{
						consts.eyeEnabled = consts.eyeEnabled ? false : true;
						eepromConstantsDirty = true;
						programSelectState--;
					}
					
					timesToBlinkLight = 1;
				}
			}
			else if ( programSelectState == ceStateSelectingDeltaMethod )
			{
				if ( currentEntry >= 4 )
				{
					timesToBlinkLight = 4;
					programSelectState = ceStateSelectingRegister;
				}
				else
				{
					programSelectState++;
					plusMinusDelta = currentEntry;
					timesToBlinkLight = 1;
				}
			}
			else
			{
				programSelectState = ceStateSelectingRegister;

				if ( selectedRegister == ceRegisterFireMode )
				{
					consts.fireMode = currentEntry;
					timesToBlinkLight = consts.fireMode;
					inProgramMode = false;
				}
				else if ( selectedRegister == ceRegisterFireRate )
				{
					if ( plusMinusDelta == 1 )
					{
						consts.ballsPerSecondX10 += currentEntry;
					}
					else if ( plusMinusDelta == 2 )
					{
						consts.ballsPerSecondX10 -= currentEntry;
					}
					else
					{
						consts.ballsPerSecondX10 = currentEntry;
					}
					
					timesToBlinkLight = consts.ballsPerSecondX10 / 10;
				}
				else if ( selectedRegister == ceRegisterDwell1 )
				{
					if ( plusMinusDelta == 1 )
					{
						consts.dwell1 += currentEntry;
					}
					else if ( plusMinusDelta == 2 )
					{
						consts.dwell1 -= currentEntry;
					}
					else
					{
						consts.dwell1 = currentEntry;
					}

					timesToBlinkLight = consts.dwell1;
				}
				else // dwell2
				{
					if ( plusMinusDelta == 1 )
					{
						consts.dwell2 += currentEntry;
					}
					else if ( plusMinusDelta == 2 )
					{
						consts.dwell2 -= currentEntry;
					}
					else
					{
						consts.dwell2 = currentEntry;
					}

					timesToBlinkLight = consts.dwell2;
				}

				eepromConstantsDirty = true;
			}

			currentEntry = 0;
		}
	}
	
	// if the LED is being told to blink, handle that here
	if ( timesToBlinkLight )
	{
		if ( !blinkBox )
		{
			if ( blinkOn )
			{
				blinkOn = false;
				isLedOn = true;
				blinkBox = LIGHT_ON_FOR;
			}
			else
			{
				blinkOn = true;
				isLedOn = false;
				blinkBox = LIGHT_OFF_FOR;
				
				if ( !--timesToBlinkLight )
				{
					isLedOn = inProgramMode;
				}
			}
		}
		else
		{
			blinkBox--;
		}
	}

	usbPoll(); // check for USB activity

	// only check the eye when we need to, as the emitter consumes non-trivial power
	if ( consts.eyeEnabled && sampleEye )
	{
		digitizeEye();
	}
}
