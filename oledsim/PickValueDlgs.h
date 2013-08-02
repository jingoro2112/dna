#pragma once

#include "../util/str.hpp"

//------------------------------------------------------------------------------
class CPickValueDlgs : public CDialogEx
{
	DECLARE_DYNAMIC(CPickValueDlgs)

public:
	CPickValueDlgs(CWnd* pParent = NULL);   // standard constructor
	virtual ~CPickValueDlgs();

	void setPrompt( const char* prompt ) { m_prompt = prompt; }
	void setX( int x ) { m_x = x; }
	void setY( int y ) { m_y = y; }
	int getX() const { return m_x; }
	int getY() const { return m_y; }

// Dialog Data
	enum { IDD = IDD_PICK_VALUES };

	BOOL OnInitDialog();
	
protected:
	int m_x;
	int m_y;
	Cstr m_prompt;
	
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedOk();
	afx_msg void OnBnClickedCancel();
};
