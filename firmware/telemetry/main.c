#include <rna.h>
/*------------------------------------------------------------------------------*
 * Copyright: (c) 2013 by Curt Hartung avr@northarc.com
 * This work is released under the Creating Commons 3.0 license
 * found at http://creativecommons.org/licenses/by-nc-sa/3.0/legalcode
 * and in the LICENCE.txt file included with this distribution
 */

//------------------------------------------------------------------------------
unsigned char rnaInputSetup( unsigned char *data, unsigned char dataLen, unsigned char from, unsigned char totalLen )
{
	return 0;
}

//------------------------------------------------------------------------------
void rnaInputStream( unsigned char *data, unsigned char dataLen )
{
}

//------------------------------------------------------------------------------
int __attribute__((OS_main)) main(void)
{
	rnaInit();
	sei();
	
	char c[100];
	
	for(;;)
	{
		_delay_ms(100);
		rnaSend( 0x5, c, 100 );
	}
}
