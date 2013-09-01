#ifndef DNA_DEFS_H
#define DNA_DEFS_H
/*------------------------------------------------------------------------------*/
/* Copyright: (c) 2013 by Curt Hartung
 * This work is released under the Creating Commons 3.0 license
 * found at http://creativecommons.org/licenses/by-nc-sa/3.0/legalcode
 * and in the LICENCE.txt file included with this distribution
 */

#define OLED_AM88_v1_00				0x01
#define BOOTLOADER_OLED_AM88_v1_00	0x02
#define DNA_AT84_v1_00				0x03
#define BOOTLOADER_DNA_AT84_v1_00	0x04

#define BOOTLOADER_ENTRY			0xC80

#define DNA_PRODUCT_MASK			0x7F
#define DNA_RTS						0x80

//------------------------------------------------------------------------------
enum BootloaderCommands
{
	BootloaderCommandLoadZeroPage = 0xA5,
	BootloaderCommandCommitPage = 0xA6,
};


//------------------------------------------------------------------------------
enum RNABusDevices
{
	RNADeviceDNA = 0x1,
	RNADeviceOLED = 0x2,
	RNADeviceBUTTON = 0x3,
	RNADeviceTELEMETRY = 0x4,
};


// pins on the DNA
// B0  the clock input, unavailable for use
// B1  D- USB pin
// B2  D+ USB pin
// B3  servo/RNA bus wire
//
// A0  Pin8 MOSFET control (active high)
// A1  Pin3 input (pulled low with 3.6k)
// A2  Pin5 direct out, general purpose
// A3  Pin10 MOSFET control (active high)
// A4  Pin7 direct out, general purpose
// A5  Pin9 LED drive
// A6  Onboard LED drive (active low)
// A7  Battery Voltage input

//------------------------------------------------------------------------------
enum USBReports
{
	Report_Command = 1,
};

//------------------------------------------------------------------------------
enum DNAUSBCommands
{
	USBCommandCodePage = 1,
	USBCommandEnterApp,
	USBCommandEnterBootloader,
	USBCommandWriteData,
	USBCommandRNACommand,
};

//------------------------------------------------------------------------------
enum DNARNACommands
{
	RNACommandAppJump = 1,
	RNACommandCodePageWrite,
	
};

//------------------------------------------------------------------------------
enum StatusBits
{
	Status_RNAAddressBit0 = 1<<0,
	Status_RNAAddressBit1 = 1<<1,
	Status_RNAAddressBit2 = 1<<2,
	Status_RNAAddressBit4 = 1<<3,
	Status_DataFromMCUSetup = 1<<4,
	Status_CommandToMCUSetup = 1<<5,
	Status_DataToRNA = 1<<6,
};

#endif
