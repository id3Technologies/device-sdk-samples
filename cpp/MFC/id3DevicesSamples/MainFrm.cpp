// MainFrm.cpp : implementation of the CMainFrame class
//

#include "pch.h"
#include "framework.h"
#include "id3DevicesSamples.h"

#include "MainFrm.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// CMainFrame

IMPLEMENT_DYNAMIC(CMainFrame, CFrameWnd)

BEGIN_MESSAGE_MAP(CMainFrame, CFrameWnd)
    ON_WM_CREATE()
    ON_WM_SETFOCUS()
    ON_WM_CLOSE()
END_MESSAGE_MAP()

static UINT indicators[] =
{
    ID_INDICATOR_TEXT,
    ID_SEPARATOR,           // status line indicator
    ID_INDICATOR_CAPS
};

// construction/destruction of CMainFrame

CMainFrame::CMainFrame() noexcept
{
    // TODO: add member initialization code here
}

CMainFrame::~CMainFrame()
{
}

int CMainFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
    if (CFrameWnd::OnCreate(lpCreateStruct) == -1)
        return -1;

    // create a view so that it occupies the frame's client area
    if (!m_wndView.Create(nullptr, nullptr, AFX_WS_DEFAULT_VIEW, CRect(0, 0, 0, 0), this, AFX_IDW_PANE_FIRST, nullptr))
    {
        TRACE0("Unable to create the view window\n");
        return -1;
    }

    if (!m_wndStatusBar.Create(this))
    {
        TRACE0("Unable to create the status bar\n");
        return -1;      // creation failed
    }
    m_wndStatusBar.SetIndicators(indicators, sizeof(indicators) / sizeof(UINT));
    m_wndStatusBar.SetPaneInfo(0, ID_INDICATOR_TEXT, SBPS_STRETCH, 0);

    m_wndView.Initialize(&m_wndStatusBar);

    return 0;
}

BOOL CMainFrame::PreCreateWindow(CREATESTRUCT& cs)
{
    if (!CFrameWnd::PreCreateWindow(cs))
        return FALSE;
    // TODO: modify the Window class or styles here by modifying
    //  the CREATESTRUCT cs

    cs.dwExStyle &= ~WS_EX_CLIENTEDGE;
    cs.lpszClass = AfxRegisterWndClass(0);
    return TRUE;
}

void CMainFrame::OnClose()
{
    m_wndView.SendMessage(WM_CLOSE);
    CFrameWnd::OnClose();
}

void CMainFrame::DisposeCapture()
{
    m_wndView.Dispose();
}

// CMainFrame diagnostics

#ifdef _DEBUG
void CMainFrame::AssertValid() const
{
    CFrameWnd::AssertValid();
}

void CMainFrame::Dump(CDumpContext& dc) const
{
    CFrameWnd::Dump(dc);
}
#endif //_DEBUG


// CMainFrame message handlers

void CMainFrame::OnSetFocus(CWnd* /*pOldWnd*/)
{
    // set focus to the view window
    m_wndView.SetFocus();
}

BOOL CMainFrame::OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo)
{
    // give the view first chance to handle the command
    if (m_wndView.OnCmdMsg(nID, nCode, pExtra, pHandlerInfo))
        return TRUE;

    // otherwise, use the default handling
    return CFrameWnd::OnCmdMsg(nID, nCode, pExtra, pHandlerInfo);
}
