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

#define rnaSetHigh() (RNA_DDR &= ~(1<<RNA_PIN_NUMBER))
#define rnaSetLow() (RNA_DDR |= (1<<RNA_PIN_NUMBER))
#define rnaIsHigh() (RNA_PIN & (1<<RNA_PIN_NUMBER))
#define rnaIsLow()  (!(RNA_PIN & (1<<RNA_PIN_NUMBER)))

#ifndef RNA_POLL_DRIVEN
#if defined (PROTO88)
	#define rnaEnableINT()  (EIMSK |= (1 << INT1))
	#define rnaClearINT()  (EIFR |= (1<<INTF1))
	#define rnaDisableINT() (EIMSK &= ~(1 << INT1))
	#define rnaINTArm() (EICRA |= (1 << ISC11)); (EICRA &= ~(1 << ISC10))
#elif defined (DNA)
	#define rnaEnableINT() (GIMSK |= (1<<PCIE1)
	#define rnaClearINT() (GIFR |= (1<<PCIF1))
	#define rnaDisableINT() (GIMSK &= ~(1<<PCIE1)
	#define rnaINTarm()  (PCMSK1 |= (1<<PCINT11))
#elif defined (DNAPROTO)
	#define rnaEnableINT() (GIMSK |= (1<<PCIE0))
	#define rnaClearINT() (GIFR |= (1<<PCIF0))
	#define rnaDisableINT() (GIMSK &= ~(1<<PCIE0))
	#define rnaINTArm() (PCMSK0 |= (1<<PCINT2))
#elif defined (OLED)
	#define rnaEnableINT() (GIMSK |= (1 << INT0))
	#define rnaClearINT() (GIFR |= (1<<INTF0))
	#define rnaDisableINT() (GIMSK &= ~(1<<INT0))
	#define rnaINTArm() (MCUCR |= (1<<ISC01)); (MCUCR &= ~(1<<ISC00))
#else
	#error Unsupported hardware platform for RNA bus
#endif
#else
	#define rnaEnableINT()
	#define rnaClearINT()
	#define rnaDisableINT()
	#define rnaINTArm()
#endif

extern unsigned int rnaHeapStart; // initialized after rnaInit to be __heap_start

void rnaInit( unsigned char peerAddress );
unsigned char rnaShiftOutByte( unsigned char data, unsigned char high );
unsigned char rnaShiftInByte( unsigned char high );

void rnaSend( unsigned char address, unsigned char *data, unsigned char len );
void rnaSendSystem( unsigned char address, unsigned char *data, unsigned char len );
unsigned char rnaProbe( unsigned char address );
void rnaPoll();

unsigned char rnaInputSetup( unsigned char *data, unsigned char from, unsigned char len );
void rnaInputStream( unsigned char *data, unsigned char bytes );


#endif
