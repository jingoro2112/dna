/* Copyright: (c) 2013 by Curt Hartung
 * This work is released under the Creating Commons 3.0 license
 * found at http://creativecommons.org/licenses/by-nc-sa/3.0/legalcode
 * and in the LICENCE.txt file included with this distribution
 */

#include "stdafx.h"
#include "morlock.h"
#include "morlockDlg.h"
#include "afxdialogex.h"

#include "../dnausb/dnausb.h"
#include "../firmware/dna/dna_defs.h"
#include "../firmware/morlock/eeprom_consts.h"
#include "../util/str.hpp"

EEPROMConstants g_morlockConstants;
bool g_morlockConstantsDirty = false;

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

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
END_MESSAGE_MAP()

//------------------------------------------------------------------------------
BOOL CMorlockDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	m_fireCycle.init( 200, 50, 20, 20 );

	memset( &g_morlockConstants, 0, sizeof(g_morlockConstants) );

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
	CDialogEx::OnCancel();
}

//------------------------------------------------------------------------------
void CMorlockDlg::buildFireCycleGraphic()
{

	m_fireCycle.blit( ::GetDC(GetSafeHwnd()) );
}

//------------------------------------------------------------------------------
void CMorlockDlg::populateDialogFromConstants()
{
	Cstr buf;
	
	buf.format( "%d", g_morlockConstants.dwell1 );
	SetDlgItemText( IDC_DWELL1, buf );
}


//------------------------------------------------------------------------------
void CMorlockDlg::morlockCommThread( void* arg )
{
	CMorlockDlg *md = (CMorlockDlg *)arg;
	
	unsigned char buffer[64];
	Cstr temp;
	DNADEVICE device = INVALID_DNADEVICE_VALUE;
	for( ;; )
	{
		if ( device == INVALID_DNADEVICE_VALUE )
		{
			md->SetDlgItemText( IDC_PRODUCT, "n/a" );
			md->SetDlgItemText( IDC_STATUS, "DISCONNECTED" );
			
			if ( !(device = DNAUSB::openDevice(0x16C0, 0x05DF, "dna@northarc.com", (char  *)buffer)) )
			{
				Sleep( 500 );
				continue;
			}

			unsigned char id;
			if ( !DNAUSB::getProductId(device, &id) )
			{
				goto disconnected;
			}
			
			md->SetDlgItemText( IDC_PRODUCT, temp.format("%s [%d]", buffer, (int)id) );
			md->SetDlgItemText( IDC_STATUS, "connected" );

			buffer[0] = ceCommandGetConstants;
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

		if ( g_morlockConstantsDirty )
		{
			md->SetDlgItemText( IDC_STATUS, "sending..." );

			g_morlockConstantsDirty = false;
		}

		continue;
		
disconnected:
		DNAUSB::closeDevice( device );
		device = INVALID_DNADEVICE_VALUE;
	}
}

//------------------------------------------------------------------------------
void CMorlockDlg::OnDeltaposSpin1(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMUPDOWN pNMUpDown = reinterpret_cast<LPNMUPDOWN>(pNMHDR);

	g_morlockConstants.dwell1 -= pNMUpDown->iDelta;

	populateDialogFromConstants();

/*
	buf.fileToBuffer( "digital_16.mwl" );
	Cstr out;
	for( int i=0; i<128; i++ )
	{
		for ( int j=0; j<128; j++ )
		{
			out.appendFormat( "0x%02X, ", (unsigned char)buf[j + i*128 + 8] );
		}

		out += "\n";
	}
	out.bufferToFile( "header.h" );
*/	
	
	*pResult = 0;
}
