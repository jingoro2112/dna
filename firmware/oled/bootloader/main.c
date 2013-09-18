#include <dna.h>
#include <rna.h>

//------------------------------------------------------------------------------
unsigned char rnaInputSetup( unsigned char *data, unsigned char from, unsigned char len )
{
	return 0;
}

//------------------------------------------------------------------------------
void rnaInputStream( unsigned char *data, unsigned char bytes )
{
}

//------------------------------------------------------------------------------
int __attribute__((noreturn)) main(void)
{
	// todo- hold OLED in /reset?
	
	rnaInit( RNADeviceOLED );
	for(;;)
	{
		rnaPoll(); // trivial main, the RNA system commands will take over from here
	}
}

