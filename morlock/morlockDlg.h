#pragma once
#include "../util/canvas.hpp"

//------------------------------------------------------------------------------
class CMorlockDlg : public CDialogEx
{
public:
	CMorlockDlg(CWnd* pParent = NULL);

// Dialog Data
	enum { IDD = IDD_MORLOCK_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support

	void buildFireCycleGraphic();

	Canvas m_fireCycle;

	// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedOk();
	afx_msg void OnBnClickedCancel();
	afx_msg void OnBnClickedButton1();
	afx_msg void OnDeltaposSpin1(NMHDR *pNMHDR, LRESULT *pResult);
};
