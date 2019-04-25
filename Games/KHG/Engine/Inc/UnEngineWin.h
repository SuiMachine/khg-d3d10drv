/*=============================================================================
	UnEngineWin.h: Unreal engine windows-specific code.
	Copyright 1997-1999 Epic Games, Inc. All Rights Reserved.

Revision history:
	* Created by Tim Sweeney.
=============================================================================*/

/*-----------------------------------------------------------------------------
	Exec hook.
-----------------------------------------------------------------------------*/

// FExecHook.
class FExecHook : public FExec, public FNotifyHook
{
private:
	WConfigProperties* Preferences;
	void NotifyDestroy( void* Src )
	{
		if( Src==Preferences )
			Preferences = NULL;
	}
	UBOOL Exec( const TCHAR* Cmd, FOutputDevice& Ar )
	{
		guard(FExecHook::Exec);
		if( ParseCommand(&Cmd,TEXT("ShowLog")) )
		{
			if( !GLog )
			{
				GLog = new WLog( TEXT("GameLog") );
				GLog->OpenWindow( 1, 0 );
				GLog->Log( NAME_Title, LocalizeGeneral("Start") );
				if( GIsClient )
					SetPropX( *GLog, TEXT("IsBrowser"), (HANDLE)1 );
			}
			return 1;
		}
		else if( ParseCommand(&Cmd,TEXT("TakeFocus")) )
		{
			TObjectIterator<UEngine> EngineIt;
			if
			(	EngineIt
			&&	EngineIt->Client
			&&	EngineIt->Client->Viewports.Num() )
				SetForegroundWindow( (HWND)EngineIt->Client->Viewports(0)->GetWindow() );
			return 1;
		}
		else if( ParseCommand(&Cmd,TEXT("EditActor")) )
		{
			//Fix added by Legend on 4/12/2000
			UClass* Class;
			FName ActorName;
			TObjectIterator<UEngine> EngineIt;

			AActor* Found = NULL;

			if( EngineIt && ParseObject<UClass>( Cmd, TEXT("Class="), Class, ANY_PACKAGE ) )
			{
				AActor* Player  = EngineIt->Client ? EngineIt->Client->Viewports(0)->Actor : NULL;
				FLOAT   MinDist = 999999.0;
				for( TObjectIterator<AActor> It; It; ++It )
				{
					FLOAT Dist = Player ? FDist(It->Location,Player->Location) : 0.0f;
					if
					(	(!Player || It->GetLevel()==Player->GetLevel())
					&&	(!It->bDeleteMe)
					&&	(It->IsA( Class) )
					&&	(Dist<MinDist) )
					{
						MinDist = Dist;
						Found   = *It;
					}
				}
			}
			else if( EngineIt && Parse( Cmd, TEXT("Name="), ActorName ) )
			{
				// look for actor by name
				for( TObjectIterator<AActor> It; It; ++It )
				{
					if( !It->bDeleteMe && It->GetName() == *ActorName )
					{
						Found = *It;
						break;
					}
				}
			}

			if( Found )
			{
				WObjectProperties* P = new WObjectProperties( TEXT("EditActor"), 0, TEXT(""), NULL, 1 );
				P->OpenWindow( (HWND)EngineIt->Client->Viewports(0)->GetWindow() );
				P->Root.SetObjects( (UObject**)&Found, 1 );
				//P->Show(1);
			}
			else Ar.Logf( TEXT("Bad or missing class or name") );
			return 1;
		}
		else if( ParseCommand(&Cmd,TEXT("HideLog")) )
		{
			if ( GLog )
			{
				RemovePropX( *GLog, TEXT("IsBrowser") );
				delete GLog;
				GLog = NULL;
			}
			return 1;
		}
		else if( ParseCommand(&Cmd,TEXT("Preferences")) && !GIsClient )
		{
			if( !Preferences )
			{
				Preferences = new WConfigProperties( TEXT("Preferences"), LocalizeGeneral("AdvancedOptionsTitle",TEXT("Window")) );
				Preferences->SetNotifyHook( this );
				Preferences->OpenWindow( GLog ? GLog->hWnd : NULL );
				Preferences->ForceRefresh();
			}
			SetFocus( *Preferences );
			return 1;
		}
		else return 0;
		unguard;
	}
public:
	FExecHook()
	: Preferences( NULL )
	{}
};

/*-----------------------------------------------------------------------------
	Startup and shutdown.
-----------------------------------------------------------------------------*/

//
// Initialize.
//
static UEngine* InitEngine()
{
	guard(InitEngine);
	DOUBLE LoadTime = appSeconds();

	// Set exec hook.
	static FExecHook GLocalHook;
	GExec = &GLocalHook;

	// Create mutex so installer knows we're running.
	CreateMutexX( NULL, 0, TEXT("UnrealIsRunning") );

	// First-run menu.
	INT FirstRun=0;
	GetConfigInt( TEXT("FirstRun"), TEXT("FirstRun"), FirstRun );
	if( ParseParam(appCmdLine(),TEXT("FirstRun")) )
		FirstRun=0;

	// First-run and version upgrading code.
	if( FirstRun<ENGINE_VERSION && !GIsEditor && GIsClient )
	{
		// Get system directory.
		TCHAR SysDir[256]=TEXT(""), WinDir[256]=TEXT("");
#if UNICODE
		if( !GUnicodeOS )
		{
			ANSICHAR ASysDir[256]="", AWinDir[256]="";
			GetSystemDirectoryA( ASysDir, ARRAY_COUNT(SysDir) );
			GetWindowsDirectoryA( AWinDir, ARRAY_COUNT(WinDir) );
			appStrcpy( SysDir, ANSI_TO_TCHAR(ASysDir) );
			appStrcpy( WinDir, ANSI_TO_TCHAR(AWinDir) );
		}
		else
#endif
		{
			GetSystemDirectory( SysDir, ARRAY_COUNT(SysDir) );
			GetWindowsDirectory( WinDir, ARRAY_COUNT(WinDir) );
		}

#if 0
		if( FirstRun<219 )
		{
			// Autodetect and ask about detected render devices.
			TArray<FRegistryObjectInfo> RenderDevices;
			UObject::GetRegistryObjects( RenderDevices, UClass::StaticClass(), URenderDevice::StaticClass(), 0 );
			for( INT i=0; i<RenderDevices.Num(); i++ )
			{
				TCHAR File1[256], File2[256];
				appSprintf( File1, TEXT("%s\\%s"), SysDir, *RenderDevices(i).Autodetect );
				appSprintf( File2, TEXT("%s\\%s"), WinDir, *RenderDevices(i).Autodetect );
				if( RenderDevices(i).Autodetect!=TEXT("") && (GFileManager->FileSize(File1)>=0 || GFileManager->FileSize(File2)>=0) )
				{
					TCHAR Path[256], *Str;
					appStrcpy( Path, *RenderDevices(i).Object );
					Str = appStrstr(Path,TEXT("."));
					if( Str )
					{
						*Str++ = 0;
						if( ::MessageBox( NULL, Localize(Str,TEXT("AskInstalled"),Path), Localize(TEXT("FirstRun"),TEXT("Caption"),TEXT("Window")), MB_YESNO|MB_ICONQUESTION|MB_TASKMODAL )==IDYES )
						{
							if( ::MessageBox( NULL, Localize(Str,TEXT("AskUse"),Path), Localize(TEXT("FirstRun"),TEXT("Caption"),TEXT("Window")), MB_YESNO|MB_ICONQUESTION|MB_TASKMODAL )==IDYES )
							{
								SetConfigString( TEXT("Engine.Engine"), TEXT("GameRenderDevice"), *RenderDevices(i).Object );
								SetConfigString( TEXT("SoftDrv.SoftwareRenderDevice"), TEXT("HighDetailActors"), TEXT("True") );
								break;
							}
						}
					}
				}
			}
		}
#endif
		if( FirstRun<ENGINE_VERSION )
		{
			// Initiate first run of this version.
			FirstRun = ENGINE_VERSION;
			SetConfigInt( TEXT("FirstRun"), TEXT("FirstRun"), FirstRun );
			::MessageBox( NULL, Localize(TEXT("FirstRun"),TEXT("Starting"),TEXT("Window")), Localize(TEXT("FirstRun"),TEXT("Caption"),TEXT("Window")), MB_OK|MB_ICONINFORMATION|MB_TASKMODAL );
		}
	}

	// Create the global engine object.
	UClass* EngineClass;
	if( !GIsEditor )
	{
		// Create game engine.
		EngineClass = UObject::StaticLoadClass( UGameEngine::StaticClass(), NULL, TEXT("ini:Engine.Engine.GameEngine"), NULL, LOAD_NoFail, NULL );
	}
	else
	{
		// Editor.
		EngineClass = UObject::StaticLoadClass( UEngine::StaticClass(), NULL, TEXT("ini:Engine.Engine.EditorEngine"), NULL, LOAD_NoFail, NULL );
	}
	UEngine* Engine = ConstructObject<UEngine>( EngineClass );
	Engine->Init();
	debugf( TEXT("Startup time: %f seconds"), appSeconds()-LoadTime );

	return Engine;
	unguard;
}

//
// Unreal's main message loop.  All windows in Unreal receive messages
// somewhere below this function on the stack.
//
static void MainLoop( UEngine* Engine )
{
	guard(MainLoop);
	check(Engine);

	// Enter main loop.
	guard(EnterMainLoop);
	if( GLog )
		GLog->SetExec( Engine );
	unguard;

	// Loop while running.
	GIsRunning = 1;
	DWORD ThreadId = GetCurrentThreadId();
	HANDLE hThread = GetCurrentThread();
	DOUBLE OldTime = appSeconds();
	while( GIsRunning && !GIsRequestingExit )
	{
		// Update the world.
		guard(UpdateWorld);
		DOUBLE NewTime   = appSeconds();
		FLOAT  DeltaTime = NewTime - OldTime;
		Engine->Tick( DeltaTime );
		if( GWindowManager )
			GWindowManager->Tick( DeltaTime );
		OldTime = NewTime;
		unguard;

		// Enforce optional maximum tick rate.
		guard(EnforceTickRate);
#if 1
		//FLOAT MaxTickRate = Engine->GetMaxTickRate();
		FLOAT MaxTickRate = 60.;
		FLOAT Delta;
		while ( (Delta = (1.0/MaxTickRate) - (appSeconds()-OldTime)) > 0.0 );
#else
		FLOAT MaxTickRate = Engine->GetMaxTickRate();
		if( MaxTickRate>0.0 )
		{
			FLOAT Delta = (1.0/MaxTickRate) - (appSeconds()-OldTime);
			appSleep( Max(0.f,Delta) );
		}
#endif
		unguard;

		// Handle all incoming messages.
		guard(MessagePump);
		MSG Msg;
		//GTempDouble=0;
		while( PeekMessageX(&Msg,NULL,0,0,PM_REMOVE) )
		{
			if( Msg.message == WM_QUIT )
				GIsRequestingExit = 1;

			guard(TranslateMessage);
			TranslateMessage( &Msg );
			unguardf(( TEXT("%08X %i"), (INT)Msg.hwnd, Msg.message ));

			guard(DispatchMessage);
			DispatchMessageX( &Msg );
			unguardf(( TEXT("%08X %i"), (INT)Msg.hwnd, Msg.message ));
		}
		unguard;

		// If editor thread doesn't have the focus, don't suck up too much CPU time.
		if( GIsEditor )
		{
			guard(ThrottleEditor);
			static UBOOL HadFocus=1;
			UBOOL HasFocus = (GetWindowThreadProcessId(GetForegroundWindow(),NULL) == ThreadId );
			if( HadFocus && !HasFocus )
			{
				// Drop our priority to speed up whatever is in the foreground.
				SetThreadPriority( hThread, THREAD_PRIORITY_BELOW_NORMAL );
			}
			else if( HasFocus && !HadFocus )
			{
				// Boost our priority back to normal.
				SetThreadPriority( hThread, THREAD_PRIORITY_NORMAL );
			}
			if( !HasFocus )
			{
				// Surrender the rest of this timeslice.
				Sleep(0);
			}
			HadFocus = HasFocus;
			unguard;
		}
	}
	GIsRunning = 0;

	// Exit main loop.
	guard(ExitMainLoop);
	if( GLog )
		GLog->SetExec( NULL );
	GExec = NULL;
	unguard;

	unguard;
}

/*-----------------------------------------------------------------------------
	Splash screen.
-----------------------------------------------------------------------------*/

//
// Splash screen, implemented with old-style Windows code so that it
// can be opened super-fast before initialization.
//
#if 0
BOOL CALLBACK SplashDialogProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
	return uMsg==WM_INITDIALOG;
}
HWND hWndSplash = NULL;
void InitSplash( HINSTANCE hInstance, UBOOL Show, const TCHAR* Filename )
{
	if( Show )
	{
		hWndSplash = TCHAR_CALL_OS(CreateDialogW(hInstance, MAKEINTRESOURCEW(IDDIALOG_Splash), NULL, SplashDialogProc),CreateDialogA(hInstance, MAKEINTRESOURCEA(IDDIALOG_Splash), NULL, SplashDialogProc) );
		check(hWndSplash);
		FWindowsBitmap Bitmap;
		verify(Bitmap.LoadFile( Filename ) );
		INT screenWidth  = GetSystemMetrics( SM_CXSCREEN );
		INT screenHeight = GetSystemMetrics( SM_CYSCREEN );
		HWND hWndLogo = GetDlgItem(hWndSplash,IDC_Logo);
		check(hWndLogo);
		ShowWindow( hWndSplash, SW_SHOW );
		SendMessageX( hWndLogo, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)Bitmap.GetBitmapHandle() );
		SetWindowPos( hWndSplash, NULL, (screenWidth - Bitmap.SizeX)/2, (screenHeight - Bitmap.SizeY)/2, Bitmap.SizeX, Bitmap.SizeY, 0 );
		UpdateWindow( hWndSplash );
	}
}
void ExitSplash()
{
	if( hWndSplash )
		DestroyWindow( hWndSplash );
}
#else
BOOL CALLBACK SplashDialogProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
	if( uMsg==WM_DESTROY )
		PostQuitMessage(0);
	return 0;
}
HWND    hWndSplash = NULL;
HBITMAP hBitmap    = NULL;
INT     BitmapX    = 0;
INT     BitmapY    = 0;
DWORD   ThreadId   = 0;
HANDLE  hThread    = 0;
DWORD WINAPI ThreadProc( VOID* Parm )
{
	hWndSplash = TCHAR_CALL_OS(CreateDialogW(hInstance,MAKEINTRESOURCEW(IDDIALOG_Splash), NULL, SplashDialogProc),CreateDialogA(hInstance, MAKEINTRESOURCEA(IDDIALOG_Splash), NULL, SplashDialogProc) );
	if( hWndSplash )
	{
		HWND hWndLogo = GetDlgItem(hWndSplash,IDC_Logo);
		if( hWndLogo )
		{
			SetWindowPos(hWndSplash,HWND_TOPMOST,(GetSystemMetrics(SM_CXSCREEN)-BitmapX)/2,(GetSystemMetrics(SM_CYSCREEN)-BitmapY)/2,BitmapX,BitmapY,SWP_SHOWWINDOW);
			SetWindowPos(hWndSplash,HWND_NOTOPMOST,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE);
			SendMessageX( hWndLogo, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)hBitmap );
			UpdateWindow( hWndSplash );
			MSG Msg;
			while( TCHAR_CALL_OS(GetMessageW(&Msg,NULL,0,0),GetMessageA(&Msg,NULL,0,0)) )
				DispatchMessageX(&Msg);
		}
	}
	return 0;
}
void InitSplash( const TCHAR* Filename )
{
	FWindowsBitmap Bitmap(1);
	if( Filename )
	{
		verify(Bitmap.LoadFile(Filename) );
		hBitmap = Bitmap.GetBitmapHandle();
		BitmapX = Bitmap.SizeX;
		BitmapY = Bitmap.SizeY;
	}
	hThread=CreateThread(NULL,0,&ThreadProc,NULL,0,&ThreadId);
}
void ExitSplash()
{
	if( ThreadId )
		TCHAR_CALL_OS(PostThreadMessageW(ThreadId,WM_QUIT,0,0),PostThreadMessageA(ThreadId,WM_QUIT,0,0));
}
#endif

/*-----------------------------------------------------------------------------
	The End.
-----------------------------------------------------------------------------*/
