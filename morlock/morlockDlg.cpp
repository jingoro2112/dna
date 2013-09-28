/* Copyright: (c) 2013 by Curt Hartung avr@northarc.com
 * This work is released under the Creating Commons 3.0 license
 * found at http://creativecommons.org/licenses/by-nc-sa/3.0/legalcode
 * and in the LICENCE.txt file included with this distribution
 */

#include "stdafx.h"
#include "morlock.h"
#include "morlockDlg.h"
#include "afxdialogex.h"

#include "digital_16.h"
#include "Settings.h"
#include "comm_thread.hpp"

#include "../firmware/dna/dna_defs.h"
#include "../firmware/morlock/morlock_defs.h"
#include "../util/hash.hpp"
#include "../util/json_parser.hpp"
#include "../util/simple_log.hpp"
#include "../util/read_hex.hpp"


SimpleLog Log;

EEPROMConstants g_morlockConstants;
CMorlockDlg *CMorlockDlg::m_singleton = 0;

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
	{ IDC_SINGLE_SOLENOID, "Select single-solenoid cycle, enabling second power channel to run an accessory" },
	{ IDC_DUAL_SOLENOID, "Select dual-solenoid cycle" },
	{ IDC_DWELL2_SPIN, "number of millisecond time the second solenoid will hold the bolt open" },
	{ IDC_MAX_DWELL2_SPIN, "When the eye is used, this is the longest time the bolt will be held open before giving up and closing it" },
	{ IDC_DWELL2_HOLDOFF_SPIN, "In dual-solenoid mode, this is how long the bolt solenoid waits to begin its cycle" },
	{ IDC_ROF_SPIN, "Cap on the rate of fire" },
	{ IDC_SHOTS_IN_BURST_SPIN, "How many shots to fire in burst mode" },
	{ IDC_ENHANCED_TRIGGER_TIMEOUT_SPIN, "How long after the trigger is pulled will its release no longer trigger an enhanced shot, such as autoresponse" },
	{ IDC_RAMP_TOP_MODE, "When ramping fully engages, what mode of fire will be used" },
	{ IDC_FIRE_MODE, "What mode of fire will be used" },
	{ IDC_RAMP_ENABLE_COUNT_SPIN, "How many shots must be fire in semi before ramping kicks in" },
	{ IDC_RAMP_CLIMB_SPIN, "Number of shots added per trigger pull in ramping mode, cumulative, this adjusts ramping speed" },
	{ IDC_RAMP_TIMEOUT_SPIN, "Time before ramping reverts to normal semi and must be re-engaged" },
	{ IDC_ABS_TIMEOUT_SPIN, "How long the marker is idle before the Anti Bolt-Stick is added to dwell 1 on the next shot" },
	{ IDC_ABS_ADDITION_SPIN, "How many milliseconds are added to dwell 1 to mitigate first-shot dropoff" },
	{ IDC_DEBOUNCE_SPIN, "Debounce constant to filter switch noise, lowering this can cause sporatic shots" },
	{ IDC_REBOUNCE_SPIN, "Constant for filtering transistion in the switch, lowering this could cause erratic switch behavior in enhanced trigger modes" },
	{ IDC_ACCESORY_RUNTIME_SPIN, "In single-solenoid mode, how long the second channel will be turned on to run an accessory" },
	{ IDC_DIMMER_SPIN, "dimmer for the on-board LED" },
	{ IDC_EYE_HOLDOFF_SPIN, "If the eye is being used, and paint is not in the breech, when it is detected, wait this amount of time to be sure it has seated" },
	{ IDC_BOLT_HOLDOFF_SPIN, "when using the eye, this is how long to way after the bolt cycles to begin looking at it again to detect the next paintball, this prevents accidentally detecting the bolt as a paintball" },
	{ IDC_EYE_ENABLED, "Enable the eye" },
	{ IDC_CHECK3, "When checked, the morlock cannot be trigger programmed, only by tethering" },
		
	{ 0, 0 },
};

//------------------------------------------------------------------------------
enum RampPresets
{
	ceRampTurbo,
	ceRampDoubleTap,
	ceRampOMG,
	ceRampMillenium,
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
	ON_BN_CLICKED(IDC_EYE_ENABLED, &CMorlockDlg::OnBnClickedEyeEnabled)
	ON_BN_CLICKED(IDC_CHECK3, &CMorlockDlg::OnBnClickedCheck3)
	ON_CBN_SELCHANGE(IDC_RAMP_PRESETS, &CMorlockDlg::OnCbnSelchangeRampPresets)
	ON_BN_CLICKED(IDC_REFLASH, &CMorlockDlg::OnBnClickedReflash)
	ON_NOTIFY(UDN_DELTAPOS, IDC_EYE_DETECT_SPIN, &CMorlockDlg::OnDeltaposEyeDetectSpin)
	ON_NOTIFY(UDN_DELTAPOS, IDC_SHORT_CYCLE_PREVENT_SPIN, &CMorlockDlg::OnDeltaposShortCyclePreventSpin)
	ON_NOTIFY(UDN_DELTAPOS, IDC_EYE_DETECT_HOLDOFF_SPIN2, &CMorlockDlg::OnDeltaposEyeDetectHoldoffSpin2)
	ON_BN_CLICKED(IDC_AUTO_APPLY, &CMorlockDlg::OnBnClickedAutoApply)
	ON_BN_CLICKED(IDC_EYE_STRONG, &CMorlockDlg::OnBnClickedEyeStrong)
	ON_BN_CLICKED(IDC_EYE_WEAK, &CMorlockDlg::OnBnClickedEyeWeak)
END_MESSAGE_MAP()

//------------------------------------------------------------------------------
BOOL CMorlockDlg::OnInitDialog()
{
	m_singleton = this;
	
	CDialogEx::OnInitDialog();

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	m_gfx.init( 500, 120, 20, 20 );

	m_morlockConstantsDirty = false;
	USBComm::g_morlockConstantsNeedCommit = false;

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

	combo = (CComboBox *)GetDlgItem( IDC_RAMP_PRESETS );
	combo->ResetContent();
	combo->SetItemData( combo->AddString( "None" ), -1 );
	combo->SetItemData( combo->AddString( "Turbo" ), ceRampTurbo );
	combo->SetItemData( combo->AddString( "Double Tap" ), ceRampDoubleTap );
	combo->SetItemData( combo->AddString( "OMG" ), ceRampOMG );
	combo->SetItemData( combo->AddString( "Millenium" ), ceRampMillenium );
	combo->SetCurSel( 0 );

	// why would you NOT want this on?
	((CButton *)GetDlgItem( IDC_AUTO_APPLY ))->SetCheck( true );

	m_tips.Create( this, TTS_ALWAYSTIP );
	for( int i=0; c_tips[i].windowId ; i++ )
	{
		m_tips.AddTool( GetDlgItem(c_tips[i].windowId), c_tips[i].tip );
	}
	m_tips.Activate( TRUE );

	USBComm::g_morlockDialog = this;
	_beginthread( USBComm::morlockCommThread, 0, 0 );

	return TRUE;
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
		m_gfx.blit( ::GetDC(GetSafeHwnd()) );
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
	if ( USBComm::g_connected && (m_morlockConstantsDirty || USBComm::g_morlockConstantsNeedCommit) )
	{
		if ( MessageBox("Current changes are unsaved, are you sure you want to exit?", "warning", MB_OKCANCEL) != IDOK )
		{
			return;
		}
	}
	
	USBComm::g_active = false;
	CDialogEx::OnCancel();
}

const int lineLength = 6;
const int halfBar = 6;

//------------------------------------------------------------------------------
void CMorlockDlg::drawInvertedBar( Canvas& canvas, unsigned int originX, unsigned int originY, unsigned int length, unsigned int r, unsigned int g, unsigned int b, const char* label )
{
	unsigned int width;
	unsigned int height;
	unsigned int i,j;
	canvas.getBounds( &width, &height );

	for( i=originX; i<(length + originX); i++ )
	{
		m_gfx.set( i, originY + (halfBar+1), CANVAS_RGBA(r, g, b, 255) );
	}
	for( j=originY - halfBar; j < originY + halfBar; j++ )
	{
		m_gfx.set( originX, j, CANVAS_RGBA(r, g, b, 255) );
	}

	for( i=originX; i<(length + originX); i++ )
	{
		for( j=originY - halfBar; j < originY + halfBar; j++ )
		{
			m_gfx.set( i, j, CANVAS_RGBA(r, g, b, 150) );
		}
	}

	if ( label )
	{
		unsigned int textLen = textLength( label, digital_16 );
		textAt( label, m_gfx, digital_16, digital_16_bitmap, (originX + (length / 2)) - (textLen / 2), originY + 10, (float)r, (float)g, (float)b, 255 );

		m_gfx.drawLine( originX,
							  originY + halfBar,
							  originX + lineLength,
							  originY + halfBar + lineLength,
							  CANVAS_RGBA(r,g,b,200) );

		m_gfx.drawLine( originX + length,
							  originY + halfBar,
							  (originX + length) - lineLength,
							  originY + halfBar + lineLength,
							  CANVAS_RGBA(r,g,b,200) );

		m_gfx.drawLine( originX + lineLength,
							  originY + halfBar + lineLength,
							  originX + (length / 2) - ((textLen / 2) + 3),
							  originY + halfBar + lineLength,
							  CANVAS_RGBA(r,g,b,100) );

		m_gfx.drawLine( originX + (length / 2) + ((textLen / 2) + 3),
							  originY + halfBar + lineLength,
							  (originX + length) - lineLength,
							  originY + halfBar + lineLength,
							  CANVAS_RGBA(r,g,b,100) );
	}
}

//------------------------------------------------------------------------------
void CMorlockDlg::drawBar( Canvas& canvas,
						   unsigned int originX, unsigned int originY,
						   unsigned int length, unsigned int r, unsigned int g, unsigned int b, const char* label )
{
	unsigned int width;
	unsigned int height;
	unsigned int i,j;
	canvas.getBounds( &width, &height );

	for( i=originX; i<(length + originX); i++ )
	{
		m_gfx.set( i, originY - (halfBar+1), CANVAS_RGBA(r, g, b, 255) );
	}
	for( j=originY - halfBar; j < originY + halfBar; j++ )
	{
		m_gfx.set( originX, j, CANVAS_RGBA(r, g, b, 255) );
	}

	for( i=originX; i<(length + originX); i++ )
	{
		for( j=originY - halfBar; j < originY + halfBar; j++ )
		{
			m_gfx.set( i, j, CANVAS_RGBA(r, g, b, 150) );
		}
	}

	if ( label )
	{
		unsigned int textLen = textLength( label, digital_16 );
		textAt( label, m_gfx, digital_16, digital_16_bitmap, (originX + (length / 2)) - (textLen / 2), originY - 22, (float)r, (float)g, (float)b, 255 );

		m_gfx.drawLine( originX,
							  originY - halfBar,
							  originX + lineLength,
							  (originY - halfBar) - lineLength,
							  CANVAS_RGBA(r,g,b,200) );

		m_gfx.drawLine( originX + length,
							  originY - halfBar,
							  (originX + length) - lineLength,
							  (originY - halfBar) - lineLength,
							  CANVAS_RGBA(r,g,b,200) );

		m_gfx.drawLine( originX + lineLength,
							  (originY - halfBar) - lineLength,
							  originX + (length / 2) - ((textLen / 2) + 3),
							  (originY - halfBar) - lineLength,
							  CANVAS_RGBA(r,g,b,100) );

		m_gfx.drawLine( originX + (length / 2) + ((textLen / 2) + 3),
							  (originY - halfBar) - lineLength,
							  (originX + length) - lineLength,
							  (originY - halfBar) - lineLength,
							  CANVAS_RGBA(r,g,b,100) );
	}
}

//------------------------------------------------------------------------------
void CMorlockDlg::renderGraphics()
{
	m_gfx.clear();

	unsigned int width;
	unsigned int height;
	unsigned int widthCoord;
	unsigned int heightCoord;
	unsigned int i,j;

	m_gfx.getBounds( &width, &height );
	widthCoord = width - 1;
	heightCoord = height - 1;
	// first a cool grid, everything is cooler with green lines
	for( i=0; i<width; i+= 20 )
	{
		for( j=0; j<height; j++ )
		{
			m_gfx.set( i, j, CANVAS_RGB(0,40,0) );
		}
	}
	for( i=0; i<width; i++ )
	{
		for( j=0; j<height; j+=20 )
		{
			m_gfx.set( i, j, CANVAS_RGB(0,40,0) );
		}
	}

	const int halfBar = 6;
	const int lineLength = 6;
	unsigned int origin = 0;
	unsigned int length;
	float scale = (float)width / ((10.f / (float)g_morlockConstants.ballsPerSecondX10) * 1000);
	Cstr buf;


	buf.format( "Total Fire Cycle %.1f", (10.f / (float)g_morlockConstants.ballsPerSecondX10) * 1000);
	unsigned int textLen = textLength( buf, digital_16 );
	textAt( buf, m_gfx, digital_16, digital_16_bitmap, (width / 2) - (textLen / 2), 0, 0, 255, 0, 255 );

	m_gfx.drawLine( 0, 0, lineLength, lineLength, CANVAS_RGBA(0,255,0,200) );
	
	m_gfx.drawLine( widthCoord,
						  0,
						  widthCoord - lineLength,
						  lineLength,
						  CANVAS_RGBA(0,255,70,200) );
	
	m_gfx.drawLine( lineLength,
						  lineLength,
						  (width / 2) - ((textLen / 2) + 3),
						  lineLength,
						  CANVAS_RGBA(0,255,70,100) );
	
	m_gfx.drawLine( widthCoord - lineLength,
						  lineLength,
						  (width / 2) + ((textLen / 2) + 3),
						  lineLength,
						  CANVAS_RGBA(0,255,70,100) );

	if ( g_morlockConstants.singleSolenoid )
	{
		// using the eye? draw the eye cycle
		if ( g_morlockConstants.eyeEnabled )
		{
			length = (unsigned int)(scale * (float)g_morlockConstants.eyeHoldoff);
			drawBar( m_gfx, origin, height/2, length, 0, 180, 255, 0 );

			m_gfx.drawLine( length / 2, height/2 - halfBar, length / 2 + (lineLength*2), (height/2 - halfBar) - (lineLength*2), CANVAS_RGB(0,180,255) );
			buf.format( "%d", g_morlockConstants.eyeHoldoff );

			textAt( buf, m_gfx, digital_16, digital_16_bitmap, (length / 2), 24, 0, 180, 255, 255);
			
			origin += length;
		}

		length = (unsigned int)(scale * (float)g_morlockConstants.dwell1);
		drawBar( m_gfx, origin, height/2, length, 255, 0, 0, buf.format( "%d", g_morlockConstants.dwell1 ) );

		origin += length;

		// using the eye? draw the eye cycle
		if ( g_morlockConstants.eyeEnabled )
		{
			length = (unsigned int)(scale * (float)g_morlockConstants.boltHoldoff);

			drawBar( m_gfx, origin, height/2, length, 200, 200, 0, buf.format("%d", g_morlockConstants.boltHoldoff) );
		}
	}
	else
	{
		length = (unsigned int)(scale * (float)g_morlockConstants.dwell1);
		drawBar( m_gfx, origin, height/2 - halfBar, length, 255, 0, 0, buf.format( "%d", g_morlockConstants.dwell1 ) );
		length = (unsigned int)(scale * (float)g_morlockConstants.dwell2Holdoff);
		drawInvertedBar( m_gfx, origin, height/2 + halfBar, length, 0, 200, 250, buf.format( "%d", g_morlockConstants.dwell2Holdoff ) );

		origin += length;


		// using the eye? draw the eye cycle
		if ( g_morlockConstants.eyeEnabled )
		{
			length = (unsigned int)(scale * (float)g_morlockConstants.eyeHoldoff);
			drawBar( m_gfx, origin, height/2, length, 0, 180, 255, 0 );

			m_gfx.drawLine( origin + length/2,
							height/2 - halfBar,
							origin + (length/2) + (lineLength*2),
							(height/2 - halfBar) - (lineLength*2),
							CANVAS_RGB(0,180,255) );

			textAt( buf.format("Holdoff %d", g_morlockConstants.eyeHoldoff), m_gfx, digital_16, digital_16_bitmap, origin + (length / 2), 24, 0, 180, 255, 255);

			origin += length;
		}

		length = (unsigned int)(scale * (float)g_morlockConstants.dwell2);
		drawBar( m_gfx, origin, height/2, length, 160, 32, 240, buf.format( "%d", g_morlockConstants.dwell2 ) );
	}

	if ( g_morlockConstants.eyeEnabled )
	{
		if ( ((g_morlockConstants.eyeLevel > g_morlockConstants.eyeDetectLevel) && g_morlockConstants.eyeHighBlocked)
			 || ((g_morlockConstants.eyeLevel < g_morlockConstants.eyeDetectLevel) && !g_morlockConstants.eyeHighBlocked) )
		{
			drawBar( m_gfx, 0, height - (halfBar), (width * g_morlockConstants.eyeLevel) / 0xFF, 0xFF, 0x45, 0x00, buf.format("%d", g_morlockConstants.eyeLevel) );
		}
		else
		{
			drawBar( m_gfx, 0, height - (halfBar), (width * g_morlockConstants.eyeLevel) / 0xFF, 0x18, 0x74, 0xCD, buf.format("%d", g_morlockConstants.eyeLevel) );
		}
			
		m_gfx.drawLine( (width * g_morlockConstants.eyeDetectLevel) / 0xFF,
						height - (halfBar * 3),
						(width * g_morlockConstants.eyeDetectLevel) / 0xFF,
						height,
						CANVAS_RGBA(0xFF,0,0,200) );
		m_gfx.drawLine( (width * g_morlockConstants.eyeDetectLevel) / 0xFF + 1,
						height - (halfBar * 3),
						(width * g_morlockConstants.eyeDetectLevel) / 0xFF + 1,
						height,
						CANVAS_RGBA(0xFF,0,0,200) );
	}

	
	m_gfx.blit( ::GetDC(GetSafeHwnd()) );
}

//------------------------------------------------------------------------------
void CMorlockDlg::populateDialogFromConstants()
{
	if ( !USBComm::g_connected )
	{
		return;
	}
	
	Cstr buf;

	((CButton *)GetDlgItem( IDC_SINGLE_SOLENOID ))->SetCheck( g_morlockConstants.singleSolenoid );
	((CButton *)GetDlgItem( IDC_DUAL_SOLENOID ))->SetCheck( !g_morlockConstants.singleSolenoid );

	((CButton *)GetDlgItem( IDC_EYE_STRONG ))->SetCheck( g_morlockConstants.eyeStrong );
	((CButton *)GetDlgItem( IDC_EYE_WEAK ))->SetCheck( !g_morlockConstants.eyeStrong );

	SetDlgItemText( IDC_DWELL2, buf.format("%d", g_morlockConstants.dwell2) );
	SetDlgItemText( IDC_MAX_DWELL2, buf.format("%d", g_morlockConstants.maxDwell2) );
	SetDlgItemText( IDC_DWELL2_HOLDOFF, buf.format("%d", g_morlockConstants.dwell2Holdoff) );
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

	SetDlgItemText( IDC_ABS_TIMEOUT, buf.format("%d", g_morlockConstants.ABSTimeout/1000) );
	SetDlgItemText( IDC_ABS_ADDITION, buf.format("%d", g_morlockConstants.ABSAddition) );
	((CButton *)GetDlgItem( IDC_ABS ))->SetCheck( g_morlockConstants.ABSTimeout );

	SetDlgItemText( IDC_REBOUNCE, buf.format("%d", g_morlockConstants.rebounce) );
	SetDlgItemText( IDC_DEBOUNCE, buf.format("%d", g_morlockConstants.debounce) );

	SetDlgItemText( IDC_ACCESSORY_RUNTIME, buf.format("%d", g_morlockConstants.accessoryRunTime) );

	SetDlgItemText( IDC_DIMMER, buf.format("%d", g_morlockConstants.dimmer) );

	SetDlgItemText( IDC_EYE_HOLDOFF, buf.format("%d", g_morlockConstants.eyeHoldoff) );
	SetDlgItemText( IDC_BOLT_HOLDOFF, buf.format("%d", g_morlockConstants.boltHoldoff) );
	((CButton *)GetDlgItem( IDC_EYE_ENABLED ))->SetCheck( g_morlockConstants.eyeEnabled );
	SetDlgItemText( IDC_EYE_DETECT_LEVEL, buf.format("%d", g_morlockConstants.eyeDetectLevel) );
	SetDlgItemText( IDC_EYE_DETECT_HOLDOFF, buf.format("%d", g_morlockConstants.eyeDetectHoldoff) );
	((CButton *)GetDlgItem( IDC_EYE_SENSE_REVERSE ))->SetCheck( g_morlockConstants.eyeHighBlocked );
	
	((CButton *)GetDlgItem( IDC_CHECK3 ))->SetCheck( g_morlockConstants.locked );



	SetDlgItemText( IDC_SHORT_CYCLE_PREVENT, buf.format("%d", g_morlockConstants.shortCyclePreventionInterval) );

	if ( ((CButton *)GetDlgItem(IDC_AUTO_APPLY))->GetState() & 0x3 )
	{
		USBComm::g_morlockConstantsNeedCommit = true;
		m_morlockConstantsDirty = false;
	}
	
	renderGraphics();
}

//------------------------------------------------------------------------------
void CMorlockDlg::save()
{
	JsonValue val;

	val.add( "SingleSolenoid", (long long)g_morlockConstants.singleSolenoid );
	val.add( "FireMode", (long long)g_morlockConstants.fireMode );
	val.add( "BallsPerSecond", (float)g_morlockConstants.ballsPerSecondX10 / 10.f );
	val.add( "BurstCount", (long long)g_morlockConstants.burstCount );
	val.add( "EnhancedTriggerTimeout", (long long)g_morlockConstants.enhancedTriggerTimeout );
	val.add( "BoltHoldoff", (long long)g_morlockConstants.boltHoldoff );
	val.add( "AccessortRunTime", (long long)g_morlockConstants.accessoryRunTime );
	val.add( "Dimmer", (long long)g_morlockConstants.dimmer );
	val.add( "ABSTimeout", (long long)g_morlockConstants.ABSTimeout );
	val.add( "ABSAddition", (long long)g_morlockConstants.ABSAddition );
	val.add( "Rebounce", (long long)g_morlockConstants.rebounce );
	val.add( "Debounce", (long long)g_morlockConstants.debounce );
	val.add( "Dwell1", (long long)g_morlockConstants.dwell1 );
	val.add( "Dwell2Holdoff", (long long)g_morlockConstants.dwell2Holdoff );
	val.add( "Dwell2", (long long)g_morlockConstants.dwell2 );
	val.add( "maxDwell2", (long long)g_morlockConstants.maxDwell2 );
	val.add( "eyeHoldoff", (long long)g_morlockConstants.eyeHoldoff );
	val.add( "eyeHighBlocked", (long long)g_morlockConstants.eyeHighBlocked );
	val.add( "Locked", (long long)g_morlockConstants.locked );
	val.add( "RampEnableCount", (long long)g_morlockConstants.rampEnableCount );
	val.add( "RampClimb", (long long)g_morlockConstants.rampClimb );
	val.add( "RampTopMode", (long long)g_morlockConstants.rampTopMode );
	val.add( "RampTimeout", (long long)g_morlockConstants.rampTimeout );
	val.add( "EyeEnabled", (long long)g_morlockConstants.eyeEnabled );
	val.add( "EyeDetectLevel", (long long)g_morlockConstants.eyeDetectLevel );
	val.add( "ShortCyclePreventionInterval", (long long)g_morlockConstants.shortCyclePreventionInterval );
	val.add( "EyeDetectHoldoff", (long long)g_morlockConstants.eyeDetectHoldoff );

	Cstr buffer;
	val.writePretty( buffer );
	buffer.bufferToFile( m_currentPath );

	m_morlockConstantsDirty = false;
}

//------------------------------------------------------------------------------
unsigned int CMorlockDlg::getJsonValue( const char* value, JsonValue &stream )
{
	JsonValue *ret = stream.get( value );
	if ( !ret )
	{
		Cstr temp;
		MessageBox( temp.format("Morlock profile corrupt/invalid [%s] key not found", value), "error", MB_OK | MB_ICONSTOP);
		return 0;
	}
	else
	{
		return (unsigned int)ret->asInt();
	}
}

//------------------------------------------------------------------------------
void CMorlockDlg::load( Cstr& buf )
{
	JsonValue stream( buf );

	if ( !stream.valid() )
	{
		MessageBox( "Morlock profile is corrupt", "error", MB_OK | MB_ICONSTOP);
	}

	g_morlockConstants.singleSolenoid = (uint8)getJsonValue( "SingleSolenoid", stream );
	g_morlockConstants.fireMode = (uint8)getJsonValue( "fireMode", stream );
	g_morlockConstants.ballsPerSecondX10 = (uint16)getJsonValue( "ballsPerSecondX10", stream );
	g_morlockConstants.burstCount = (uint8)getJsonValue( "burstCount", stream );
	g_morlockConstants.enhancedTriggerTimeout = (uint16)getJsonValue( "enhancedTriggerTimeout", stream );
	g_morlockConstants.boltHoldoff = (uint8)getJsonValue( "boltHoldoff", stream );
	g_morlockConstants.accessoryRunTime = (uint8)getJsonValue( "accessoryRunTime", stream );
	g_morlockConstants.dimmer = (uint8)getJsonValue( "dimmer", stream );
	g_morlockConstants.ABSTimeout = (uint16)getJsonValue( "ABSTimeout", stream );
	g_morlockConstants.ABSAddition = (uint8)getJsonValue( "ABSAddition", stream );
	g_morlockConstants.rebounce = (uint8)getJsonValue( "rebounce", stream );
	g_morlockConstants.debounce = (uint8)getJsonValue( "debounce", stream );
	g_morlockConstants.dwell1 = (uint8)getJsonValue( "dwell1", stream );
	g_morlockConstants.dwell2Holdoff = (uint8)getJsonValue( "dwell2Holdoff", stream );
	g_morlockConstants.dwell2 = (uint8)getJsonValue( "dwell2", stream );
	g_morlockConstants.maxDwell2 = (uint8)getJsonValue( "maxDwell2", stream );
	g_morlockConstants.eyeHoldoff = (uint8)getJsonValue( "eyeHoldoff", stream );
	g_morlockConstants.eyeHighBlocked = (uint8)getJsonValue( "eyeHighBlocked", stream );
	g_morlockConstants.locked = (uint8)getJsonValue( "locked", stream );
	g_morlockConstants.rampEnableCount = (uint8)getJsonValue( "rampEnableCount", stream );
	g_morlockConstants.rampClimb = (uint8)getJsonValue( "rampClimb", stream );
	g_morlockConstants.rampTopMode = (uint8)getJsonValue( "rampTopMode", stream );
	g_morlockConstants.rampTimeout = (uint16)getJsonValue( "rampTimeout", stream );
	g_morlockConstants.eyeEnabled = (uint8)getJsonValue( "eyeEnabled", stream );
	g_morlockConstants.eyeDetectLevel = (uint8)getJsonValue( "eyeDetectLevel", stream );
	g_morlockConstants.shortCyclePreventionInterval = (uint8)getJsonValue( "shortCyclePreventionInterval", stream );
	g_morlockConstants.eyeDetectHoldoff = (uint8)getJsonValue( "eyeDetectHoldoff", stream );

	populateDialogFromConstants();
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
unsigned int CMorlockDlg::textLength( const char* text, const MFont* font )
{
	unsigned int length = 0;
	for( int i=0; text[i]; i++ )
	{
		char c = text[i] - 32;
		length += font[c].pre + font[c].post + font[c].w;
	}

	return length;
}

//------------------------------------------------------------------------------
BOOL CMorlockDlg::PreTranslateMessage(MSG* pMsg)
{
	m_tips.RelayEvent(pMsg);  
	return CDialog::PreTranslateMessage(pMsg);
}

//------------------------------------------------------------------------------
void CMorlockDlg::OnDeltaposSpin1(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMUPDOWN pNMUpDown = reinterpret_cast<LPNMUPDOWN>(pNMHDR);
	m_morlockConstantsDirty = true;
	g_morlockConstants.dwell1 -= pNMUpDown->iDelta;

	if ( g_morlockConstants.dwell1 < 1 )
	{
		g_morlockConstants.dwell1 = 1;
	}

	populateDialogFromConstants();

	*pResult = 0;
}

//------------------------------------------------------------------------------
void CMorlockDlg::OnDeltaposDwell2HoldoffSpin(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMUPDOWN pNMUpDown = reinterpret_cast<LPNMUPDOWN>(pNMHDR);
	*pResult = 0;

	m_morlockConstantsDirty = true;
	g_morlockConstants.dwell2Holdoff -= pNMUpDown->iDelta;
	populateDialogFromConstants();
}

//------------------------------------------------------------------------------
void CMorlockDlg::OnDeltaposDwell2Spin(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMUPDOWN pNMUpDown = reinterpret_cast<LPNMUPDOWN>(pNMHDR);
	*pResult = 0;

	m_morlockConstantsDirty = true;
	g_morlockConstants.dwell2 -= pNMUpDown->iDelta;
	if ( g_morlockConstants.dwell2 < 1 )
	{
		g_morlockConstants.dwell2 = 1;
	}

	populateDialogFromConstants();
}

//------------------------------------------------------------------------------
void CMorlockDlg::OnDeltaposMaxDwell2Spin(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMUPDOWN pNMUpDown = reinterpret_cast<LPNMUPDOWN>(pNMHDR);
	*pResult = 0;

	m_morlockConstantsDirty = true;
	if ( (g_morlockConstants.maxDwell2 - pNMUpDown->iDelta) >= g_morlockConstants.dwell2  )
	{
		g_morlockConstants.maxDwell2 -= pNMUpDown->iDelta;
		if ( g_morlockConstants.maxDwell2 < 1 )
		{
			g_morlockConstants.maxDwell2 = 1;
		}

		populateDialogFromConstants();
	}
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
	g_morlockConstants.ballsPerSecondX10 -= pNMUpDown->iDelta;
	if ( g_morlockConstants.ballsPerSecondX10 < 10 )
	{
		g_morlockConstants.ballsPerSecondX10 = 10;
	}
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
		g_morlockConstants.ABSTimeout = 30000;
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
	g_morlockConstants.ABSTimeout -= pNMUpDown->iDelta * 1000;
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
						 _T(".json"),
						 NULL,
						 OFN_HIDEREADONLY | OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_READONLY,
						 _T("Morlock JSON (*.json)|*.json||"));

	if ( dlgFile.DoModal() == IDOK)
	{
		POSITION startPos = dlgFile.GetStartPosition();
		while( startPos )
		{
			m_currentPath = dlgFile.GetNextPathName( startPos );

			Cstr buffer;
			if ( buffer.fileToBuffer(m_currentPath) )
			{
				load( buffer );
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
							 _T(".json"),
							 buf,
							 OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
							 _T("Morlock JSON (*.json)|*.json||"));

		if ( dlgFile.DoModal() != IDOK )
		{
			return;
		}

		m_currentPath = dlgFile.GetPathName();
	}

	m_currentPath = "c:\\curt\\test.json";
	if ( m_currentPath.size() )
	{
		save();
	}
}

//------------------------------------------------------------------------------
void CMorlockDlg::OnBnClickedApply()
{
	if ( !USBComm::g_connected )
	{
		MessageBox( "Cannot commit changes, no device detected", "Error", MB_OK | MB_ICONSTOP );
		return;
	}
	
	USBComm::g_morlockConstantsNeedCommit = true;
	m_morlockConstantsDirty = false;
}

//------------------------------------------------------------------------------
void CMorlockDlg::OnDeltaposEyeHoldoffSpin(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMUPDOWN pNMUpDown = reinterpret_cast<LPNMUPDOWN>(pNMHDR);
	*pResult = 0;

	m_morlockConstantsDirty = true;
	if ( (int)g_morlockConstants.eyeHoldoff - pNMUpDown->iDelta >= 0 )
	{
		g_morlockConstants.eyeHoldoff -= pNMUpDown->iDelta;
	}
	populateDialogFromConstants();
}

//------------------------------------------------------------------------------
void CMorlockDlg::OnBnClickedEyeSenseReverse()
{
	m_morlockConstantsDirty = true;
	if ( ((CButton *)GetDlgItem(IDC_EYE_SENSE_REVERSE))->GetState() & 0x3 )
	{
		g_morlockConstants.eyeHighBlocked = 1;
	}
	else
	{
		g_morlockConstants.eyeHighBlocked = 0;
	}
	populateDialogFromConstants();
}

//------------------------------------------------------------------------------
void CMorlockDlg::OnDeltaposDimmerSpin(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMUPDOWN pNMUpDown = reinterpret_cast<LPNMUPDOWN>(pNMHDR);
	*pResult = 0;
	m_morlockConstantsDirty = true;
	g_morlockConstants.dimmer -= pNMUpDown->iDelta;
	if ( g_morlockConstants.dimmer > 8 )
	{
		g_morlockConstants.dimmer = 8;
	}
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

//------------------------------------------------------------------------------
void CMorlockDlg::OnBnClickedEyeEnabled()
{
	m_morlockConstantsDirty = true;
	if ( ((CButton *)GetDlgItem(IDC_EYE_ENABLED))->GetState() & 0x3 )
	{
		g_morlockConstants.eyeEnabled = 1;
	}
	else
	{
		g_morlockConstants.eyeEnabled = 0;
	}
	populateDialogFromConstants();
}

//------------------------------------------------------------------------------
void CMorlockDlg::OnBnClickedCheck3()
{
	m_morlockConstantsDirty = true;
	if ( ((CButton *)GetDlgItem(IDC_CHECK3))->GetState() & 0x3 )
	{
		g_morlockConstants.locked = 1;
	}
	else
	{
		g_morlockConstants.locked = 0;
	}
	populateDialogFromConstants();
}

//------------------------------------------------------------------------------
void CMorlockDlg::OnCbnSelchangeRampPresets()
{
	CComboBox *combo = (CComboBox *)GetDlgItem( IDC_RAMP_PRESETS );
	int preset = combo->GetItemData( combo->GetCurSel() );

	switch( preset )
	{
		case ceRampTurbo:
		{
			m_morlockConstantsDirty = true;
			g_morlockConstants.rampTopMode = ceSemi;
			g_morlockConstants.ballsPerSecondX10 = 15;
			g_morlockConstants.enhancedTriggerTimeout = 120;
			g_morlockConstants.rampEnableCount = 1;
			g_morlockConstants.rampTimeout = 120;
			g_morlockConstants.rampClimb = 1;
			break;	
		}
		
		case ceRampDoubleTap:
		{
			m_morlockConstantsDirty = true;
			g_morlockConstants.rampTopMode = ceAutoresponse;
			g_morlockConstants.ballsPerSecondX10 = 15;
			g_morlockConstants.enhancedTriggerTimeout = 1000;
			g_morlockConstants.rampEnableCount = 3;
			g_morlockConstants.rampTimeout = 1000;
			g_morlockConstants.rampClimb = 1;
			break;	
		}			
   
		case ceRampOMG:
		{
			m_morlockConstantsDirty = true;
			g_morlockConstants.rampTopMode = ceFullAuto;
			g_morlockConstants.ballsPerSecondX10 = 15;
			g_morlockConstants.enhancedTriggerTimeout = 1000;
			g_morlockConstants.rampEnableCount = 3;
			g_morlockConstants.rampTimeout = 1000;
			g_morlockConstants.rampClimb = 2;
			break;	
		}

		case ceRampMillenium:
		{
			m_morlockConstantsDirty = true;
			g_morlockConstants.rampTopMode = ceSemi;
			g_morlockConstants.ballsPerSecondX10 = 105;
			g_morlockConstants.enhancedTriggerTimeout = 200;
			g_morlockConstants.rampEnableCount = 3;
			g_morlockConstants.rampTimeout = 200;
			g_morlockConstants.rampClimb = 1;
			break;	
		}

		default:
		{
			break;
		}
	}
	populateDialogFromConstants();

}

//------------------------------------------------------------------------------
void CMorlockDlg::OnBnClickedReflash()
{
	CSettings settings;
	USBComm::g_settingsDialog = &settings;
	settings.DoModal();
	populateDialogFromConstants();
	USBComm::g_settingsDialog = 0;
}

//------------------------------------------------------------------------------
void CMorlockDlg::OnDeltaposEyeDetectSpin(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMUPDOWN pNMUpDown = reinterpret_cast<LPNMUPDOWN>(pNMHDR);
	*pResult = 0;
	m_morlockConstantsDirty = true;
	g_morlockConstants.eyeDetectLevel -= pNMUpDown->iDelta;
	populateDialogFromConstants();
}

//------------------------------------------------------------------------------
void CMorlockDlg::OnDeltaposShortCyclePreventSpin(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMUPDOWN pNMUpDown = reinterpret_cast<LPNMUPDOWN>(pNMHDR);
	*pResult = 0;
	m_morlockConstantsDirty = true;
	g_morlockConstants.shortCyclePreventionInterval -= pNMUpDown->iDelta;
	populateDialogFromConstants();
}

//------------------------------------------------------------------------------
void CMorlockDlg::OnDeltaposEyeDetectHoldoffSpin2(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMUPDOWN pNMUpDown = reinterpret_cast<LPNMUPDOWN>(pNMHDR);
	*pResult = 0;
	m_morlockConstantsDirty = true;
	g_morlockConstants.eyeDetectHoldoff -= pNMUpDown->iDelta;
	populateDialogFromConstants();
}

//------------------------------------------------------------------------------
void CMorlockDlg::OnBnClickedAutoApply()
{
}

//------------------------------------------------------------------------------
void CMorlockDlg::OnBnClickedEyeStrong()
{
	m_morlockConstantsDirty = true;
	g_morlockConstants.eyeStrong = 1;
	populateDialogFromConstants();
}

//------------------------------------------------------------------------------
void CMorlockDlg::OnBnClickedEyeWeak()
{
	m_morlockConstantsDirty = true;
	g_morlockConstants.eyeStrong = 0;
	populateDialogFromConstants();
}
