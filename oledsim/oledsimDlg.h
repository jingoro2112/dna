#pragma once

#include "../util/oled.hpp"
#include "../util/str.hpp"
#include "../util/canvas.hpp"

//------------------------------------------------------------------------------
class COledSimDlg : public CDialogEx
{
public:
	COledSimDlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	enum { IDD = IDD_OLEDSIM_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support

	void clearCanvas();
	void setPixel( int x, int y, bool pixel =true );
	void redraw();
	void oledToCanvas( unsigned char map[4096] );
	void checkMouse( CPoint& point );
	void save();
	
	Canvas m_canvasBig;
	Canvas m_canvasSmall;
	Oled m_oled;
	Cstr m_currentPath;

	bool m_placePixels;
	bool m_erasePixels;

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
	afx_msg void OnBnClickedTest1();
	afx_msg void OnBnClickedImport();
	afx_msg void OnBnClickedExport();
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnBnClickedSave();
	afx_msg void OnBnClickedSaveAs();
	afx_msg void OnBnClickedLoad();
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
};
