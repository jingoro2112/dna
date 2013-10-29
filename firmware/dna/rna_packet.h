#ifndef RNA_PACKET_H
#define RNA_PACKET_H
/*------------------------------------------------------------------------------*
 * Copyright: (c) 2013 by Curt Hartung avr@northarc.com
 * This work is released under the Creating Commons 3.0 license
 * found at http://creativecommons.org/licenses/by-nc-sa/3.0/legalcode
 * and in the LICENCE.txt file included with this distribution
 */

#include "dna_types.h"

//------------------------------------------------------------------------------
enum RNAPacketTypes
{
	RNATypeNone = 0,
	RNATypeEnterBootloader = 1,
	RNATypeCodePage,
	RNATypeEnterApp,
	RNATypeEEPROMLoad,
	RNATypeButtonStatus, // single byte bitvector of the buttons status, 0xFF means power off request
	RNATypeReplay,

	RNATypeOledClear,
	RNATypeOledPixel,
	RNATypeOledConsole,
	RNATypeOledText,
	RNATypeOledLine,

	RNATypeGetConfigData,
	RNATypeSetConfigData,

	RNATypeRequestButtonStatus,

	RNATypeSetConfigItem,
};

//------------------------------------------------------------------------------
struct PacketCodePage
{
	uint16 page;
	uint16 code[32];
};

//------------------------------------------------------------------------------
struct PacketButtonCodePage
{
	uint16 page;
	uint16 code[16];
};

//------------------------------------------------------------------------------
struct PacketEnterApp
{
	uint16 lastAddress;
	uint16 checksum;
};

//------------------------------------------------------------------------------
struct PacketEEPROMLoad
{
	uint16 offset;
	uint8 data[128];
};

//------------------------------------------------------------------------------
struct PacketReplay
{
	unsigned char millisecondsRepresented; // how much time is valid

	// each bit is always exactly 1 millisecond
	unsigned char trigger[16];
	unsigned char eye[16];
	unsigned char solenoid1[16];
	unsigned char solenoid2[16];
};

#endif