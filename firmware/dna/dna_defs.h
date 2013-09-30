#ifndef DNA_DEFS_H
#define DNA_DEFS_H
/*------------------------------------------------------------------------------*
 * Copyright: (c) 2013 by Curt Hartung avr@northarc.com
 * This work is released under the Creating Commons 3.0 license
 * found at http://creativecommons.org/licenses/by-nc-sa/3.0/legalcode
 * and in the LICENCE.txt file included with this distribution
 */

//#define							0x01
//#define							0x02
#define DNA_AT84					0x03
#define BOOTLOADER_DNA_AT84			0x04

#define BOOTLOADER_ENTRY			0xCA0
#define OLED_BOOTLOADER_ENTRY		0xD00

#define DNA_PRODUCT_MASK			0x7F
#define DNA_RTS						0x80

#define DNA_VERSION					1

//------------------------------------------------------------------------------
enum BootloaderCommands
{
	BootloaderCommandCommitPage = 0xFE,
};

//------------------------------------------------------------------------------
enum RNABusDevices
{
	RNADeviceDNA = 0x1,
	RNADeviceOLED = 0x2,
	RNADeviceBUTTON = 0x3,
	RNADeviceTELEMETRY = 0x4,
};

// USB size constants
#define REPORT_DNA  0x01
#define REPORT_DNA_SIZE 0x07
#define REPORT_DNA_DATA 0x02
#define REPORT_DNA_DATA_SIZE 0x3F // apparantly 64 bytes is the most you can transfer over a HID interface

#define MAX_USER_DATA_REPORT_SIZE (REPORT_DNA_DATA_SIZE - 1) // the most data that can be sent at one time

//------------------------------------------------------------------------------
enum DNAUSBCommands
{
	USBCommandUser = 1,
	USBCommandEnterBootloader,
};

#endif
