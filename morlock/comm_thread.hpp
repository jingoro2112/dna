/* Copyright: (c) 2013 by Curt Hartung avr@northarc.com
 * This work is released under the Creating Commons 3.0 license
 * found at http://creativecommons.org/licenses/by-nc-sa/3.0/legalcode
 * and in the LICENCE.txt file included with this distribution
 */

#pragma once

#include "stdafx.h"
#include <windows.h>

#include "../util/str.hpp"

class CMorlockDlg;
class CSettings;

namespace USBComm
{
extern CMorlockDlg* g_morlockDialog;
extern CSettings* g_settingsDialog;
extern bool g_active;
extern bool g_morlockConstantsNeedCommit;
extern bool g_connected;
extern Cstr g_codeImage;
extern bool g_reflash;



void morlockCommThread( void* arg );
	
}