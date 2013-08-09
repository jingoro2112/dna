/* Copyright: (c) 2013 by Curt Hartung
 * This work is released under the Creating Commons 3.0 license
 * found at http://creativecommons.org/licenses/by-nc-sa/3.0/legalcode
 * and in the LICENCE.txt file included with this distribution
 */

#include "stdafx.h"
#include "morlock.h"
#include "morlockDlg.h"
#include "afxdialogex.h"

#include "digital_16.h"

#include "../dnausb/dnausb.h"
#include "../firmware/dna/dna_defs.h"
#include "../firmware/morlock/morlock_defs.h"
#include "../util/hash.hpp"

EEPROMConstants g_morlockConstants;
bool CMorlockDlg::m_active = true;

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

//------------------------------------------------------------------------------
struct ToolTipTable
{
	int windowId;
	const char* tip;
};

//------------------------------------------------------------------------------
const ToolTipTable c_tips[]=
{
	{ IDC_SPIN1, "number of milliseconds the fire-control solenoid will remain open" },
	{ IDC_DWELL1, "number of milliseconds the fire-control solenoid will remain open" },
	{ 0, 0 },
};

//------------------------------------------------------------------------------
CMorlockDlg::CMorlockDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CMorlockDlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

//------------------------------------------------------------------------------
void CMorlockDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

//------------------------------------------------------------------------------
BEGIN_MESSAGE_MAP(CMorlockDlg, CDialogEx)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDOK, &CMorlockDlg::OnBnClickedOk)
	ON_BN_CLICKED(IDCANCEL, &CMorlockDlg::OnBnClickedCancel)
	ON_BN_CLICKED(IDC_BUTTON1, &CMorlockDlg::OnBnClickedButton1)
	ON_NOTIFY(UDN_DELTAPOS, IDC_SPIN1, &CMorlockDlg::OnDeltaposSpin1)
	ON_WM_MOUSEMOVE()
	ON_NOTIFY(UDN_DELTAPOS, IDC_DWELL2_HOLDOFF_SPIN, &CMorlockDlg::OnDeltaposDwell2HoldoffSpin)
	ON_NOTIFY(UDN_DELTAPOS, IDC_DWELL2_SPIN, &CMorlockDlg::OnDeltaposDwell2Spin)
	ON_NOTIFY(UDN_DELTAPOS, IDC_MAX_DWELL2_SPIN, &CMorlockDlg::OnDeltaposMaxDwell2Spin)
	ON_BN_CLICKED(IDC_SINGLE_SOLENOID, &CMorlockDlg::OnBnClickedSingleSolenoid)
	ON_BN_CLICKED(IDC_DUAL_SOLENOID, &CMorlockDlg::OnBnClickedDualSolenoid)
	ON_NOTIFY(UDN_DELTAPOS, IDC_ROF_SPIN, &CMorlockDlg::OnDeltaposRofSpin)
	ON_NOTIFY(UDN_DELTAPOS, IDC_SHOTS_IN_BURST_SPIN, &CMorlockDlg::OnDeltaposShotsInBurstSpin)
	ON_NOTIFY(UDN_DELTAPOS, IDC_ENHANCED_TRIGGER_TIMEOUT_SPIN, &CMorlockDlg::OnDeltaposEnhancedTriggerTimeoutSpin)
	ON_CBN_SELCHANGE(IDC_FIRE_MODE, &CMorlockDlg::OnCbnSelchangeFireMode)
	ON_NOTIFY(UDN_DELTAPOS, IDC_RAMP_ENABLE_COUNT_SPIN, &CMorlockDlg::OnDeltaposRampEnableCountSpin)
	ON_CBN_SELCHANGE(IDC_RAMP_TOP_MODE, &CMorlockDlg::OnCbnSelchangeRampTopMode)
	ON_NOTIFY(UDN_DELTAPOS, IDC_RAMP_TIMEOUT_SPIN, &CMorlockDlg::OnDeltaposRampTimeoutSpin)
	ON_BN_CLICKED(IDC_ABS, &CMorlockDlg::OnBnClickedAbs)
	ON_NOTIFY(UDN_DELTAPOS, IDC_ABS_TIMEOUT_SPIN, &CMorlockDlg::OnDeltaposAbsTimeoutSpin)
	ON_NOTIFY(UDN_DELTAPOS, IDC_ABS_ADDITION_SPIN, &CMorlockDlg::OnDeltaposAbsAdditionSpin)
	ON_NOTIFY(UDN_DELTAPOS, IDC_REBOUNCE_SPIN, &CMorlockDlg::OnDeltaposRebounceSpin)
	ON_BN_CLICKED(IDC_LOAD, &CMorlockDlg::OnBnClickedLoad)
	ON_BN_CLICKED(IDC_SAVE, &CMorlockDlg::OnBnClickedSave)
	ON_BN_CLICKED(IDC_APPLY, &CMorlockDlg::OnBnClickedApply)
	ON_NOTIFY(UDN_DELTAPOS, IDC_EYE_HOLDOFF_SPIN, &CMorlockDlg::OnDeltaposEyeHoldoffSpin)
	ON_BN_CLICKED(IDC_EYE_SENSE_REVERSE, &CMorlockDlg::OnBnClickedEyeSenseReverse)
	ON_NOTIFY(UDN_DELTAPOS, IDC_DIMMER_SPIN, &CMorlockDlg::OnDeltaposDimmerSpin)
	ON_NOTIFY(UDN_DELTAPOS, IDC_ACCESORY_RUNTIME_SPIN, &CMorlockDlg::OnDeltaposAccesoryRuntimeSpin)
	ON_NOTIFY(UDN_DELTAPOS, IDC_RAMP_CLIMB_SPIN, &CMorlockDlg::OnDeltaposRampClimbSpin)
	ON_NOTIFY(UDN_DELTAPOS, IDC_DEBOUNCE_SPIN, &CMorlockDlg::OnDeltaposDebounceSpin)
	ON_NOTIFY(UDN_DELTAPOS, IDC_BOLT_HOLDOFF_SPIN, &CMorlockDlg::OnDeltaposBoltHoldoffSpin)
	ON_BN_CLICKED(IDC_SAVE_AS, &CMorlockDlg::OnBnClickedSaveAs)
END_MESSAGE_MAP()

//------------------------------------------------------------------------------
BOOL CMorlockDlg::OnInitDialog()
{
	assert( sizeof(EEPROMConstants) <= DNAUSB_DATA_SIZE );

	CDialogEx::OnInitDialog();

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	m_fireCycle.init( 500, 120, 20, 20 );
	m_eyeCycle.init( 300, 100, 20, 210  );

	m_connected = false;
	m_morlockConstantsDirty = false;
	m_morlockConstantsNeedCommit = false;

	g_morlockConstants.installDefaults();
	populateDialogFromConstants();

	CComboBox *combo = (CComboBox *)GetDlgItem( IDC_FIRE_MODE );
	combo->ResetContent();
	combo->SetItemData( combo->AddString( "1 Semi" ), ceSemi );
	combo->SetItemData( combo->AddString( "2 Autoresponse" ), ceAutoresponse );
	combo->SetItemData( combo->AddString( "3 Burst" ), ceBurst );
	combo->SetItemData( combo->AddString( "4 Full Auto" ), ceFullAuto );
	combo->SetItemData( combo->AddString( "5 Ramp" ), ceRamp );
	combo->SetItemData( combo->AddString( "6 Cocker" ), ceCocker );
	combo->SetItemData( combo->AddString( "7 Sniper" ), ceSniper );
	combo->SetCurSel( 0 );

	combo = (CComboBox *)GetDlgItem( IDC_RAMP_TOP_MODE );
	combo->ResetContent();
	combo->SetItemData( combo->AddString( "1 Semi" ), ceSemi );
	combo->SetItemData( combo->AddString( "2 Autoresponse" ), ceAutoresponse );
	combo->SetItemData( combo->AddString( "3 Burst" ), ceBurst );
	combo->SetItemData( combo->AddString( "4 Full Auto" ), ceFullAuto );
	combo->SetCurSel( 0 );
	
	m_tips.Create( this, TTS_ALWAYSTIP );
	for( int i=0; c_tips[i].windowId ; i++ )
	{
		m_tips.AddTool( GetDlgItem(c_tips[i].windowId), c_tips[i].tip );
	}
	m_tips.Activate( TRUE );

	m_active = true;
	_beginthread( morlockCommThread, 0, this );

	return TRUE;  // return TRUE  unless you set the focus to a control
}

//------------------------------------------------------------------------------
void CMorlockDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
		m_eyeCycle.blit( ::GetDC(GetSafeHwnd()) );
		m_fireCycle.blit( ::GetDC(GetSafeHwnd()) );
	}
}

//------------------------------------------------------------------------------
// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CMorlockDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

//------------------------------------------------------------------------------
void CMorlockDlg::OnBnClickedOk()
{
	
}

//------------------------------------------------------------------------------
void CMorlockDlg::OnBnClickedCancel()
{
	
}

//------------------------------------------------------------------------------
void CMorlockDlg::OnBnClickedButton1()
{
	if ( m_morlockConstantsDirty && m_connected )
	{
		if ( MessageBox("Current changes are unsaved, are you sure you want to exit?", "warning", MB_OKCANCEL) != IDOK )
		{
			return;
		}
	}
	m_active = false;
	CDialogEx::OnCancel();
}

//------------------------------------------------------------------------------
void CMorlockDlg::renderGraphics()
{
	m_eyeCycle.clear();
	m_fireCycle.clear();

	unsigned int width;
	unsigned int height;
	unsigned int i,j;

	m_fireCycle.getBounds( &width, &height );
	// first a cool grid, everythin is cooler with green lines
	for( i=0; i<width; i+= 20 )
	{
		for( j=0; j<height; j++ )
		{
			m_fireCycle.set( i, j, CANVAS_RGB(0,40,0) );
		}
	}
	for( i=0; i<width; i++ )
	{
		for( j=0; j<height; j+=20 )
		{
			m_fireCycle.set( i, j, CANVAS_RGB(0,40,0) );
		}
	}

	float scale = (float)width / ((10.f / (float)g_morlockConstants.ballsPerSecondX10) * 1000);
	

	const int halfBar = 6;

	// now draw the dwell1 bar
	unsigned int barLength = (int)(scale * (float)g_morlockConstants.dwell1);
	for( i=0; i<barLength; i++ )
	{
		m_fireCycle.set( i, height/2 - (halfBar+1), CANVAS_RGBA(255,0,0,255) );
	}
	for( j=height/2 - halfBar; j < height/2 + halfBar; j++ )
	{
		m_fireCycle.set( 0, j, CANVAS_RGBA(255,0,0,255) );
	}

	for( i=0; i<barLength; i++ )
	{
		for( j=height/2 - halfBar; j < height/2 + halfBar; j++ )
		{
			m_fireCycle.set( i, j, CANVAS_RGBA(255,0,0,150) );
		}
	}

	// and label it
	const int lineLength = 6;
	m_fireCycle.drawLine( 2, height/2 - (halfBar + 2), 2 + lineLength, height/2 - (halfBar + 2) - lineLength, CANVAS_RGBA(255,0,0,255) );
	m_fireCycle.drawLine( barLength - 2, height/2 - (halfBar + 2), (barLength - 2) - lineLength, height/2 - (halfBar + 2) - lineLength, CANVAS_RGBA(255,0,0,255) );

	unsigned int textCenter = getTextWidth( "dwell 1", digital_16 ) / 2;

	
	textAt( "dwell 1", m_fireCycle, digital_16, digital_16_bitmap, 10, 10, 0, 2	 55, 255, 255 );


	m_eyeCycle.getBounds( &width, &height );
	// first a cool grid, everythin is cooler with green lines
	for( i=0; i<width; i+= 20 )
	{
		for( j=0; j<height; j++ )
		{
			m_eyeCycle.set( i, j, CANVAS_RGB(0,40,0) );
		}
	}
	for( i=0; i<width; i++ )
	{
		for( j=0; j<height; j+=20 )
		{
			m_eyeCycle.set( i, j, CANVAS_RGB(0,40,0) );
		}
	}

	
	m_eyeCycle.blit( ::GetDC(GetSafeHwnd()) );
	m_fireCycle.blit( ::GetDC(GetSafeHwnd()) );
}

//------------------------------------------------------------------------------
void CMorlockDlg::populateDialogFromConstants()
{
	Cstr buf;

	((CButton *)GetDlgItem( IDC_SINGLE_SOLENOID ))->SetCheck( g_morlockConstants.singleSolenoid );
	((CButton *)GetDlgItem( IDC_DUAL_SOLENOID ))->SetCheck( !g_morlockConstants.singleSolenoid );

	SetDlgItemText( IDC_DWELL2, buf.format("%d", g_morlockConstants.dwell2) );
	SetDlgItemText( IDC_MAX_DWELL2, buf.format("%d", g_morlockConstants.maxDwell2) );
	SetDlgItemText( IDC_DWELL2_HOLDOFF, buf.format("%d", g_morlockConstants.dwell1ToDwell2Holdoff) );
	SetDlgItemText( IDC_DWELL1, buf.format("%d", g_morlockConstants.dwell1) );


	SetDlgItemText( IDC_ROF, buf.format("%.1f", (float)g_morlockConstants.ballsPerSecondX10 / 10) );
	SetDlgItemText( IDC_SHOTS_IN_BURST, buf.format("%d", g_morlockConstants.burstCount) );
	SetDlgItemText( IDC_ENHANCED_TRIGGER_TIMEOUT, buf.format("%d", g_morlockConstants.enhancedTriggerTimeout) );

	CComboBox *combo = (CComboBox *)GetDlgItem( IDC_RAMP_TOP_MODE );
	combo->SetCurSel( g_morlockConstants.rampTopMode - 1 );
	combo = (CComboBox *)GetDlgItem( IDC_FIRE_MODE );
	combo->SetCurSel( g_morlockConstants.fireMode - 1 );
	
	SetDlgItemText( IDC_RAMP_ENABLE_COUNT, buf.format("%d", g_morlockConstants.rampEnableCount) );
	SetDlgItemText( IDC_RAMP_CLIMB, buf.format("%d", g_morlockConstants.rampClimb) );
	SetDlgItemText( IDC_RAMP_TIMEOUT, buf.format("%d", g_morlockConstants.rampTimeout) );

	SetDlgItemText( IDC_ABS_TIMEOUT, buf.format("%d", g_morlockConstants.ABSTimeout) );
	SetDlgItemText( IDC_ABS_ADDITION, buf.format("%d", g_morlockConstants.ABSAddition) );

	SetDlgItemText( IDC_REBOUNCE, buf.format("%d", g_morlockConstants.rebounce) );
	SetDlgItemText( IDC_DEBOUNCE, buf.format("%d", g_morlockConstants.debounce) );

	SetDlgItemText( IDC_ACCESSORY_RUNTIME, buf.format("%d", g_morlockConstants.accessoryRunTime) );

	SetDlgItemText( IDC_DIMMER, buf.format("%d", g_morlockConstants.dimmer) );

	SetDlgItemText( IDC_EYE_HOLDOFF, buf.format("%d", g_morlockConstants.eyeHoldoff) );
	SetDlgItemText( IDC_BOLT_HOLDOFF, buf.format("%d", g_morlockConstants.boltHoldoff) );

	renderGraphics();
}

//------------------------------------------------------------------------------
void CMorlockDlg::save()
{
	Cstr buffer;
	buffer.append( &g_morlockConstants, 4 ); // write 4 dummy bytes
	buffer.append( &g_morlockConstants, sizeof(EEPROMConstants) );
	*(unsigned int*)buffer.c_str() = Hash::hash( buffer.c_str() + 4, sizeof(EEPROMConstants)); // store a CRC
	buffer.bufferToFile( m_currentPath );
}

//------------------------------------------------------------------------------
void CMorlockDlg::morlockCommThread( void* arg )
{
	CMorlockDlg *md = (CMorlockDlg *)arg;
	
	unsigned char buffer[64];
	Cstr temp;
	DNADEVICE device = INVALID_DNADEVICE_VALUE;
	bool stringsSet = false;
	while( m_active )
	{
		if ( device == INVALID_DNADEVICE_VALUE )
		{
			md->m_connected = false;
			if ( !stringsSet )
			{
				stringsSet = true;
				md->SetDlgItemText( IDC_PRODUCT, "n/a" );
				md->SetDlgItemText( IDC_STATUS, "DISCONNECTED" );
			}
			
			if ( !(device = DNAUSB::openDevice(0x16C0, 0x05DF, "p@northarc.com", (char  *)buffer)) )
			{
				Sleep( 500 );
				continue;
			}

			unsigned char id;
			if ( !DNAUSB::getProductId(device, &id) )
			{
				goto disconnected;
			}

			unsigned char productId;
			DNAUSB::getProductId( device, &productId );

			if ( productId == BOOTLOADER_DNA_AT84_v1_00 )
			{
				md->SetDlgItemText( IDC_PRODUCT, "DNA BOOTLOADER Hardware v1.0" );
			}
			else if ( productId == DNA_AT84_v1_00 )
			{
				md->SetDlgItemText( IDC_PRODUCT, "DNA Hardware v1.0" );
			}
			
			md->SetDlgItemText( IDC_STATUS, "connected" );

			buffer[0] = ceCommandGetEEPROMConstants;
			if ( !DNAUSB::sendData(device, buffer) )
			{
				goto disconnected;
			}

			if ( !DNAUSB::getData(device, buffer) )
			{
				goto disconnected;
			}

			memcpy( &g_morlockConstants, buffer, sizeof(g_morlockConstants) );
			md->populateDialogFromConstants();
			
			md->SetDlgItemText( IDC_STATUS, "connected" );
		}

		md->m_connected = true;

		if ( md->m_morlockConstantsNeedCommit )
		{
			md->SetDlgItemText( IDC_STATUS, "sending..." );

			buffer[0] = ceCommandSetEEPROMConstants;
			if ( !DNAUSB::sendData(device, buffer) )
			{
				goto disconnected;
			}

			memcpy( buffer, &g_morlockConstants, sizeof(g_morlockConstants) );
			if ( !DNAUSB::sendData(device, buffer) )
			{
				goto disconnected;
			}

			md->SetDlgItemText( IDC_STATUS, "idle" );

			md->m_morlockConstantsNeedCommit = false;
		}

		Sleep( 500 ); // basically polling for changes
		continue;
		
disconnected:
		DNAUSB::closeDevice( device );
		device = INVALID_DNADEVICE_VALUE;
	}
}

//------------------------------------------------------------------------------
unsigned int CMorlockDlg::getTextWidth( const char* string, const MFont* font )
{
	unsigned int length = 0;
	for( int i=0; string[i]; i++ )
	{
		if ( string[i] >=32 && string[i] < 127 )
		{
			char c = string[i] - 32;
			length += font[c].pre + font[c].post + font[c].w;

		}
	}

	return length;
}

//------------------------------------------------------------------------------
void CMorlockDlg::textAt( const char* text, Canvas& canvas, const MFont* font, const unsigned char* bitmap,
						  int x, int y, float r, float g, float b, float a )
{
	if ( !font || !text )
	{
		return;
	}

	char c;

	for(int i=0; text[i]; i++)
	{
		c = text[i] - 32;

		x += font[c].pre;

		int glyphX = font[c].x;
		int glyphXEnd = glyphX + font[c].w;
		for( ; glyphX < glyphXEnd; glyphX++, x++ )
		{
			int glyphY = font[c].y;
			int glyphYEnd = glyphY + font[c].h;
			for( ; glyphY < glyphYEnd; glyphY++, y++ )
			{
				int pixel = (int)bitmap[glyphY*128 + glyphX];

				if ( pixel )
				{
					canvas.set( x, y, CANVAS_RGBA( (unsigned int)((r / 255.f) * (float)pixel),
												   (unsigned int)((g / 255.f) * (float)pixel),
												   (unsigned int)((b / 255.f) * (float)pixel),
												   (unsigned int)((a / 255.f) * 255.f)));
				}
			}
			y -= font[c].h;
		}

		x += font[c].post;
	}
}

//------------------------------------------------------------------------------
BOOL CMorlockDlg::PreTranslateMessage(MSG* pMsg)
{
	m_tips.RelayEvent(pMsg);  
	return CDialog::PreTranslateMessage(pMsg);
}

//------------------------------------------------------------------------------
void CMorlockDlg::OnMouseMove(UINT nFlags, CPoint point)
{
	TOOLINFO t;
	if ( GetDlgItem(IDC_PRODUCT)->OnToolHitTest( point, &t ) != -1 )
	{
		printf("yo");
	}
	CDialogEx::OnMouseMove(nFlags, point);
}

//------------------------------------------------------------------------------
void CMorlockDlg::OnDeltaposSpin1(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMUPDOWN pNMUpDown = reinterpret_cast<LPNMUPDOWN>(pNMHDR);
	m_morlockConstantsDirty = true;
	g_morlockConstants.dwell1 -= pNMUpDown->iDelta;
	populateDialogFromConstants();

	*pResult = 0;
}

//------------------------------------------------------------------------------
void CMorlockDlg::OnDeltaposDwell2HoldoffSpin(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMUPDOWN pNMUpDown = reinterpret_cast<LPNMUPDOWN>(pNMHDR);
	*pResult = 0;

	m_morlockConstantsDirty = true;
	g_morlockConstants.dwell1ToDwell2Holdoff -= pNMUpDown->iDelta;
	populateDialogFromConstants();
}

//------------------------------------------------------------------------------
void CMorlockDlg::OnDeltaposDwell2Spin(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMUPDOWN pNMUpDown = reinterpret_cast<LPNMUPDOWN>(pNMHDR);
	*pResult = 0;

	m_morlockConstantsDirty = true;
	g_morlockConstants.dwell2 -= pNMUpDown->iDelta;
	populateDialogFromConstants();
}

//------------------------------------------------------------------------------
void CMorlockDlg::OnDeltaposMaxDwell2Spin(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMUPDOWN pNMUpDown = reinterpret_cast<LPNMUPDOWN>(pNMHDR);
	*pResult = 0;

	m_morlockConstantsDirty = true;
	g_morlockConstants.maxDwell2 -= pNMUpDown->iDelta;
	populateDialogFromConstants();
}

//------------------------------------------------------------------------------
void CMorlockDlg::OnBnClickedSingleSolenoid()
{
	m_morlockConstantsDirty = true;
	g_morlockConstants.singleSolenoid = 1;
	populateDialogFromConstants();
}

//------------------------------------------------------------------------------
void CMorlockDlg::OnBnClickedDualSolenoid()
{
	m_morlockConstantsDirty = true;
	g_morlockConstants.singleSolenoid = 0;
	populateDialogFromConstants();
}

//------------------------------------------------------------------------------
void CMorlockDlg::OnDeltaposRofSpin(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMUPDOWN pNMUpDown = reinterpret_cast<LPNMUPDOWN>(pNMHDR);
	*pResult = 0;

	m_morlockConstantsDirty = true;
	g_morlockConstants.ballsPerSecondX10 -= (pNMUpDown->iDelta * 5);
	populateDialogFromConstants();
}

//------------------------------------------------------------------------------
void CMorlockDlg::OnDeltaposShotsInBurstSpin(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMUPDOWN pNMUpDown = reinterpret_cast<LPNMUPDOWN>(pNMHDR);
	*pResult = 0;

	m_morlockConstantsDirty = true;
	g_morlockConstants.burstCount -= pNMUpDown->iDelta;
	populateDialogFromConstants();
}

//------------------------------------------------------------------------------
void CMorlockDlg::OnDeltaposEnhancedTriggerTimeoutSpin(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMUPDOWN pNMUpDown = reinterpret_cast<LPNMUPDOWN>(pNMHDR);
	*pResult = 0;

	m_morlockConstantsDirty = true;
	g_morlockConstants.enhancedTriggerTimeout -= pNMUpDown->iDelta;
	populateDialogFromConstants();
}

//------------------------------------------------------------------------------
void CMorlockDlg::OnCbnSelchangeFireMode()
{
	m_morlockConstantsDirty = true;
	CComboBox *combo = (CComboBox *)GetDlgItem( IDC_FIRE_MODE );
	g_morlockConstants.fireMode = (unsigned char)combo->GetItemData( combo->GetCurSel() );
	populateDialogFromConstants();
}

//------------------------------------------------------------------------------
void CMorlockDlg::OnDeltaposRampEnableCountSpin(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMUPDOWN pNMUpDown = reinterpret_cast<LPNMUPDOWN>(pNMHDR);
	*pResult = 0;

	m_morlockConstantsDirty = true;
	g_morlockConstants.rampEnableCount -= pNMUpDown->iDelta;
	populateDialogFromConstants();
}

//------------------------------------------------------------------------------
void CMorlockDlg::OnCbnSelchangeRampTopMode()
{
	m_morlockConstantsDirty = true;
	CComboBox *combo = (CComboBox *)GetDlgItem( IDC_RAMP_TOP_MODE );
	g_morlockConstants.rampTopMode = (unsigned char)combo->GetItemData( combo->GetCurSel() );
	populateDialogFromConstants();
}

//------------------------------------------------------------------------------
void CMorlockDlg::OnDeltaposRampTimeoutSpin(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMUPDOWN pNMUpDown = reinterpret_cast<LPNMUPDOWN>(pNMHDR);
	*pResult = 0;

	m_morlockConstantsDirty = true;
	g_morlockConstants.rampTimeout -= pNMUpDown->iDelta;
	populateDialogFromConstants();
}

//------------------------------------------------------------------------------
void CMorlockDlg::OnBnClickedAbs()
{
	m_morlockConstantsDirty = true;
	if ( ((CButton *)GetDlgItem(IDC_ABS))->GetState() & 0x3 )
	{
		g_morlockConstants.ABSTimeout = DEFAULT_ANTI_BOLT_STICK_TIMEOUT;
	}
	else
	{
		g_morlockConstants.ABSTimeout = 0;
	}
	populateDialogFromConstants();
}

//------------------------------------------------------------------------------
void CMorlockDlg::OnDeltaposAbsTimeoutSpin(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMUPDOWN pNMUpDown = reinterpret_cast<LPNMUPDOWN>(pNMHDR);
	*pResult = 0;

	m_morlockConstantsDirty = true;
	g_morlockConstants.ABSTimeout -= pNMUpDown->iDelta;
	populateDialogFromConstants();
}

//------------------------------------------------------------------------------
void CMorlockDlg::OnDeltaposAbsAdditionSpin(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMUPDOWN pNMUpDown = reinterpret_cast<LPNMUPDOWN>(pNMHDR);
	*pResult = 0;

	m_morlockConstantsDirty = true;
	g_morlockConstants.ABSAddition -= pNMUpDown->iDelta;
	populateDialogFromConstants();
}

//------------------------------------------------------------------------------
void CMorlockDlg::OnDeltaposRebounceSpin(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMUPDOWN pNMUpDown = reinterpret_cast<LPNMUPDOWN>(pNMHDR);
	*pResult = 0;

	m_morlockConstantsDirty = true;
	g_morlockConstants.rebounce -= pNMUpDown->iDelta;
	populateDialogFromConstants();
}

//------------------------------------------------------------------------------
void CMorlockDlg::OnBnClickedLoad()
{
	CFileDialog dlgFile( TRUE,
						 _T(".mlk"),
						 NULL,
						 OFN_HIDEREADONLY | OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_READONLY,
						 _T("Morlock Profiles (*.mlk)|*.mlk||"));

	if ( dlgFile.DoModal() == IDOK)
	{
		POSITION startPos = dlgFile.GetStartPosition();
		while( startPos )
		{
			m_currentPath = dlgFile.GetNextPathName( startPos );

			Cstr buffer;
			if ( buffer.fileToBuffer(m_currentPath) )
			{
				if ( buffer.size() != (sizeof(EEPROMConstants) + 4) )
				{
					MessageBox( "Not a Morlock profile", "error", MB_OK | MB_ICONSTOP);
					m_currentPath = "";
				}
				else 
				{
					if ( Hash::hash( buffer.c_str() + 4, sizeof(EEPROMConstants)) != *(unsigned int*)buffer.c_str() )
					{
						MessageBox( "Morlock profile is corrupt", "error", MB_OK | MB_ICONSTOP);
						m_currentPath = "";
					}
					else
					{
						memcpy( &g_morlockConstants, buffer.c_str() + 4, sizeof(EEPROMConstants) );
						populateDialogFromConstants();
					}
				}
			}
			else
			{
				m_currentPath = "";
			}
		}
	}
}

//------------------------------------------------------------------------------
void CMorlockDlg::OnBnClickedSave()
{
	if ( !m_currentPath.length() )
	{
		char buf[ MAX_PATH + 2 ] = "";
		CFileDialog dlgFile( FALSE,
							 _T(".mlk"),
							 buf,
							 OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
							 _T("Morlock Profiles (*.mlk)|*.mlk||"));

		if ( dlgFile.DoModal() != IDOK )
		{
			return;
		}

		m_currentPath = dlgFile.GetPathName();
	}

	if ( m_currentPath.size() )
	{
		save();
	}
}

//------------------------------------------------------------------------------
void CMorlockDlg::OnBnClickedApply()
{
	m_morlockConstantsNeedCommit = true;
}

//------------------------------------------------------------------------------
void CMorlockDlg::OnDeltaposEyeHoldoffSpin(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMUPDOWN pNMUpDown = reinterpret_cast<LPNMUPDOWN>(pNMHDR);
	*pResult = 0;

	m_morlockConstantsDirty = true;
	g_morlockConstants.eyeHoldoff -= pNMUpDown->iDelta;
	populateDialogFromConstants();
}

//------------------------------------------------------------------------------
void CMorlockDlg::OnBnClickedEyeSenseReverse()
{
	m_morlockConstantsDirty = true;
	if ( ((CButton *)GetDlgItem(IDC_ABS))->GetState() & 0x3 )
	{
		g_morlockConstants.eyeHighBlocked = DEFAULT_ANTI_BOLT_STICK_TIMEOUT;
	}
	else
	{
		g_morlockConstants.eyeHighBlocked = 0;
	}
}

//------------------------------------------------------------------------------
void CMorlockDlg::OnDeltaposDimmerSpin(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMUPDOWN pNMUpDown = reinterpret_cast<LPNMUPDOWN>(pNMHDR);
	*pResult = 0;
	m_morlockConstantsDirty = true;
	g_morlockConstants.dimmer -= pNMUpDown->iDelta;
	populateDialogFromConstants();
}

//------------------------------------------------------------------------------
void CMorlockDlg::OnDeltaposAccesoryRuntimeSpin(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMUPDOWN pNMUpDown = reinterpret_cast<LPNMUPDOWN>(pNMHDR);
	*pResult = 0;
	m_morlockConstantsDirty = true;
	g_morlockConstants.accessoryRunTime -= pNMUpDown->iDelta;
	populateDialogFromConstants();
}

//------------------------------------------------------------------------------
void CMorlockDlg::OnDeltaposRampClimbSpin(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMUPDOWN pNMUpDown = reinterpret_cast<LPNMUPDOWN>(pNMHDR);
	*pResult = 0;
	m_morlockConstantsDirty = true;
	g_morlockConstants.rampClimb -= pNMUpDown->iDelta;
	populateDialogFromConstants();
}

//------------------------------------------------------------------------------
void CMorlockDlg::OnDeltaposDebounceSpin(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMUPDOWN pNMUpDown = reinterpret_cast<LPNMUPDOWN>(pNMHDR);
	*pResult = 0;
	m_morlockConstantsDirty = true;
	g_morlockConstants.debounce -= pNMUpDown->iDelta;
	populateDialogFromConstants();
}

//------------------------------------------------------------------------------
void CMorlockDlg::OnDeltaposBoltHoldoffSpin(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMUPDOWN pNMUpDown = reinterpret_cast<LPNMUPDOWN>(pNMHDR);
	*pResult = 0;
	m_morlockConstantsDirty = true;
	g_morlockConstants.boltHoldoff -= pNMUpDown->iDelta;
	populateDialogFromConstants();
}

//------------------------------------------------------------------------------
void CMorlockDlg::OnBnClickedSaveAs()
{
	char buf[ MAX_PATH + 2 ] = "";
	CFileDialog dlgFile( FALSE,
						 _T(".mlk"),
						 buf,
						 OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
						 _T("Morlock Profiles (*.mlk)|*.mlk||"));

	if ( dlgFile.DoModal() != IDOK )
	{
		return;
	}

	m_currentPath = dlgFile.GetPathName();

	if ( m_currentPath.size() )
	{
		save();
	}
}
