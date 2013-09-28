/* Copyright: (c) 2013 by Curt Hartung avr@northarc.com
 * This work is released under the Creating Commons 3.0 license
 * found at http://creativecommons.org/licenses/by-nc-sa/3.0/legalcode
 * and in the LICENCE.txt file included with this distribution
 */

#include "stdafx.h"
#include "morlock.h"

#include "comm_thread.hpp"
#include "morlockDlg.h"
#include "Settings.h"

#include "../dnausb/dnausb.h"

#include "../splice/splice.hpp"

#include "../firmware/dna/dna_defs.h"

#include "../util/architecture.hpp"
#include "../util/http_comm.hpp"

#include "../firmware/morlock/morlock_defs.h"

extern EEPROMConstants g_morlockConstants;

namespace USBComm
{

CMorlockDlg* g_morlockDialog = 0;
CSettings* g_settingsDialog = 0;
bool g_active = true;
bool g_morlockConstantsNeedCommit = false;
bool g_connected = false;
Cstr g_codeImage;
bool g_reflash = false;

//------------------------------------------------------------------------------
void flashStatus( const unsigned int percent )
{
	if ( g_settingsDialog )
	{
		g_settingsDialog->m_progress->SetPos( (percent * 2) / 3 );
	}
}

//------------------------------------------------------------------------------
void morlockCommThread( void* arg )
{
	bool nagged = false;
	unsigned char buffer[256];
	Cstr temp;
	DNADEVICE device = INVALID_DNADEVICE_VALUE;
	bool stringsSet = false;
	g_connected = false;
	unsigned char productId = 0;
		
	while( g_active )
	{
		Sleep( 100 ); // basically polling for changes

		unsigned char version;
		if ( device == INVALID_DNADEVICE_VALUE )
		{
			productId = 0;
			g_connected = false;
			if ( !stringsSet )
			{
				stringsSet = true;
				g_morlockDialog->SetDlgItemText( IDC_PRODUCT, "n/a" );
				g_morlockDialog->SetDlgItemText( IDC_STATUS, "DISCONNECTED" );
			}

			device = DNAUSB::openDevice( 0x16C0, 0x05DF, "p@northarc.com", 0 );
			if ( device == INVALID_DNADEVICE_VALUE )
			{
				continue;
			}
			
			g_morlockDialog->SetDlgItemText( IDC_STATUS, "negotiating.." );

			if ( !DNAUSB::getProductId(device, &productId, &version) )
			{
				goto disconnected;
			}

			g_morlockDialog->SetDlgItemText( IDC_PRODUCT, temp.format("%s %d", Splice::stringFromId(productId, (char *)buffer), (int)version) );

			g_connected = true;
			g_morlockDialog->SetDlgItemText( IDC_STATUS, "connected" );

			if ( productId == BOOTLOADER_DNA_AT84 )
			{
				version = *buffer;
			}
			else if ( productId == DNA_AT84 )
			{
				if ( !DNAUSB::sendCommand(device, ceCommandGetEEPROMConstants) )
				{
					goto disconnected;
				}

				Arch::sleep(10);

				unsigned char size;
				if ( !DNAUSB::getData(device, buffer, &size) )
				{
					goto disconnected;
				}

				memcpy( &g_morlockConstants, buffer, sizeof(g_morlockConstants) );
				g_morlockConstants.transposeValues();

				g_morlockDialog->populateDialogFromConstants();
				g_morlockConstantsNeedCommit = false;

				g_morlockDialog->SetDlgItemText( IDC_PRODUCT, temp.format("%s %d M%d", Splice::stringFromId(productId, (char *)buffer), (int)version, g_morlockConstants.version) );

				if ( !nagged )
				{
					nagged = true;
					Cstr latestVersion;
					if ( HTTP::fetchDocument(latestVersion, "northarc.com", "dna_publish/morlock_current_version.txt") )
					{
						unsigned char ver = (unsigned char)atoi( latestVersion );
						if ( ver != g_morlockConstants.version )
						{
							g_morlockDialog->MessageBox( latestVersion.format( "Old version of Morlock code detected:%d suggest flashing current version:%d",
																			   (int)g_morlockConstants.version, (int)ver ),
														 "info",
														 MB_OK);
						}
					}
				}
			}
			else
			{
				g_morlockDialog->MessageBox( temp.format("Unrecognized device 0x%02X", productId), "error", MB_OK | MB_ICONSTOP);
				goto disconnected;
			}
		}

		if ( g_reflash )
		{
			g_reflash = false;

			DNAUSB::closeDevice( device );
			device = INVALID_DNADEVICE_VALUE;

			Arch::sleep( 2000 );

			DNAUSB::sendCode( 0x16C0,
							  0x05DF,
							  "p@northarc.com",
							  (const unsigned char *)g_codeImage.c_str(),
							  g_codeImage.size(),
							  flashStatus );

			nagged = false;
			
			if ( g_settingsDialog )
			{
				g_settingsDialog->m_progress->SetPos( 100 );
				Sleep( 500 );
				g_settingsDialog->m_progress->SetPos( 0 );
				g_settingsDialog->enableControls();
			}
		}

		if ( productId == DNA_AT84 )
		{
			if ( g_morlockConstantsNeedCommit )
			{
				if ( g_settingsDialog )
				{
					g_settingsDialog->m_progress->SetPos( 25 );
				}

				g_morlockConstantsNeedCommit = false;

				g_morlockDialog->SetDlgItemText( IDC_STATUS, "sending..." );

				buffer[0] = ceCommandSetEEPROMConstants;
				g_morlockConstants.transposeValues();
				memcpy( buffer + 1, &g_morlockConstants, sizeof(g_morlockConstants) );
				g_morlockConstants.transposeValues();

				if ( !DNAUSB::sendData(device, buffer, sizeof(g_morlockConstants) + 1) )
				{
					goto disconnected;
				}

				if ( g_settingsDialog )
				{
					g_settingsDialog->m_progress->SetPos( 75 );
				}

				Arch::sleep( 250 );
				g_morlockDialog->SetDlgItemText( IDC_STATUS, "connected" );

				if ( g_settingsDialog )
				{
					g_settingsDialog->m_progress->SetPos( 100 );
				}

				Arch::sleep( 250 );

				if ( g_settingsDialog )
				{
					g_settingsDialog->m_progress->SetPos( 0 );
				}
			}

			if ( g_morlockConstants.eyeEnabled )
			{
				if ( !DNAUSB::sendCommand(device, ceCommandGetEEPROMConstants) )
				{
					goto disconnected;
				}

				Arch::sleep(10);

				EEPROMConstants constants;
				if ( !DNAUSB::getData(device, (unsigned char*)&constants, 0) )
				{
					goto disconnected;
				}

				constants.transposeValues();
				g_morlockConstants.eyeLevel = constants.eyeLevel;

				g_morlockDialog->renderGraphics();
			}
		}

		continue;

disconnected:
		stringsSet = false;
		DNAUSB::closeDevice( device );
		device = INVALID_DNADEVICE_VALUE;
	}
}

}