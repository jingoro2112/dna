#ifndef OLED_H
#define OLED_H
/*------------------------------------------------------------------------------*/

#define OLED_ADDRESS 0x78


#define oledCSHigh() (PORTB |= 0b00000100)
#define oledCSLow()  (PORTB &= 0b11111011)
#define oledResetHigh() (PORTB |= 0b00000010)
#define oledResetLow() (PORTB &= 0b11111101)

void oledCommand( unsigned char cmd );
void oledClear();
#define oledPowerOff() oledCommand( 0xAE )
void oledInit();

void oledSetPixel( unsigned char x, unsigned char y );
void oledClearPixel( unsigned char x, unsigned char y );

#endif

