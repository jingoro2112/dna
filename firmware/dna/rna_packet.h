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
	RNATypeEnterBootloader,
	RNATypeCodePage,
	RNATypeEnterApp,
	RNATypeEEPROMLoad,
};

//------------------------------------------------------------------------------
struct PacketCodePage
{
	uint16 page;
	uint16 code[32];
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

#endif