/* Copyright: (c) 2013 by Curt Hartung
 * This work is released under the Creating Commons 3.0 license
 * found at http://creativecommons.org/licenses/by-nc-sa/3.0/legalcode
 * and in the LICENCE.txt file included with this distribution
 */

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
	void populateDialogFromConstants();

	void rigForMorlock();
	void rigForUnknown();
	void rigForDisconnected();
	
	static void morlockCommThread( void* arg );

	Canvas m_fireCycle;

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
