
// id3CaptureSamples.h : fichier d'en-tête principal de l'application id3CaptureSamples
//
#pragma once

#ifndef __AFXWIN_H__
	#error "include 'pch.h' before including this file for PCH"
#endif

#include "resource.h"       // symboles principaux


// id3CaptureSamplesApp :
// Consultez id3CaptureSamples.cpp pour l'implémentation de cette classe
//

class id3DevicesSamplesApp : public CWinApp
{
public:
	id3DevicesSamplesApp() noexcept;


// Substitutions
public:
	virtual BOOL InitInstance();
	virtual int ExitInstance();

// Implémentation

public:
	afx_msg void OnAppAbout();
	DECLARE_MESSAGE_MAP()
};

extern id3DevicesSamplesApp theApp;
