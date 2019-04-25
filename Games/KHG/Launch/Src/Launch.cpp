/*=============================================================================
	Launch.cpp: Game launcher.
	Copyright 1997-1999 Epic Games, Inc. All Rights Reserved.

Revision history:
	* Created by Tim Sweeney.
=============================================================================*/

#include "LaunchPrivate.h"
#include "UnEngineWin.h"

/*-----------------------------------------------------------------------------
	Global variables.
-----------------------------------------------------------------------------*/

// General.
extern "C" {HINSTANCE hInstance;}
extern "C" {TCHAR GPackage[64]=TEXT("Launch");}

// Log file.
#include "FOutputDeviceFile.h"
FOutputDeviceFile Log;

// Error handler.
#include "FOutputDeviceWindowsError.h"
FOutputDeviceWindowsError Error;

// Feedback.
//#include "FFeedbackContextWindows.h"
//FFeedbackContextWindows Warn;

#if 0
// File manager.
#include "FFileManagerWindows.h"
FFileManagerWindows FileManager;
#endif

// Config.
#include "FConfigCacheIni.h"

/*-----------------------------------------------------------------------------
	WinMain.
-----------------------------------------------------------------------------*/

//
// Main entry point.
// This is an example of how to initialize and launch the engine.
//
INT WINAPI WinMain( HINSTANCE hInInstance, HINSTANCE hPrevInstance, char*, INT nCmdShow )
{
	// Remember instance.
	INT ErrorLevel = 0;
	GIsStarted     = 1;
	hInstance      = hInInstance;
	const TCHAR* CmdLine = GetCommandLine();
	appStrcpy( GPackage, appPackage() );

	// See if this should be passed to another instances.
	if( !appStrfind(CmdLine,TEXT("Server")) && !appStrfind(CmdLine,TEXT("NewWindow")) && !appStrfind(CmdLine,TEXT("changevideo")) && !appStrfind(CmdLine,TEXT("TestRenDev")) )
	{
		TCHAR ClassName[256];
		MakeWindowClassName(ClassName,TEXT("WLog"));
		for( HWND hWnd=NULL; ; )
		{
			hWnd = TCHAR_CALL_OS(FindWindowExW(hWnd,NULL,ClassName,NULL),FindWindowExA(hWnd,NULL,TCHAR_TO_ANSI(ClassName),NULL));
			if( !hWnd )
				break;
			if( GetPropX(hWnd,TEXT("IsBrowser")) )
			{
				while( *CmdLine && *CmdLine!=' ' )
					CmdLine++;
				if( *CmdLine==' ' )
					CmdLine++;
				COPYDATASTRUCT CD;
				DWORD Result;
				CD.dwData = WindowMessageOpen;
				CD.cbData = (appStrlen(CmdLine)+1)*sizeof(TCHAR*);
				CD.lpData = const_cast<TCHAR*>( CmdLine );
				SendMessageTimeout( hWnd, WM_COPYDATA, (WPARAM)NULL, (LPARAM)&CD, SMTO_ABORTIFHUNG|SMTO_BLOCK, 30000, &Result );
				GIsStarted = 0;
				return 0;
			}
		}
	}

	// Begin guarded code.
#ifndef _DEBUG
	try
	{
#endif
		// Init core.
		GIsClient = GIsGuarded = 1;
		//appInit( GPackage, CmdLine, &Malloc, &Log, &Error, &Warn, &FileManager, FConfigCacheIni::Factory, 1 );
		appInit();
		//if( ParseParam(appCmdLine(),TEXT("MAKE")) )//oldver
			//appErrorf( TEXT("'Unreal -make' is obsolete, use 'ucc make' now") );

		// Init mode.
		GIsServer     = 1;
		GIsClient     = !ParseParam(appCmdLine(),TEXT("SERVER")) && !ParseParam(appCmdLine(),TEXT("MAKE"));
		GIsEditor     = ParseParam(appCmdLine(),TEXT("EDITOR")) || ParseParam(appCmdLine(),TEXT("MAKE"));
		GLazyLoad     = GIsEditor || !GIsClient || ParseParam(appCmdLine(),TEXT("LAZY"));

		// Figure out whether to show log or splash screen.
		UBOOL ShowLog = ParseParam(CmdLine,TEXT("LOG"));
		// Note: Splash Screen is loaded out of Resource file in Khg 1.1
		FString Filename = FString( TEXT("..\\Help\\") ) + GPackage + TEXT("Logo.bmp");
		if( appFSize(*Filename)<0 )
			Filename = TEXT("..\\Help\\Logo.bmp");
		appStrcpy( GPackage, appPackage() );
		//InitSplash( hInstance, !ShowLog && !ParseParam(CmdLine,TEXT("server")), *Filename );
		if ( !ShowLog && !ParseParam(CmdLine,TEXT("server")) && !appStrfind(CmdLine,TEXT("TestRenDev")) )
			InitSplash( *Filename );

		// Init windowing.
		InitWindowing();

		// Create log window, but only show it if ShowLog.
		if ( ShowLog )
		{
			GLog = new WLog( TEXT("GameLog") );
			GLog->OpenWindow( ShowLog, 0 );
			GLog->Log( NAME_Title, LocalizeGeneral("Start") );
			if( GIsClient )
				SetPropX( *GLog, TEXT("IsBrowser"), (HANDLE)1 );
		}
		else
		{
			GLog = NULL;
		}

		// Init engine.
		UEngine* Engine = InitEngine();
		if ( Engine )
		{
			if ( GLog )
				GLog->Log( NAME_Title, LocalizeGeneral("Run") );

			// Hide splash screen.
			ExitSplash();

			// Optionally Exec an exec file
			TCHAR Temp[4096] = TEXT("exec "); //!!
			if( Parse(CmdLine, TEXT("EXEC="), Temp + appStrlen(Temp), 4096 ) )
			{
				if( Engine->Client && Engine->Client->Viewports.Num() && Engine->Client->Viewports(0) )
					Engine->Client->Viewports(0)->Exec( Temp, *GLog );
			}

			// Start main engine loop, including the Windows message pump.
			if( !GIsRequestingExit )
				MainLoop( Engine );
		}

		// Delete is-running semaphore file.
		appUnlink( TEXT("Running.ini") );

		// Clean shutdown.
		if ( GLog )
		{
			RemovePropX( *GLog, TEXT("IsBrowser") );
			GLog->Log( NAME_Title, LocalizeGeneral("Exit") );
			delete GLog;
		}
		appPreExit();
		GIsGuarded = 0;
#ifndef _DEBUG
	}
	catch( ... )
	{
		// Crashed.
		ErrorLevel = 1;
		appHandleError();
	}
#endif

	// Final shut down.
	appExit();
	GIsStarted = 0;
	return ErrorLevel;
}

/*-----------------------------------------------------------------------------
	The End.
-----------------------------------------------------------------------------*/
