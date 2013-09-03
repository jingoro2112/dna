#ifndef RNA_CONFIG_H
#define RNA_CONFIG_H
/*------------------------------------------------------------------------------*/

// port and number the RNA communicates over. the protocol will take
// this port over and set its direciton appropriately upon Init
#define RNA_PORT_LETTTER	B
#define RNA_PIN_NUMBER		2

// the address of this node, range is 0x1 - 0xF
#define RNA_MY_ADDRESS		0x2 // RNADeviceOLED

#define RNA_BOOTLOADER_ENTRY 0x0

// this is used by the bootloader, put in only if the bootloader is
// going ot be used, note you may need ot change where it jumps to
/*
const PROGMEM int rnaTrampoline[] =
{
	0xE6E0, // e0 e7  ldi r30, 0x60	-- ijmp to the bootloader
	0xE0FC, // fc e0  ldi r31, 0x0C
	0x9409, // 09 94  ijmp
};
*/

#endif
