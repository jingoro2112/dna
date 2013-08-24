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

#define BOOTLOADER_ENTRY			0x0C60

#define DNA_PRODUCT_MASK			0x7F
#define DNA_RTS						0x80


//------------------------------------------------------------------------------
// RNABusDevices
#define RNADeviceDNA 0x1
#define RNADeviceOLED 0x2
#define RNADeviceBUTTON 0x3
#define RNADeviceTELEMETRY 0x4


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
};

//------------------------------------------------------------------------------
enum DNARNACommands
{
	RNACommandCodePage = 1,
	RNACommandEnterApp,
	RNACommandEnterBootloader,
};

//------------------------------------------------------------------------------
enum StatusBits
{
	Status_DataFromMCUSetup = 1<<0,
	Status_CommandToMCUSetup = 1<<1,
};

#endif
