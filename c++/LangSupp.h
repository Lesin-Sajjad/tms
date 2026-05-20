// LangSupp.h : main header file for the LANGSUPP application
//

#define VC_EXTRALEAN		// Exclude rarely-used stuff from Windows headers

#include <afxwin.h>         // MFC core and standard components
#include <afxext.h>         // MFC extensions
#include <afxdisp.h>        // MFC Automation classes
#include <afxdtctl.h>		// MFC support for Internet Explorer 4 Common Controls
#ifndef _AFX_NO_AFXCMN_SUPPORT
    #include <afxcmn.h>    // MFC support for Windows Common Controls
#endif

#include "resource.h"		// main symbols

/////////////////////////////////////////////////////////////////////////////
// CLangSuppApp:
// See LangSupp.cpp for the implementation of this class
//

class CLangSuppApp : public CWinApp
{
public:
	CLangSuppApp();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CLangSuppApp)
	public:
	virtual BOOL InitInstance();
	//}}AFX_VIRTUAL

// Implementation

	//{{AFX_MSG(CLangSuppApp)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////
// CLangSuppDlg dialog

class CLangSuppDlg : public CDialog
{
// Construction
public:
	CLangSuppDlg(CWnd* pParent = NULL);	// standard constructor


// Dialog Data
	//{{AFX_DATA(CLangSuppDlg)
	enum { IDD = IDD_LANGSUPP_DIALOG };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA

// Implementation
protected:
    short       m_shand;                    // The handle of the 1401 or -1 if not opened
    int         m_ictr;                     // The number of line of text shown in m_myListCtrl

    void        DisplayMessageString(CString cStr);
    CString     Get1401Type(int nType1401);
    bool        Open1401();
    bool        Load1401Commands();
    bool        ReadTestErr();
    bool        SendErrStr(short snum);
    bool        AdcMemTest();
    void        Close1401();


    CListCtrl m_myListCtrl;

	// Generated message map functions
	//{{AFX_MSG(CLangSuppDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnTest1401();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
