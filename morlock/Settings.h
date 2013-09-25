/* Copyright: (c) 2013 by Curt Hartung avr@northarc.com
 * This work is released under the Creating Commons 3.0 license
 * found at http://creativecommons.org/licenses/by-nc-sa/3.0/legalcode
 * and in the LICENCE.txt file included with this distribution
 */

#pragma once

//------------------------------------------------------------------------------
class CSettings : public CDialogEx
{
	DECLARE_DYNAMIC(CSettings)

public:
	CSettings(CWnd* pParent = NULL);   // standard constructor
	virtual ~CSettings();

	CProgressCtrl *m_progress;

	void disableControls();
	void enableControls();

	enum { IDD = IDD_DIALOG1 };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	virtual BOOL OnInitDialog();

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedCancel();
	afx_msg void OnBnClickedDone();
	afx_msg void OnBnClickedFactoryReset();
	afx_msg void OnBnClickedFlashFromInternet();
	afx_msg void OnBnClickedFlashFromFile();
	afx_msg void OnNMCustomdrawProgress1(NMHDR *pNMHDR, LRESULT *pResult);
};
