// LangSupp.cpp : Defines the class behaviors for the application.
//

#include "LangSupp.h"
#include <use1401.h>                           // 1401 access functions


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define NUMSAMP 2000                        // Number of ADC samples to take

/////////////////////////////////////////////////////////////////////////////
// CLangSuppApp

BEGIN_MESSAGE_MAP(CLangSuppApp, CWinApp)
	//{{AFX_MSG_MAP(CLangSuppApp)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG
	ON_COMMAND(ID_HELP, CWinApp::OnHelp)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CLangSuppApp construction

CLangSuppApp::CLangSuppApp()
{
}

/////////////////////////////////////////////////////////////////////////////
// The one and only CLangSuppApp object

CLangSuppApp theApp;

/////////////////////////////////////////////////////////////////////////////
// CLangSuppApp initialization

BOOL CLangSuppApp::InitInstance()
{
	CLangSuppDlg dlg;           // The dialog that does everything
	m_pMainWnd = &dlg;          // This is needed to make a dialog-only application work...
	dlg.DoModal();
	return FALSE;
}

/////////////////////////////////////////////////////////////////////////////
//
// CLangSuppDlg dialog

CLangSuppDlg::CLangSuppDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CLangSuppDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CLangSuppDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
	// Note that LoadIcon does not require a subsequent DestroyIcon in Win32
}

BEGIN_MESSAGE_MAP(CLangSuppDlg, CDialog)
	//{{AFX_MSG_MAP(CLangSuppDlg)
	ON_BN_CLICKED(IDC_TEST_1401, OnTest1401)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
//
// CLangSuppDlg message handlers
//
/////////////////////////////////////////////////////////////////////////////
//
BOOL CLangSuppDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

    m_myListCtrl.SubclassDlgItem(IDC_LIST_TEXT, this);
    m_myListCtrl.DeleteAllItems();                              // Clear the list out
	
	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
//
// Adds string to the list box control - rather complicated.
//
void CLangSuppDlg::DisplayMessageString(CString cStr)
{
    LVITEM lvItem;                                       // structure that specifies or receives the attributes of a list view item
    memset(&lvItem, 0, sizeof(lvItem));                  // make sure structure all set
    lvItem.mask      = LVIF_PARAM|LVIF_TEXT|LVIF_STATE;  // we pass data and text
    lvItem.stateMask = LVIS_SELECTED|LVIS_FOCUSED;
    lvItem.pszText   = cStr.GetBuffer(0);
    lvItem.iItem     = m_ictr++;
    int index = m_myListCtrl.InsertItem(&lvItem);
}

/////////////////////////////////////////////////////////////////////////////
//
CString CLangSuppDlg::Get1401Type(int nType1401)
{
    switch (nType1401)
    {
        case U14TYPE1401   : return "Standard 1401";
                             break;
        case U14TYPEPLUS   : return "1401plus";
                             break;
        case U14TYPEU1401  : return "u1401";
                             break;
        case U14TYPEPOWER  : return "Power1401";
                             break;
        case U14TYPEU14012 : return "Micro1401 mk II";
                             break;
        case U14TYPEPOWER2 : return "Power1401 mk II";
                             break;
        default :            return "Unknown";
   }
}

/////////////////////////////////////////////////////////////////////////////
//
//  Tries to get the 1401 for use by this application 
//  Returns true is successful or false if failed
//
bool CLangSuppDlg::Open1401()
{
    m_shand = U14Open1401(0);				                                    // Try to open 1401 and save error codes

    if (m_shand < 0)
    {
        CString   cstr;

        cstr.Format("Error opening 1401! Error %i", m_shand);
        DisplayMessageString(cstr);
        return false;		                                                    // fail to open 1401
    }
    else
        DisplayMessageString("Successful opening of "+ Get1401Type(U14TypeOf1401(m_shand)));
    return true;
}

/////////////////////////////////////////////////////////////////////////////
//
//  Loads a command into the 1401
//  Returns true is successful or false if failed
//
bool CLangSuppDlg::Load1401Commands()
{
    CString     scmd = "kill,adcmem";
    CString     cstr;
    DWORD       lRetVal  = U14Ld(m_shand, "", scmd);

    short       s1401Err = (short)(lRetVal & 0xFFFF);
    if (s1401Err != U14ERR_NOERROR)
    {
		cstr.Format("Load commands failed! Error %i",s1401Err);
        DisplayMessageString(cstr);
        return false;
    }
    else
    {
        cstr.Format("Successful loading of 1401 commands : %s ", scmd);
        DisplayMessageString(cstr);
    }
    return true;
}

/////////////////////////////////////////////////////////////////////////////
//
//  Retrieves numbers from the 1401 converted to longs
//  Returns false if failed or true if successful
//           
bool CLangSuppDlg::ReadTestErr()
{
    CString     cstr;
    long        lNums[10];                                              // buffer to read values
     
    memset(lNums, 0, sizeof(lNums));

    short s1401Err = U14LongsFrom1401(m_shand, lNums, 2);               // gets values from 1401
    
    if (s1401Err != U14ERR_NOERROR)
    {
        if ((lNums[0] != 0) || (lNums[1] != 0))                     
        {
            cstr.Format("Error numbers from 1401 Error e0 = %ld, e1 = %ld.", lNums[0], lNums[1]);
            DisplayMessageString(cstr);
            return false;
        }
        else
        {
	        cstr.Format("Numbers received from 1401. Value e0 = %ld, e1 = %ld.", lNums[0], lNums[1]);
            DisplayMessageString(cstr);
        }
    }
    else
    {
	    cstr.Format("Failure getting numbers from 1401. Error %i",s1401Err);
        DisplayMessageString(cstr);
        return false;
    }
    return true;
}

/////////////////////////////////////////////////////////////////////////////
//
//  Sends the ERR command to the 1401 and gets the result.
//  Returns false if failed or true if successf
//
bool CLangSuppDlg::SendErrStr(short snum)
{
    CString     cstr;
    short       s1401Err = U14SendString(m_shand, ";;ERR;");            // send "ERR" string to the 1401

    if (s1401Err != U14ERR_NOERROR)
    {
        cstr.Format("Sent ERR string! Error %i",s1401Err);
        DisplayMessageString(cstr);
        return false;
    }
    else
    {
        DisplayMessageString("Successfully sent ERR string");
        if (!ReadTestErr())                                          // get values from 1401
            return false;
    }
    return true;
}

/////////////////////////////////////////////////////////////////////////////
//
//  Try to close 1401 and display error codes

void CLangSuppDlg::Close1401()
{
    short   s1401Err = U14Close1401(m_shand);

    if (s1401Err < U14ERR_NOERROR)
    {
        CString cstr;
        cstr.Format("Failure closing 1401! Error %d", s1401Err);
        DisplayMessageString(cstr);
    }
    else
        DisplayMessageString("1401 closed successfully");
}

/////////////////////////////////////////////////////////////////////////////
//
// Example of data capture
//
bool CLangSuppDlg::AdcMemTest()
{
    CString cstr;

    cstr.Format("Successful transfer area setting");
    DisplayMessageString(cstr);
                                                //pre and count 10,10 give 10000 Hz sampling rate
    cstr.Format("ADCMEM,I,2,0,%d,0 1,1,C,10,10;", NUMSAMP*2);
    CString cst = cstr;                         //preserving this string

    short s1401Err = U14SendString(m_shand, cstr);
    if (s1401Err != U14ERR_NOERROR)
    {
        cstr.Format("Error sending %s string. Error %i", cst, s1401Err);
        DisplayMessageString(cstr);
        return false;
    }
    else
    {
        cstr.Format("Successfully sent %s string ", cst);
        DisplayMessageString(cstr);

        if (SendErrStr(2))                                                      // send ERR and get back 2 numbers
        {
            long lnum = 0;

            do
            {
                cstr.Format("%s", "ADCMEM,?;");
                cst = cstr;
                s1401Err = U14SendString(m_shand, cstr);                       // get status
            
                if (s1401Err != U14ERR_NOERROR)
                {
                    cstr.Format("Error sending %s string. Error %i", cst, s1401Err);
                    DisplayMessageString(cstr);
                    return false;
                }
                else
                     s1401Err = U14LongsFrom1401(m_shand, &lnum, 1);         // gets the value from the 1401
            }while ((lnum != 0) && (lnum != -1));                            // until finished well or badly
        
            if (lnum == 0)
                DisplayMessageString("Job done required number of points captured");
            else
                DisplayMessageString("Job done but some samples missed");
        }
       
    }
    short sbuf[NUMSAMP];                                                   // A buffer to read back data
    s1401Err = U14ToHost(m_shand, (char*)sbuf, sizeof(short)*NUMSAMP, 0, 0);     // Read back data from the 1401
    if (s1401Err != U14ERR_NOERROR)
    {
        cstr.Format("To host block transfer %d failed",s1401Err);
        DisplayMessageString(cstr);
        return false;
    }
    else
        DisplayMessageString("Job done required number of points transfered");

    return true;
}

/////////////////////////////////////////////////////////////////////////////
//
void CLangSuppDlg::OnTest1401()                                 // the 1401 main test 
{
    m_ictr = 0;                                                 // reset counter
    m_myListCtrl.DeleteAllItems();                              // Delete all of the items from the list view control.

    if (!Open1401())                                            // Try to open 1401 and save error codes
        return;

    if (Load1401Commands())                                     // try to load 1401 selected commands 
        if (SendErrStr(2))                                      // send ERR string and read back 2 numbers
            AdcMemTest();                                       // ADCMEM data capture
    Close1401();                                                // Always close the 1401 after using it
}

/////////////////////////////////////////////////////////////////////////////
//