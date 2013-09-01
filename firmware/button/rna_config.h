#ifndef RNA_CONFIG_H
#define RNA_CONFIG_H
/*------------------------------------------------------------------------------*/

#define RNA_PORT_LETTTER	B
#define RNA_PIN_NUMBER		2

#define RNA_MY_ADDRESS		0x3

#define RNA_BOOTLOADER_ENTRY 0x0

//------------------------------------------------------------------------------
const PROGMEM int rnaTrampoline[] =
{
	0xE6E0, // e0 e7  ldi r30, 0x60	-- ijmp to the bootloader
	0xE0FC, // fc e0  ldi r31, 0x0C
	0x9409, // 09 94  ijmp
};


#endif
