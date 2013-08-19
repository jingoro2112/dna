/* Copyright: (c) 2013 by Curt Hartung
 * This work is released under the Creating Commons 3.0 license
 * found at http://creativecommons.org/licenses/by-nc-sa/3.0/legalcode
 * and in the LICENCE.txt file included with this distribution
 */

#pragma once
#include "../util/canvas.hpp"
#include "../util/str.hpp"
#include "font.h"

//------------------------------------------------------------------------------
class CMorlockDlg : public CDialogEx
{
public:
	CMorlockDlg(CWnd* pParent = NULL);

// Dialog Data
	enum { IDD = IDD_MORLOCK_DIALOG };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support

	void drawBar( Canvas& canvas, unsigned int originX, unsigned int originY, unsigned int length, unsigned int r, unsigned int g, unsigned int b, const char* label );
	void drawInvertedBar( Canvas& canvas, unsigned int originX, unsigned int originY, unsigned int length, unsigned int r, unsigned int g, unsigned int b, const char* label );
	void renderGraphics();
	void populateDialogFromConstants();
	void save();
	void load( Cstr& buf );
	
	static void morlockCommThread( void* arg );

	Canvas m_fireCycle;
	CToolTipCtrl m_tips;

	bool m_connected;
	static bool m_active;
	bool m_morlockConstantsDirty;
	bool m_morlockConstantsNeedCommit;
	Cstr m_currentPath;

	HICON m_hIcon;

	unsigned int getTextWidth( const char* string, const MFont* font );
	void textAt( const char* text, Canvas& canvas, const MFont* font, const unsigned char* bitmap,
				 int x, int y, float r, float g, float b, float a );
	unsigned int textLength( const char* text, const MFont* font );

	// Generated message map functions
	BOOL PreTranslateMessage( MSG* pMsg );
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
			
public:
	afx_msg void OnBnClickedOk();
	afx_msg void OnBnClickedCancel();
	afx_msg void OnBnClickedButton1();
	afx_msg void OnDeltaposSpin1(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnDeltaposDwell2HoldoffSpin(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnDeltaposDwell2Spin(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnDeltaposMaxDwell2Spin(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnBnClickedSingleSolenoid();
	afx_msg void OnBnClickedDualSolenoid();
	afx_msg void OnDeltaposRofSpin(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnDeltaposShotsInBurstSpin(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnDeltaposEnhancedTriggerTimeoutSpin(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnCbnSelchangeFireMode();
	afx_msg void OnDeltaposRampEnableCountSpin(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnCbnSelchangeRampTopMode();
	afx_msg void OnDeltaposRampTimeoutSpin(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnBnClickedAbs();
	afx_msg void OnDeltaposAbsTimeoutSpin(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnDeltaposAbsAdditionSpin(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnDeltaposRebounceSpin(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnBnClickedLoad();
	afx_msg void OnBnClickedSave();
	afx_msg void OnBnClickedApply();
	afx_msg void OnDeltaposEyeHoldoffSpin(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnBnClickedEyeSenseReverse();
	afx_msg void OnDeltaposDimmerSpin(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnDeltaposAccesoryRuntimeSpin(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnDeltaposRampClimbSpin(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnDeltaposDebounceSpin(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnDeltaposBoltHoldoffSpin(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnBnClickedSaveAs();
	afx_msg void OnBnClickedEyeEnabled();
	afx_msg void OnBnClickedCheck3();
	afx_msg void OnCbnSelchangeRampPresets();
};
