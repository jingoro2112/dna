/*------------------------------------------------------------------------------*
 * Copyright: (c) 2013 by Curt Hartung avr@northarc.com
 * This work is released under the Creating Commons 3.0 license
 * found at http://creativecommons.org/licenses/by-nc-sa/3.0/legalcode
 * and in the LICENCE.txt file included with this distribution
 */

#include <dna.h>
#include <usb.h>

#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>


//------------------------------------------------------------------------------
int __attribute__((OS_main)) main(void)
{
	dnaUsbInit(); // will initialize the port and start listening

	sei(); // turning interrupts on starts the system going

	enableLed();
	setLedOn();

	for(;;)
	{
		usbPoll(); // must be called at least once every 50ms to service USB data


		// user code here
	}
}

//------------------------------------------------------------------------------
void dnaUsbCommand( unsigned char command, unsigned char data[5] )
{
	// called when a USB command is recieved,

	// this call is an atomic callback from usbPoll()
}

//------------------------------------------------------------------------------
unsigned char dnaUsbInputSetup( unsigned char totalSize, unsigned char *data, unsigned char len )
{
	// called when a USB command is being sent. This is the "setup"
	// callback which will contain some (up to 7 bytes) of data, also
	// it indicates the total amount of data that will be delivered, if
	// there will be more.
	// return the number of bytes consumed (0 to len) the remaining
	// bytes (and all subsequent data for this data stream) will be
	// sent through dnaUsbInputStream() below, in up to 8-byte
	// increments

	// this call is an atomic callback from usbPoll()

	return 0; // consumed nothing
}

//------------------------------------------------------------------------------
void dnaUsbInputStream( unsigned char *data, unsigned char len )
{
	// called if there is any more data to consume after
	// dnaUsbInputSetup() is called, it will deliver from 1 to 8 bytes
	// at a time, up to the total amount of data being transmitted (up
	// to 255 bytes in aggregate)

	// this call is an atomic callback from usbPoll()
}
