#include "stdafx.h"
#include "oledsim.h"
#include "oledsimDlg.h"
#include "afxdialogex.h"
#include "PickValueDlgs.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

static CPickValueDlgs s_dlgVal;

COledSimDlg::COledSimDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(COledSimDlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void COledSimDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

//------------------------------------------------------------------------------
BEGIN_MESSAGE_MAP(COledSimDlg, CDialogEx)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDOK, &COledSimDlg::OnBnClickedOk)
	ON_BN_CLICKED(IDCANCEL, &COledSimDlg::OnBnClickedCancel)
	ON_BN_CLICKED(IDC_BUTTON1, &COledSimDlg::OnBnClickedButton1)
	ON_BN_CLICKED(IDC_TEST1, &COledSimDlg::OnBnClickedTest1)
	ON_BN_CLICKED(IDC_IMPORT, &COledSimDlg::OnBnClickedImport)
	ON_BN_CLICKED(IDC_EXPORT, &COledSimDlg::OnBnClickedExport)
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_MOUSEMOVE()
	ON_BN_CLICKED(IDC_SAVE, &COledSimDlg::OnBnClickedSave)
	ON_BN_CLICKED(IDC_SAVE_AS, &COledSimDlg::OnBnClickedSaveAs)
	ON_BN_CLICKED(IDC_LOAD, &COledSimDlg::OnBnClickedLoad)
	ON_WM_RBUTTONDOWN()
	ON_WM_RBUTTONUP()
END_MESSAGE_MAP()

//------------------------------------------------------------------------------
void COledSimDlg::clearCanvas()
{
	m_canvasBig.clear();
	m_canvasSmall.clear();
}
	
//------------------------------------------------------------------------------
void COledSimDlg::setPixel( int x, int y, bool pixel /*=true*/ )
{
	unsigned int c = pixel ? CANVAS_RGB(0xDD, 0xDD, 0xDD) : CANVAS_RGB(0,0,0);
	
	m_canvasBig.set( (x*4) + 1, (y*4) + 1, c );
	m_canvasBig.set( (x*4) + 2, (y*4) + 1, c );
	m_canvasBig.set( (x*4) + 3, (y*4) + 1, c );
	m_canvasBig.set( (x*4) + 1, (y*4) + 2, c );
	m_canvasBig.set( (x*4) + 2, (y*4) + 2, c );
	m_canvasBig.set( (x*4) + 3, (y*4) + 2, c );
	m_canvasBig.set( (x*4) + 1, (y*4) + 3, c );
	m_canvasBig.set( (x*4) + 2, (y*4) + 3, c );
	m_canvasBig.set( (x*4) + 3, (y*4) + 3, c );

	m_canvasSmall.set( x, y, c );

	m_oled.setPixel( x, y, pixel );
}

//------------------------------------------------------------------------------
void COledSimDlg::redraw()
{
	m_canvasBig.blit( ::GetDC(GetSafeHwnd()) );
	m_canvasSmall.blit( ::GetDC(GetSafeHwnd()) );
}

//------------------------------------------------------------------------------
void COledSimDlg::oledToCanvas( Oled& oled )
{
	clearCanvas();
	
	for( int y=0; y<4; y++ )
	{
		for( int x=0; x<128; x++ )
		{
			unsigned char byte = oled.getScreen()[y*128 + x];
			for( int b=0; b<8; b++ )
			{
				setPixel( x, y*8 + b, (byte & 1<<b) ? true : false );
			}
		}
	}

	m_canvasBig.blit( ::GetDC(GetSafeHwnd()) );
	m_canvasSmall.blit( ::GetDC(GetSafeHwnd()) );
}

//------------------------------------------------------------------------------
void COledSimDlg::checkMouse( CPoint& point )
{
	int x = (point.x - 21) / 4;
	int y = (point.y - 21) / 4;

	if ( m_placePixels )
	{
		if ( x >= 0 && x < 128
			 && y >= 0 && y < 32 )
		{
			setPixel( x, y );
		}
	}
	if ( m_erasePixels )
	{
		if ( x >= 0 && x < 128
			 && y >= 0 && y < 32 )
		{
			setPixel( x, y, false );
		}
	}

	redraw();
}

//------------------------------------------------------------------------------
void COledSimDlg::save()
{
	FILE *fil = fopen( m_currentPath, "wb" );
	if ( fil )
	{
		const int* buffer = m_canvasSmall.getBuffer();
		unsigned char c;
		for( int i=0; i<4096; i++ )
		{
			c = buffer[i] ? 0xFF : 0;
			fwrite( &c, 1, 1, fil );
		}

		fclose( fil );
	}
}

//------------------------------------------------------------------------------
BOOL COledSimDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	m_canvasBig.init( 513, 129, 20, 20 );
	m_canvasSmall.init( 128, 32, 220, 155 );

	m_placePixels = false;
	m_erasePixels = false;

	clearCanvas();

	return TRUE;  // return TRUE  unless you set the focus to a control
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.
//------------------------------------------------------------------------------
void COledSimDlg::OnPaint()
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
		m_canvasBig.blit( ::GetDC(GetSafeHwnd()) );
		m_canvasSmall.blit( ::GetDC(GetSafeHwnd()) );
	}
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR COledSimDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

//------------------------------------------------------------------------------
void COledSimDlg::OnBnClickedOk()
{
//	CDialogEx::OnOK();
}

//------------------------------------------------------------------------------
void COledSimDlg::OnBnClickedCancel()
{
	CDialogEx::OnCancel();
}

//------------------------------------------------------------------------------
void COledSimDlg::OnBnClickedButton1()
{
	CDialogEx::OnCancel();
}

//------------------------------------------------------------------------------
void COledSimDlg::OnBnClickedTest1()
{
	m_oled.clear();
	m_oled.stringAt( "DNA OLED (*&*&^)", 0, 0, 0 );
	m_oled.stringAt( "DNA OLED 123!@#$%^", 0, 16, 1 );
	oledToCanvas( m_oled );
}

//------------------------------------------------------------------------------
void COledSimDlg::OnBnClickedImport()
{
	CFileDialog dlgFile( TRUE,
						 _T(""),
						 NULL,
						 OFN_HIDEREADONLY | OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_READONLY,
						 _T("") );

	if ( dlgFile.DoModal() == IDOK)
	{
		s_dlgVal.setPrompt( "What resolution should this blob be imported as?" );
		s_dlgVal.setX( 128 );
		s_dlgVal.setY( 32 );
		
		if ( s_dlgVal.DoModal() == IDOK )
		{
			POSITION startPos = dlgFile.GetStartPosition();
			while( startPos )
			{
				CString path = dlgFile.GetNextPathName( startPos );

				Cstr buffer;
				if ( buffer.fileToBuffer(path) )
				{
					int index = 0;
					for( int y=0; y<s_dlgVal.getY(); y++ )
					{
						for( int x=0; x<s_dlgVal.getX(); x++ )
						{
							setPixel( x, y, buffer.c_str()[index++] ? true : false );
						}
					}

					redraw();
				}
			}
		}
	}
}

//------------------------------------------------------------------------------
void COledSimDlg::OnBnClickedExport()
{
	char buf[ MAX_PATH + 2 ] = "";
	CFileDialog dlgFile( FALSE,
						 _T(".h"),
						 buf,
						 OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
						 _T("C Header File (*.h)|*.h||"));

	if ( dlgFile.DoModal() == IDOK ) 
	{
		CString path = dlgFile.GetPathName();
	}
}

//------------------------------------------------------------------------------
void COledSimDlg::OnLButtonDown(UINT nFlags, CPoint point)
{
	m_placePixels = true;

	checkMouse( point );
	
	CDialogEx::OnLButtonDown( nFlags, point );
}

//------------------------------------------------------------------------------
void COledSimDlg::OnLButtonUp(UINT nFlags, CPoint point)
{
	m_placePixels = false;

	checkMouse( point );

	CDialogEx::OnLButtonUp(nFlags, point);
}

//------------------------------------------------------------------------------
void COledSimDlg::OnMouseMove(UINT nFlags, CPoint point)
{
	checkMouse( point );

	CDialogEx::OnMouseMove(nFlags, point);
}

//------------------------------------------------------------------------------
void COledSimDlg::OnBnClickedSave()
{
	if ( !m_currentPath.length() )
	{
		char buf[ MAX_PATH + 2 ] = "";
		CFileDialog dlgFile( FALSE,
							 _T(".obm"),
							 buf,
							 OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
							 _T("OLED Bitmap File (*.obm)|*.obm||"));

		if ( dlgFile.DoModal() != IDOK )
		{
			return;
		}

		m_currentPath = dlgFile.GetPathName();
	}

	save();
}

//------------------------------------------------------------------------------
void COledSimDlg::OnBnClickedSaveAs()
{
	char buf[ MAX_PATH + 2 ] = "";
	CFileDialog dlgFile( FALSE,
						 _T(".obm"),
						 buf,
						 OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
						 _T("OLED Bitmap File (*.obm)|*.obm||"));
	
	if ( dlgFile.DoModal() != IDOK )
	{
		return;
	}

	m_currentPath = dlgFile.GetPathName();
	
	save();
}

//------------------------------------------------------------------------------
void COledSimDlg::OnBnClickedLoad()
{
	CFileDialog dlgFile( TRUE,
						 _T(".obm"),
						 NULL,
						 OFN_HIDEREADONLY | OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_READONLY,
						 _T("OLED Bitmap File (*.obm)|*.obm||"));

	if ( dlgFile.DoModal() == IDOK)
	{
		POSITION startPos = dlgFile.GetStartPosition();
		while( startPos )
		{
			m_currentPath = dlgFile.GetNextPathName( startPos );

			Cstr buffer;
			if ( buffer.fileToBuffer(m_currentPath) )
			{
				int index = 0;
				for( int y=0; y<32; y++ )
				{
					for( int x=0; x<128; x++ )
					{
						setPixel( x, y, buffer.c_str()[index++] ? true : false );
					}
				}

				redraw();
			}
			else
			{
				m_currentPath = "";
			}
		}
	}
}

//------------------------------------------------------------------------------
void COledSimDlg::OnRButtonDown(UINT nFlags, CPoint point)
{
	m_erasePixels = true;
	checkMouse( point );

	CDialogEx::OnRButtonDown(nFlags, point);
}

//------------------------------------------------------------------------------
void COledSimDlg::OnRButtonUp(UINT nFlags, CPoint point)
{
	m_erasePixels = false;
	checkMouse( point );

	CDialogEx::OnRButtonUp(nFlags, point);
}
