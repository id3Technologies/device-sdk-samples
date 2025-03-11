
// id3CaptureSamples.cpp : définit les comportements de classe de l'application.
//

#include "pch.h"
#include "framework.h"
#include "afxwinappex.h"
#include "afxdialogex.h"
#include "id3DevicesSamples.h"
#include "MainFrm.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// id3CaptureSamplesApp

BEGIN_MESSAGE_MAP(id3DevicesSamplesApp, CWinApp)
	ON_COMMAND(ID_APP_ABOUT, &id3DevicesSamplesApp::OnAppAbout)
    ON_COMMAND(ID_FILE_OPEN, &id3DevicesSamplesApp::OnFileOpen)
END_MESSAGE_MAP()


// Construction de id3CaptureSamplesApp

id3DevicesSamplesApp::id3DevicesSamplesApp() noexcept
{
	// TODO: remplacer la chaîne d'ID de l'application ci-dessous par une chaîne ID unique ; le format recommandé
	// pour la chaîne est CompanyName.ProductName.SubProduct.VersionInformation
	SetAppID(_T("id3.Devices.Samples.AppID.NoVersion"));
}

// Le seul et unique objet id3CaptureSamplesApp

id3DevicesSamplesApp theApp;


// Initialisation de id3CaptureSamplesApp

BOOL id3DevicesSamplesApp::InitInstance()
{
	CWinApp::InitInstance();


	EnableTaskbarInteraction(FALSE);

	// AfxInitRichEdit2() est obligatoire pour utiliser un contrôle RichEdit
	// AfxInitRichEdit2();

	// Initialisation standard
	// Si vous n'utilisez pas ces fonctionnalités et que vous souhaitez réduire la taille
	// de votre exécutable final, vous devez supprimer ci-dessous
	// les routines d'initialisation spécifiques dont vous n'avez pas besoin.
	// Changez la clé de Registre sous laquelle nos paramètres sont enregistrés
	// TODO: modifiez cette chaîne avec des informations appropriées,
	// telles que le nom de votre société ou organisation
	SetRegistryKey(_T("Applications locales générées par AppWizard"));

	// Pour créer la fenêtre principale, ce code crée un nouvel objet fenêtre frame
	// qu'il définit ensuite en tant qu'objet fenêtre principale de l'application
	CFrameWnd* pFrame = new CMainFrame;
	if (!pFrame)
		return FALSE;
	m_pMainWnd = pFrame;
	// crée et charge le frame avec ses ressources
	pFrame->LoadFrame(IDR_MAINFRAME,
		WS_OVERLAPPEDWINDOW | FWS_ADDTOTITLE, nullptr,
		nullptr);

	// La seule fenêtre a été initialisée et peut donc être affichée et mise à jour
	pFrame->ShowWindow(SW_SHOW);
	pFrame->UpdateWindow();
	return TRUE;
}

int id3DevicesSamplesApp::ExitInstance()
{
	//TODO: gérez les ressources supplémentaires que vous avez ajoutées
    ((CMainFrame*)m_pMainWnd)->DisposeCapture();
	return CWinApp::ExitInstance();
}

// gestionnaires de messages de id3CaptureSamplesApp


// boîte de dialogue CAboutDlg utilisée pour la boîte de dialogue 'À propos de' pour votre application

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg() noexcept;

// Données de boîte de dialogue
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ABOUTBOX };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // Prise en charge de DDX/DDV

// Implémentation
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() noexcept : CDialogEx(IDD_ABOUTBOX)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()

// Commande App pour exécuter la boîte de dialogue
void id3DevicesSamplesApp::OnAppAbout()
{
	CAboutDlg aboutDlg;
	aboutDlg.DoModal();
}



