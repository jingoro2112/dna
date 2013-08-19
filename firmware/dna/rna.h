#ifndef RNA_H
#define RNA_H
/*------------------------------------------------------------------------------*/

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <rna_config.h>

#define RNA_CONCAT(x, y)  x##y
#define RNA_D_PORT(port)  RNA_CONCAT( PORT, port )
#define RNA_D_PIN(port)  RNA_CONCAT( PIN, port )
#define RNA_D_DDR(port)  RNA_CONCAT( DDR, port )
#define RNA_PORT  RNA_D_PORT( RNA_PORT_LETTTER )
#define RNA_PIN  RNA_D_PIN( RNA_PORT_LETTTER )
#define RNA_DDR  RNA_D_DDR( RNA_PORT_LETTTER )

#define RNA_ADDRESS_MASK 0x3F
#define RNA_RW_BIT 0x80
#define RNA_RW_BIT_MASK 0x7F
#define RNA_COMMAND_BIT 0x40
#define RNA_COMMAND_BIT_MASK 0xBF

//#define rnaSetHigh() (RNA_PORT |= (1<<RNA_PIN))
//#define rnaSetLow() (RNA_PORT &= ~(1<<RNA_PIN))

#define rnaSetHigh() (RNA_DDR &= ~(1<<RNA_PIN_NUMBER))
#define rnaSetLow() (RNA_DDR |= (1<<RNA_PIN_NUMBER))
#define rnaIsHigh() (RNA_PIN & (1<<RNA_PIN_NUMBER))
#define rnaIsLow()  (!(RNA_PIN & (1<<RNA_PIN_NUMBER)))


#if defined _AVR_IOTNX4_H_
#define rnaEnableINT0() (GIMSK |= (1 << INT0))
#define rnaClearINT0() (GIFR |= (1<<INTF0))
#define rnaDisableINT0() (GIMSK &= ~(1<<INT0))
#define rnaINT0ToFallingEdge() (MCUCR |= (1<<ISC01)); (MCUCR &= ~(1<<ISC00))
#endif

// INT11 (/reset)
//#define rnaEnablePCINT() (GIFR |= (1<<PCIF1)); (GIMSK |= (1<<PCIE1)
//#define rnaDisablePCINT() (GIMSK &= ~(1<<PCIE1)
//#define rnaPCINTarm()  (PCMSK1 |= (1<<PCINT11))
//#define rnaPCINTDisarm() (PCMSK1 &= ~(1<<PCINT11))

// INT2 (A2)
#define rnaEnablePCINT() (GIMSK |= (1<<PCIE0))
#define rnaClearPCINT() (GIFR |= (1<<PCIF0))
#define rnaDisablePCINT() (GIMSK &= ~(1<<PCIE0))
#define rnaPCINTArm() (PCMSK0 |= (1<<PCINT2))
#define rnaPCINTDisarm() (PCMSK0 &= ~(1<<PCINT2))

#if defined _AVR_IOMX8_H_
#define rnaEnableINT1()  (EIMSK |= (1 << INT1))
#define rnaClearINT1()  (EIFR |= (1<<INTF1))
#define rnaDisableINT1() (EIMSK &= ~(1 << INT1))
#define rnaINT1ToFallingEdge() (EICRA |= (1 << ISC11)); (EICRA &= ~(1 << ISC10))
#endif

// rna time constant used in delay loops for baud sync
#if F_CPU == 12000000
#define RNA_T_CONST_HALF_SETUP  6
#define RNA_T_CONST_SAMPLE 10
#define RNA_T_CONST_SAMPLE_REMAINDER 3
#define RNA_T_CONST_SETUP_EXTEND 0
#define RNA_T_CONST_ACK_WAIT 2
#define RNA_T_CONST_ACK_SETUP 6
#elif F_CPU == 8000000
#define RNA_T_CONST_HALF_SETUP 3
#define RNA_T_CONST_SAMPLE 6
#define RNA_T_CONST_SAMPLE_REMAINDER 2
#define RNA_T_CONST_SETUP_EXTEND 1
#define RNA_T_CONST_ACK_WAIT 1
#define RNA_T_CONST_ACK_SETUP 3
#endif

void rnaInit( unsigned char peerAddress );
unsigned char rnaShiftOutByte( unsigned char data );
unsigned char rnaShiftInByte();

void rnaSend( unsigned char address, unsigned char *data, unsigned char len );
unsigned char rnaProbe( unsigned char address );

unsigned char rnaInputSetup( unsigned char *data, unsigned char from, unsigned char len );
void rnaInputStream( unsigned char *data, unsigned char bytes );


#endif
