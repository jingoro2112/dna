/* Copyright: (c) 2013 by Curt Hartung avr@northarc.com
 * This work is released under the Creating Commons 3.0 license
 * found at http://creativecommons.org/licenses/by-nc-sa/3.0/legalcode
 * and in the LICENCE.txt file included with this distribution
 */

#include "stdafx.h"
#include "morlock.h"
#include "Settings.h"
#include "afxdialogex.h"

#include "comm_thread.hpp"

#include "../util/http_comm.hpp"
#include "../util/read_hex.hpp"
#include "../util/json_parser.hpp"
#include "../util/simple_log.hpp"

#include "../splice/splice.hpp"

#include "../firmware/morlock/morlock_defs.h"


IMPLEMENT_DYNAMIC(CSettings, CDialogEx)
extern EEPROMConstants g_morlockConstants;

//------------------------------------------------------------------------------
CSettings::CSettings(CWnd* pParent /*=NULL*/)
	: CDialogEx(CSettings::IDD, pParent)
{

}

//------------------------------------------------------------------------------
BOOL CSettings::OnInitDialog()
{
	m_progress = (CProgressCtrl *)GetDlgItem( IDC_PROGRESS1 );

	m_progress->SetRange( 0, 100 );
	m_progress->SetStep( 2 );
	m_progress->SetPos( 0 );

	SetDlgItemText( IDC_VERSION, MORLOCK_APP_VERSION );
	
	return TRUE;  // return TRUE  unless you set the focus to a control
}

//------------------------------------------------------------------------------
CSettings::~CSettings()
{
}

//------------------------------------------------------------------------------
void CSettings::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

//------------------------------------------------------------------------------
BEGIN_MESSAGE_MAP(CSettings, CDialogEx)
	ON_BN_CLICKED(IDCANCEL, &CSettings::OnBnClickedCancel)
	ON_BN_CLICKED(IDC_DONE, &CSettings::OnBnClickedDone)
	ON_BN_CLICKED(IDC_FACTORY_RESET, &CSettings::OnBnClickedFactoryReset)
	ON_BN_CLICKED(IDC_FLASH_FROM_INTERNET, &CSettings::OnBnClickedFlashFromInternet)
	ON_BN_CLICKED(IDC_FLASH_FROM_FILE, &CSettings::OnBnClickedFlashFromFile)
	ON_NOTIFY(NM_CUSTOMDRAW, IDC_PROGRESS1, &CSettings::OnNMCustomdrawProgress1)
END_MESSAGE_MAP()

//------------------------------------------------------------------------------
void CSettings::disableControls()
{
	((CButton *)GetDlgItem( IDC_FLASH_FROM_INTERNET ))->EnableWindow( false );	
	((CButton *)GetDlgItem( IDC_FLASH_FROM_FILE ))->EnableWindow( false );	
	((CButton *)GetDlgItem( IDC_FACTORY_RESET ))->EnableWindow( false );	
	((CButton *)GetDlgItem( IDC_DONE ))->EnableWindow( false );	
}

//------------------------------------------------------------------------------
void CSettings::enableControls()
{
	((CButton *)GetDlgItem( IDC_FLASH_FROM_INTERNET ))->EnableWindow( true );	
	((CButton *)GetDlgItem( IDC_FLASH_FROM_FILE ))->EnableWindow( true );	
	((CButton *)GetDlgItem( IDC_FACTORY_RESET ))->EnableWindow( true );	
	((CButton *)GetDlgItem( IDC_DONE ))->EnableWindow( true );
	m_progress->SetPos( 0 );
}

//------------------------------------------------------------------------------
void CSettings::OnBnClickedCancel()
{
	
}

//------------------------------------------------------------------------------
void CSettings::OnBnClickedDone()
{
	CDialogEx::OnCancel();
}

//------------------------------------------------------------------------------
void CSettings::OnBnClickedFactoryReset()
{
	if ( MessageBox("Install factory defaults?", "Are You Sure?", MB_OKCANCEL | MB_ICONSTOP ) != IDOK )
	{
		return;
	}

	g_morlockConstants.installDefaults();
	USBComm::g_morlockConstantsNeedCommit = true;
}

//------------------------------------------------------------------------------
void CSettings::OnBnClickedFlashFromInternet()
{
	if ( !USBComm::g_connected )
	{
		MessageBox( "Hardware not detected", "Error", MB_OK | MB_ICONSTOP );
		return;
	}

	Cstr hexImage;
	if ( !HTTP::fetchDocument(hexImage, "northarc.com", "dna_publish/morlock-latest.hex") )
	{
		MessageBox("Internet problem, could not fetch image",  "info", MB_OK | MB_ICONSTOP );
		return;
	}

	CLinkList<ReadHex::Chunk> chunklist;
	if ( !ReadHex::parse( chunklist, hexImage.c_str(), hexImage.size() ) )
	{
		MessageBox("Fetched image, but could not parse HEX data",  "info", MB_OK | MB_ICONSTOP );
		return;
	}

	if ( chunklist.count() != 1 )
	{
		MessageBox("Fetched image is corrupt <1>",  "info", MB_OK | MB_ICONSTOP );
		return;
	}

	ReadHex::Chunk *chunk = chunklist.getFirst();
	USBComm::g_codeImage.setString( (char *)chunk->data, chunk->size );	

	if ( !Splice::checkDNAImage( (unsigned char *)USBComm::g_codeImage.c_str(), USBComm::g_codeImage.size() ) )
	{
		MessageBox("Fetched image is corrupt <2>",  "info", MB_OK | MB_ICONSTOP );
		return;
	}

	if ( MessageBox("Reflash DNA board?", "Are You Sure?", MB_OKCANCEL | MB_ICONSTOP ) != IDOK )
	{
		return;
	}

	disableControls();
	m_progress->SetPos( 2 );
	USBComm::g_reflash = true;
}

//------------------------------------------------------------------------------
void CSettings::OnBnClickedFlashFromFile()
{
	if ( !USBComm::g_connected )
	{
		MessageBox( "Hardware not detected", "Error", MB_OK | MB_ICONSTOP );
		return;
	}

	Cstr hexImage;
	CFileDialog dlgFile( TRUE,
						 _T(".hex"),
						 NULL,
						 OFN_HIDEREADONLY | OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_READONLY,
						 _T("Morlock HEX (*.hex)|*.hex||"));

	if ( dlgFile.DoModal() == IDOK)
	{
		POSITION startPos = dlgFile.GetStartPosition();
		if( !startPos )
		{
			MessageBox("Load error <1>",  "info", MB_OK | MB_ICONSTOP );
			return;
		}
		
		Cstr path = dlgFile.GetNextPathName( startPos );
		if ( !hexImage.fileToBuffer(path) )
		{
			MessageBox("Load error <2>",  "info", MB_OK | MB_ICONSTOP );
			return;
		}
	}
	else
	{
		return;
	}

	CLinkList<ReadHex::Chunk> chunklist;
	if ( !ReadHex::parse( chunklist, hexImage.c_str(), hexImage.size() ) )
	{
		MessageBox("Could not parse HEX data",  "info", MB_OK | MB_ICONSTOP );
		return;
	}

	if ( chunklist.count() != 1 )
	{
		MessageBox("Image is corrupt <1>",  "info", MB_OK | MB_ICONSTOP );
		return;
	}

	ReadHex::Chunk *chunk = chunklist.getFirst();
	USBComm::g_codeImage.setString( (char *)chunk->data, chunk->size );	

	if ( !Splice::checkDNAImage( (unsigned char *)USBComm::g_codeImage.c_str(), USBComm::g_codeImage.size() ) )
	{
		MessageBox("Image is corrupt <2>",  "info", MB_OK | MB_ICONSTOP );
		return;
	}

	if ( MessageBox("Reflash DNA board? Are you sure?", "info", MB_OKCANCEL | MB_ICONSTOP )  != IDOK )
	{
		return;
	}

	disableControls();
	m_progress->SetPos( 2 );
	USBComm::g_reflash = true;
}

//------------------------------------------------------------------------------
void CSettings::OnNMCustomdrawProgress1(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMCUSTOMDRAW pNMCD = reinterpret_cast<LPNMCUSTOMDRAW>(pNMHDR);
	*pResult = 0;
}
