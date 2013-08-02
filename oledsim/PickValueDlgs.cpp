#include "stdafx.h"
#include "oledsim.h"
#include "PickValueDlgs.h"
#include "afxdialogex.h"


// CPickValueDlgs dialog

IMPLEMENT_DYNAMIC(CPickValueDlgs, CDialogEx)

CPickValueDlgs::CPickValueDlgs(CWnd* pParent /*=NULL*/)
	: CDialogEx(CPickValueDlgs::IDD, pParent)
{
}

BOOL CPickValueDlgs::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	char buf[256];
	sprintf( buf, "%d", m_x );
	SetDlgItemText( IDC_X, buf );
	
	sprintf( buf, "%d", m_y );
	SetDlgItemText( IDC_Y, buf );
	
	SetDlgItemText( IDC_PROMPT, m_prompt );

	return TRUE;
}

//------------------------------------------------------------------------------
CPickValueDlgs::~CPickValueDlgs()
{
}

//------------------------------------------------------------------------------
void CPickValueDlgs::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}


//------------------------------------------------------------------------------
BEGIN_MESSAGE_MAP(CPickValueDlgs, CDialogEx)
	ON_BN_CLICKED(IDOK, &CPickValueDlgs::OnBnClickedOk)
	ON_BN_CLICKED(IDCANCEL, &CPickValueDlgs::OnBnClickedCancel)
END_MESSAGE_MAP()


// CPickValueDlgs message handlers

//------------------------------------------------------------------------------
void CPickValueDlgs::OnBnClickedOk()
{
	CDialogEx::OnOK();
}

//------------------------------------------------------------------------------
void CPickValueDlgs::OnBnClickedCancel()
{
	CDialogEx::OnCancel();
}

