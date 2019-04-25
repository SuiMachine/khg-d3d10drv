/*=============================================================================
	Window.h: GUI window management code.
	Copyright 1997-1999 Epic Games, Inc. All Rights Reserved.

	Revision history:
		* Created by Tim Sweeney
=============================================================================*/

#include "..\Src\Res\WindowRes.h"

/*-----------------------------------------------------------------------------
	Defines.
-----------------------------------------------------------------------------*/

#ifndef WINDOW_API
#define WINDOW_API __declspec(dllimport)
#endif

#define WIN_OBJ 0

/*-----------------------------------------------------------------------------
	Unicode support.
-----------------------------------------------------------------------------*/

#define RegSetValueExX(a,b,c,d,e,f)		TCHAR_CALL_OS(RegSetValueExW(a,b,c,d,e,f),RegSetValueExA(a,TCHAR_TO_ANSI(b),c,d,(BYTE*)TCHAR_TO_ANSI((TCHAR*)e),f))
#define RegSetValueX(a,b,c,d,e)			TCHAR_CALL_OS(RegSetValueW(a,b,c,d,e),RegSetValueA(a,TCHAR_TO_ANSI(b),c,TCHAR_TO_ANSI(d),e))
#define RegCreateKeyX(a,b,c)			TCHAR_CALL_OS(RegCreateKeyW(a,b,c),RegCreateKeyA(a,TCHAR_TO_ANSI(b),c))
#define RegQueryValueX(a,b,c,d)			TCHAR_CALL_OS(RegQueryValueW(a,b,c,d),RegQueryValueW(a,TCHAR_TO_ANSI(b),TCHAR_TO_ANSI(c),d))
#define RegOpenKeyX(a,b,c)				TCHAR_CALL_OS(RegOpenKeyW(a,b,c),RegOpenKeyA(a,TCHAR_TO_ANSI(b),c))
#define RegDeleteKeyX(a,b)				TCHAR_CALL_OS(RegDeleteKeyW(a,b),RegDeleteKeyA(a,TCHAR_TO_ANSI(b)))
#define RegDeleteValueX(a,b)			TCHAR_CALL_OS(RegDeleteValueW(a,b),RegDeleteValueA(a,TCHAR_TO_ANSI(b)))
#define RegQueryInfoKeyX(a,b)			TCHAR_CALL_OS(RegQueryInfoKeyW(a,NULL,NULL,NULL,b,NULL,NULL,NULL,NULL,NULL,NULL,NULL),RegQueryInfoKeyA(a,NULL,NULL,NULL,b,NULL,NULL,NULL,NULL,NULL,NULL,NULL))
#define RegOpenKeyExX(a,b,c,d,e)        TCHAR_CALL_OS(RegOpenKeyExW(a,b,c,d,e),RegOpenKeyExA(a,TCHAR_TO_ANSI(b),c,d,e))
#define LookupPrivilegeValueX(a,b,c)	TCHAR_CALL_OS(LookupPrivilegeValueW(a,b,c),LookupPrivilegeValueA(TCHAR_TO_ANSI(a),TCHAR_TO_ANSI(b),c))
#define GetDriveTypeX(a)				TCHAR_CALL_OS(GetDriveTypeW(a),GetDriveTypeA(TCHAR_TO_ANSI(a)))
#define GetDiskFreeSpaceX(a,b,c,d,e)	TCHAR_CALL_OS(GetDiskFreeSpaceW(a,b,c,d,e),GetDiskFreeSpaceA(TCHAR_TO_ANSI(a),b,c,d,e))
#define SetFileAttributesX(a,b)			TCHAR_CALL_OS(SetFileAttributesW(a,b),SetFileAttributesA(TCHAR_TO_ANSI(a),b))
#define DrawTextExX(a,b,c,d,e,f)		TCHAR_CALL_OS(DrawTextExW(a,b,c,d,e,f),DrawTextExA(a,const_cast<ANSICHAR*>(TCHAR_TO_ANSI(b)),c,d,e,f))
#define DrawTextX(a,b,c,d,e)			TCHAR_CALL_OS(DrawTextW(a,b,c,d,e),DrawTextA(a,TCHAR_TO_ANSI(b),c,d,e))
#define GetTextExtentPoint32X(a,b,c,d)  TCHAR_CALL_OS(GetTextExtentPoint32W(a,b,c,d),GetTextExtentPoint32A(a,TCHAR_TO_ANSI(b),c,d))
#define DefMDIChildProcX(a,b,c,d)		TCHAR_CALL_OS(DefMDIChildProcW(a,b,c,d),DefMDIChildProcA(a,b,c,d))
#define SetClassLongX(a,b,c)			TCHAR_CALL_OS(SetClassLongW(a,b,c),SetClassLongA(a,b,c))
#define GetClassLongX(a,b)				TCHAR_CALL_OS(GetClassLongW(a,b),GetClassLongA(a,b))
#define RemovePropX(a,b)				TCHAR_CALL_OS(RemovePropW(a,b),RemovePropA(a,TCHAR_TO_ANSI(b)))
#define GetPropX(a,b)					TCHAR_CALL_OS(GetPropW(a,b),GetPropA(a,TCHAR_TO_ANSI(b)))
#define SetPropX(a,b,c)					TCHAR_CALL_OS(SetPropW(a,b,c),SetPropA(a,TCHAR_TO_ANSI(b),c))
#define ShellExecuteX(a,b,c,d,e,f)      TCHAR_CALL_OS(ShellExecuteW(a,b,c,d,e,f),ShellExecuteA(a,TCHAR_TO_ANSI(b),TCHAR_TO_ANSI(c),TCHAR_TO_ANSI(d),TCHAR_TO_ANSI(e),f))
#define CreateMutexX(a,b,c)				TCHAR_CALL_OS(CreateMutexW(a,b,c),CreateMutexA(a,b,TCHAR_TO_ANSI(c)))
#define DefFrameProcX(a,b,c,d,e)		TCHAR_CALL_OS(DefFrameProcW(a,b,c,d,e),DefFrameProcA(a,b,c,d,e))
#define RegisterWindowMessageX(a)       TCHAR_CALL_OS(RegisterWindowMessageW(a),RegisterWindowMessageA(TCHAR_TO_ANSI(a)))
#define AppendMenuX(a,b,c,d)            TCHAR_CALL_OS(AppendMenuW(a,b,c,d),AppendMenuA(a,b,c,TCHAR_TO_ANSI(d)))
#define LoadLibraryX(a)					TCHAR_CALL_OS(LoadLibraryW(a),LoadLibraryA(TCHAR_TO_ANSI(a)))
#define SystemParametersInfoX(a,b,c,d)	TCHAR_CALL_OS(SystemParametersInfoW(a,b,c,d),SystemParametersInfoA(a,b,c,d))
#define DispatchMessageX(a)				TCHAR_CALL_OS(DispatchMessageW(a),DispatchMessageA(a))
#define PeekMessageX(a,b,c,d,e)			TCHAR_CALL_OS(PeekMessageW(a,b,c,d,e),PeekMessageA(a,b,c,d,e))
#define PostMessageX(a,b,c,d)			TCHAR_CALL_OS(PostMessageW(a,b,c,d),PostMessageA(a,b,c,d))
#define SendMessageX(a,b,c,d)			TCHAR_CALL_OS(SendMessageW(a,b,c,d),SendMessageA(a,b,c,d))
#define SendMessageLX(a,b,c,d)			TCHAR_CALL_OS(SendMessageW(a,b,c,(LPARAM)d),SendMessageA(a,b,c,(LPARAM)TCHAR_TO_ANSI(d)))
#define SendMessageWX(a,b,c,d)			TCHAR_CALL_OS(SendMessageW(a,b,(WPARAM)c,d),SendMessageA(a,b,(WPARAM)TCHAR_TO_ANSI(c),d))
#define DefWindowProcX(a,b,c,d)			TCHAR_CALL_OS(DefWindowProcW(a,b,c,d),DefWindowProcA(a,b,c,d))
#define CallWindowProcX(a,b,c,d,e)		TCHAR_CALL_OS(CallWindowProcW(a,b,c,d,e),CallWindowProcA(a,b,c,d,e))
#define GetWindowLongX(a,b)				TCHAR_CALL_OS(GetWindowLongW(a,b),GetWindowLongA(a,b))
#define SetWindowLongX(a,b,c)			TCHAR_CALL_OS(SetWindowLongW(a,b,c),SetWindowLongA(a,b,c))
#define LoadMenuIdX(i,n)				TCHAR_CALL_OS(LoadMenuW(i,MAKEINTRESOURCEW(n)),LoadMenuA(i,MAKEINTRESOURCEA(n)))
#define LoadCursorIdX(i,n)				TCHAR_CALL_OS(LoadCursorW(i,MAKEINTRESOURCEW(n)),LoadCursorA(i,MAKEINTRESOURCEA(n)))
#define LoadIconIdX(i,n)				TCHAR_CALL_OS(LoadIconW(i,MAKEINTRESOURCEW(n)),LoadIconA(i,MAKEINTRESOURCEA(n)))

inline DWORD GetTempPathX( DWORD nBufferLength, LPTSTR lpBuffer )
{
	DWORD Result;
#if UNICODE
	if( !GUnicodeOS )
	{
		ANSICHAR ACh[MAX_PATH]="";
		Result = GetTempPathA( ARRAY_COUNT(ACh), ACh );
		appStrncpy( lpBuffer, appFromAnsi(ACh), nBufferLength );
	}
	else
#endif
	{
		Result = GetTempPath( nBufferLength, lpBuffer );
	}
	return Result;
}
inline LONG RegQueryValueExX( HKEY hKey, LPCTSTR lpValueName, LPDWORD lpReserved, LPDWORD lpType, LPBYTE lpData, LPDWORD lpcbData )
{
#if UNICODE
	if( !GUnicodeOS )
	{
		ANSICHAR* ACh = (ANSICHAR*)appAlloca(*lpcbData);
		LONG Result = RegQueryValueExA( hKey, TCHAR_TO_ANSI(lpValueName), lpReserved, lpType, (BYTE*)ACh, lpcbData );
		if( Result==ERROR_SUCCESS )
			MultiByteToWideChar( CP_ACP, 0, ACh, -1, (TCHAR*)lpData, *lpcbData );
		return Result;
	}
	else
#endif
	{
		return RegQueryValueEx( hKey, lpValueName, lpReserved, lpType, lpData, lpcbData );
	}
}

#if UNICODE
	#define MAKEINTRESOURCEX(a)  MAKEINTRESOURCEA(a)
	#define OSVERSIONINFOX OSVERSIONINFOA
	#define GetVersionExX GetVersionExA
	extern WINDOW_API BOOL (WINAPI* Shell_NotifyIconWX)( DWORD dwMessage, PNOTIFYICONDATAW pnid );
	extern WINDOW_API BOOL (WINAPI* SHGetSpecialFolderPathWX)( HWND hwndOwner, LPTSTR lpszPath, INT nFolder, BOOL fCreate );
	inline HRESULT SHGetSpecialFolderPathX( HWND hwndOwner, LPTSTR lpszPath, INT nFolder, BOOL fCreate )
	{
		if( !GUnicodeOS || !SHGetSpecialFolderPathWX )
		{
			ANSICHAR ACh[MAX_PATH];
			ITEMIDLIST* IdList=NULL;
#if 1 /* Needed because Windows 95 doesn't export SHGetSpecialFolderPath */
			HRESULT Result = SHGetSpecialFolderLocation( NULL, nFolder, &IdList );
			SHGetPathFromIDListA( IdList, ACh );
#else
			HRESULT Result = SHGetSpecialFolderPathA( hwndOwner, ACh, nFolder, fCreate );
#endif
			MultiByteToWideChar( CP_ACP, 0, ACh, -1, lpszPath, MAX_PATH );
			return Result;
		}
		else return SHGetSpecialFolderPathWX( hwndOwner, lpszPath, nFolder, fCreate );
	}
#else
	#define MAKEINTRESOURCEX(a) MAKEINTRESOURCEA(a)
	#define OSVERSIONINFOX OSVERSIONINFOA
	#define GetVersionExX GetVersionExA
	#define GetTempPathX GetTempPathA
	#define SHGetSpecialFolderPathX(a,b,c,d) SHGetSpecialFolderPathA(a,TCHAR_TO_ANSI(b),c,d)
#endif

/*-----------------------------------------------------------------------------
	Globals.
-----------------------------------------------------------------------------*/

// Classes.
class WWindow;
class WControl;
class WWizardDialog;
class WWizardPage;
class WDragInterceptor;

// Global functions.
WINDOW_API void InitWindowing();
WINDOW_API HBITMAP LoadFileToBitmap( const TCHAR* Filename );

// Global variables.
extern WINDOW_API HBRUSH hBrushWhite;
extern WINDOW_API HBRUSH hBrushOffWhite;
extern WINDOW_API HBRUSH hBrushHeadline;
extern WINDOW_API HBRUSH hBrushBlack;
extern WINDOW_API HBRUSH hBrushStipple;
extern WINDOW_API HBRUSH hBrushCurrent;
extern WINDOW_API HBRUSH hBrushDark;
extern WINDOW_API HBRUSH hBrushGrey;
extern WINDOW_API HFONT  hFontText;
extern WINDOW_API HFONT  hFontUrl;
extern WINDOW_API HFONT  hFontHeadline;
extern WINDOW_API class  WLog* GLog;
extern WINDOW_API HINSTANCE hInstanceWindow;
extern WINDOW_API UBOOL GNotify;
extern WINDOW_API UINT WindowMessageOpen;
extern WINDOW_API UINT WindowMessageMouseWheel;
extern WINDOW_API NOTIFYICONDATA NID;
#if UNICODE
	extern WINDOW_API NOTIFYICONDATAA NIDA;
#else
	#define NIDA NID
#endif

/*-----------------------------------------------------------------------------
	Window class definition macros.
-----------------------------------------------------------------------------*/

inline void MakeWindowClassName( TCHAR* Result, const TCHAR* Base )
{
	guard(MakeWindowClassName);
	appSprintf( Result, TEXT("%sUnreal%s"), appPackage(), Base );
	unguard;
}
#if WIN_OBJ
	#define DECLARE_WINDOWCLASS(cls,parentcls,pkg) \
	public: \
		void GetWindowClassName( TCHAR* Result ); \
		void Destroy(); \
		virtual const TCHAR* GetPackageName(); }
#else
	#define DECLARE_WINDOWCLASS(cls,parentcls,pkg) \
	public: \
		void GetWindowClassName( TCHAR* Result ); \
		~cls(); \
		virtual const TCHAR* GetPackageName();
#endif
#define DECLARE_WINDOWSUBCLASS(cls,parentcls,pkg) \
	DECLARE_WINDOWCLASS(cls,parentcls,pkg) \
	static WNDPROC SuperProc;

#define IMPLEMENT_WINDOWCLASS(cls,clsf) \
	{TCHAR Temp[256]; MakeWindowClassName(Temp,TEXT(#cls)); cls::RegisterWindowClass( Temp, clsf );}

#define IMPLEMENT_WINDOWSUBCLASS(cls,wincls) \
	{TCHAR Temp[256]; MakeWindowClassName(Temp,TEXT(#cls)); cls::SuperProc = cls::RegisterWindowClass( Temp, wincls );}

#define FIRST_AUTO_CONTROL 8192

/*-----------------------------------------------------------------------------
	FRect.
-----------------------------------------------------------------------------*/

struct FPoint
{
	INT X, Y;
	FPoint()
	{}
	FPoint( INT InX, INT InY )
	:	X( InX )
	,	Y( InY )
	{}
	static FPoint ZeroValue()
	{
		return FPoint(0,0);
	}
	static FPoint NoneValue()
	{
		return FPoint(INDEX_NONE,INDEX_NONE);
	}
	operator POINT*() const
	{
		return (POINT*)this;
	}
	const INT& operator()( INT i ) const
	{
		return (&X)[i];
	}
	INT& operator()( INT i )
	{
		return (&X)[i];
	}
	static INT Num()
	{
		return 2;
	}
	UBOOL operator==( const FPoint& Other ) const
	{
		return X==Other.X && Y==Other.Y;
	}
	UBOOL operator!=( const FPoint& Other ) const
	{
		return X!=Other.X || Y!=Other.Y;
	}
	FPoint& operator+=( const FPoint& Other )
	{
		X += Other.X;
		Y += Other.Y;
		return *this;
	}
	FPoint& operator-=( const FPoint& Other )
	{
		X -= Other.X;
		Y -= Other.Y;
		return *this;
	}
	FPoint operator+( const FPoint& Other ) const
	{
		return FPoint(*this) += Other;
	}
	FPoint operator-( const FPoint& Other ) const
	{
		return FPoint(*this) -= Other;
	}
};

struct FRect
{
	FPoint Min, Max;
	FRect()
	{}
	FRect( INT X0, INT Y0, INT X1, INT Y1 )
	:	Min( X0, Y0 )
	,	Max( X1, Y1 )
	{}
	FRect( FPoint InMin, FPoint InMax )
	:	Min( InMin )
	,	Max( InMax )
	{}
	FRect( RECT R )
	:	Min( R.left, R.top )
	,	Max( R.right, R.bottom )
	{}
	operator RECT*() const
	{
		return (RECT*)this;
	}
	const FPoint& operator()( INT i ) const
	{
		return (&Min)[i];
	}
	FPoint& operator()( INT i )
	{
		return (&Min)[i];
	}
	static INT Num()
	{
		return 2;
	}
	UBOOL operator==( const FRect& Other ) const
	{
		return Min==Other.Min && Max==Other.Max;
	}
	UBOOL operator!=( const FRect& Other ) const
	{
		return Min!=Other.Min || Max!=Other.Max;
	}
	FRect Right( INT Width )
	{
		return FRect( ::Max(Min.X,Max.X-Width), Min.Y, Max.X, Max.Y );
	}
	FRect Bottom( INT Height )
	{
		return FRect( Min.X, ::Max(Min.Y,Max.Y-Height), Max.X, Max.Y );
	}
	FPoint Size()
	{
		return FPoint( Max.X-Min.X, Max.Y-Min.Y );
	}
	INT Width()
	{
		return Max.X-Min.X;
	}
	INT Height()
	{
		return Max.Y-Min.Y;
	}
	FRect& operator+=( const FPoint& P )
	{
		Min += P;
		Max += P;
		return *this;
	}
	FRect& operator-=( const FPoint& P )
	{
		Min -= P;
		Max -= P;
		return *this;
	}
	FRect operator+( const FPoint& P ) const
	{
		return FRect( Min+P, Max+P );
	}
	FRect operator-( const FPoint& P ) const
	{
		return FRect( Min-P, Max-P );
	}
	FRect operator+( const FRect& R ) const
	{
		return FRect( Min+R.Min, Max+R.Max );
	}
	FRect operator-( const FRect& R ) const
	{
		return FRect( Min-R.Min, Max-R.Max );
	}
	FRect Inner( FPoint P ) const
	{
		return FRect( Min+P, Max-P );
	}
	UBOOL Contains( FPoint P ) const
	{
		return P.X>=Min.X && P.X<Max.X && P.Y>=Min.Y && P.Y<Max.Y;
	}
};

/*-----------------------------------------------------------------------------
	FControlSnoop.
-----------------------------------------------------------------------------*/

// For forwarding interaction with a control to an object.
class WINDOW_API FControlSnoop
{
public:
	// FControlSnoop interface.
	virtual void SnoopChar( WWindow* Src, INT Char ) {}
	virtual void SnoopKeyDown( WWindow* Src, INT Char ) {}
	virtual void SnoopLeftMouseDown( WWindow* Src, FPoint P ) {}
	virtual void SnoopRightMouseDown( WWindow* Src, FPoint P ) {}
};

/*-----------------------------------------------------------------------------
	FCommandTarget.
-----------------------------------------------------------------------------*/

//
// Interface for accepting commands.
//
class WINDOW_API FCommandTarget
{
public:
	virtual void Unused() {}
};

//
// Delegate function pointers.
//
typedef void(FCommandTarget::*TDelegate)();
typedef void(FCommandTarget::*TDelegateInt)(INT);

//
// Simple bindings to an object and a member function of that object.
//
struct WINDOW_API FDelegate
{
	FCommandTarget* TargetObject;
	void (FCommandTarget::*TargetInvoke)();
	FDelegate( FCommandTarget* InTargetObject=NULL, TDelegate InTargetInvoke=NULL );
	virtual void operator()();
};
struct WINDOW_API FDelegateInt
{
	FCommandTarget* TargetObject;
	void (FCommandTarget::*TargetInvoke)(int);
	FDelegateInt( FCommandTarget* InTargetObject=NULL, TDelegateInt InTargetInvoke=NULL );
	virtual void operator()( int I );
};

// Text formatting.
inline const TCHAR* LineFormat( const TCHAR* In )
{
	guard(LineFormat);
	static TCHAR Result[4069];
	TCHAR* Ptr = Result;
	while( *In )
		*Ptr++ = *In++!='\\' ? In[-1] : *In++=='n' ? '\n' : In[-1];
	*Ptr++ = 0;
	return Result;
	unguard;
}

/*-----------------------------------------------------------------------------
	Menu helper functions.
-----------------------------------------------------------------------------*/

//
// Load a menu and localize its text.
//
inline void LocalizeSubMenu( HMENU hMenu, const TCHAR* Name, const TCHAR* Package )
{
	guard(LocalizeSubMenu);
	for( INT i=GetMenuItemCount(hMenu)-1; i>=0; i-- )
	{
#if UNICODE
		if( GUnicode && !GUnicodeOS )
		{
			MENUITEMINFOA Info;
			ANSICHAR Buffer[1024];
			appMemzero( &Info, sizeof(Info) );
			Info.cbSize     = sizeof(Info);
			Info.fMask      = MIIM_TYPE | MIIM_SUBMENU;
			Info.cch        = ARRAY_COUNT(Buffer);
			Info.dwTypeData = Buffer;
			GetMenuItemInfoA( hMenu, i, 1, &Info );
			const ANSICHAR* String = (const ANSICHAR*)Info.dwTypeData;
			if( String && String[0]=='I' && String[1]=='D' && String[2]=='_' )
			{
				const_cast<const ANSICHAR*&>(Info.dwTypeData) = TCHAR_TO_ANSI(Localize( Name, appFromAnsi(String), Package ));
				SetMenuItemInfoA( hMenu, i, 1, &Info );
			}
			if( Info.hSubMenu )
				LocalizeSubMenu( Info.hSubMenu, Name, Package );
		}
		else
#endif
		{
			MENUITEMINFO Info;
			TCHAR Buffer[1024];
			appMemzero( &Info, sizeof(Info) );
			Info.cbSize     = sizeof(Info);
			Info.fMask      = MIIM_TYPE | MIIM_SUBMENU;
			Info.cch        = ARRAY_COUNT(Buffer);
			Info.dwTypeData = Buffer;
			GetMenuItemInfo( hMenu, i, 1, &Info );
			const TCHAR* String = (const TCHAR*)Info.dwTypeData;
			if( String && String[0]=='I' && String[1]=='D' && String[2]=='_' )
			{
				const_cast<const TCHAR*&>(Info.dwTypeData) = Localize( Name, String, Package );
				SetMenuItemInfo( hMenu, i, 1, &Info );
			}
			if( Info.hSubMenu )
				LocalizeSubMenu( Info.hSubMenu, Name, Package );
		}
	}
	unguard;
}

WINDOW_API HMENU LoadLocalizedMenu( HINSTANCE hInstance, INT Id, const TCHAR* Name, const TCHAR* Package=GPackage );

//
// Toggle a menu item and return 0 if it's now off, 1 if it's now on.
//
inline WINDOW_API UBOOL ToggleMenuItem( HMENU hMenu, UBOOL Item );

/*-----------------------------------------------------------------------------
	FWindowsBitmap.
-----------------------------------------------------------------------------*/

// A bitmap.
class FWindowsBitmap
{
public:
	INT SizeX, SizeY, Keep;
	FWindowsBitmap( UBOOL InKeep=0 )
	: hBitmap( NULL )
	, SizeX( 0 )
	, SizeY( 0 )
	, Keep( InKeep )
	{}
	~FWindowsBitmap()
	{
		if( hBitmap && !Keep )
			DeleteObject( hBitmap );
	}
	UBOOL LoadFile( const TCHAR* Filename )
	{
		if( hBitmap )
			DeleteObject( hBitmap );
		//hBitmap = LoadFileToBitmap( Filename, SizeX, SizeY );
		hBitmap = LoadFileToBitmap( Filename );
		return hBitmap!=NULL;
	}
	HBITMAP GetBitmapHandle()
	{
		return hBitmap;
	}
private:
	HBITMAP hBitmap;
	void operator=( FWindowsBitmap& ) {}
};

/*-----------------------------------------------------------------------------
	WWindow.
-----------------------------------------------------------------------------*/

#if WIN_OBJ
	#define W_DECLARE_ABSTRACT_CLASS(a,b,c) DECLARE_ABSTRACT_CLASS(a,b,c) 
	#define W_DECLARE_CLASS(a,b,c) DECLARE_CLASS(a,b,c) 
	#define W_IMPLEMENT_CLASS(a) IMPLEMENT_CLASS(a)
#else
	#define W_DECLARE_ABSTRACT_CLASS(a,b,c) public:
	#define W_DECLARE_CLASS(a,b,c) public:
	#define W_IMPLEMENT_CLASS(a)
#endif

// An operating system window.
class WINDOW_API WWindow : 
#if WIN_OBJ
public UObject, 
#endif
public FCommandTarget
{
	W_DECLARE_ABSTRACT_CLASS(WWindow,UObject,CLASS_Transient);

	// Variables.
	HWND					hWnd;
	FName					PersistentName;
	WORD					ControlId, TopControlId;
	BITFIELD				Destroyed:1;
	BITFIELD				MdiChild:1;
	WWindow*				OwnerWindow;
	FNotifyHook*			NotifyHook;
	FControlSnoop*			Snoop;
	TArray<class WControl*>	Controls;

	// Static.
	static INT              ModalCount;
	static TArray<WWindow*> _Windows;
	static TArray<WWindow*> _DeleteWindows;
	static LONG APIENTRY StaticWndProc( HWND hWnd, UINT Message, UINT wParam, LONG lParam );
	static WNDPROC RegisterWindowClass( const TCHAR* Name, DWORD Style );

	// Structors.
	WWindow( FName InPersistentName=NAME_None, WWindow* InOwnerWindow=NULL );
#if WIN_OBJ
	void Destroy();
#else
	virtual ~WWindow();
#endif

	// Accessors.
	FRect GetClientRect() const;
	void MoveWindow( FRect R, UBOOL bRepaint );
	FRect GetWindowRect() const;
	FPoint ClientToScreen( const FPoint& InP );
	FPoint ScreenToClient( const FPoint& InP );
	FRect ClientToScreen( const FRect& InR );
	FRect ScreenToClient( const FRect& InR );
	FPoint GetCursorPos();

	// WWindow interface.
	virtual void Serialize( FArchive& Ar );
	virtual const TCHAR* GetPackageName();
	virtual void DoDestroy();
	virtual void GetWindowClassName( TCHAR* Result )=0;
	virtual LONG WndProc( UINT Message, UINT wParam, LONG lParam );
	virtual INT CallDefaultProc( UINT Message, UINT wParam, LONG lParam );
	virtual UBOOL InterceptControlCommand( UINT Message, UINT wParam, LONG lParam );
	virtual FString GetText();
	virtual void SetText( const TCHAR* Text );
	virtual void SetTextf( const TCHAR* Text,... );
	virtual INT GetLength();

	void SetNotifyHook( FNotifyHook* InNotifyHook );

	// WWindow notifications.
	virtual void OnCopyData( HWND hWndSender, COPYDATASTRUCT* CD );
	virtual void OnSetFocus( HWND hWndLosingFocus );
	virtual void OnKillFocus( HWND hWndGaininFocus );
	virtual void OnSize( DWORD Flags, INT NewX, INT NewY );
	virtual void OnCommand( INT Command );
	virtual void OnActivate( UBOOL Active );
	virtual void OnChar( TCHAR Ch );
	virtual void OnKeyDown( TCHAR Ch );
	virtual void OnCut();
	virtual void OnCopy();
	virtual void OnPaste();
	virtual void OnShowWindow( UBOOL bShow );
	virtual void OnUndo();
	virtual void OnPaint();
	virtual void OnCreate();
	virtual void OnDrawItem( DRAWITEMSTRUCT* Info );
	virtual void OnMeasureItem( MEASUREITEMSTRUCT* Info );
	virtual void OnInitDialog();
	virtual void OnMouseEnter();
	virtual void OnMouseLeave();
	virtual void OnMouseHover();
	virtual void OnTimer();
	virtual void OnReleaseCapture();
	virtual void OnMdiActivate( UBOOL Active );
	virtual void OnMouseMove( DWORD Flags, FPoint Location );
	virtual void OnLeftButtonDown();
	virtual void OnRightButtonDown();
	virtual void OnLeftButtonUp();
	virtual void OnRightButtonUp();

	virtual INT OnSetCursor();
	virtual void OnClose();
	virtual void OnDestroy();

	// WWindow functions.
	void MaybeDestroy();
	void _CloseWindow();
	operator HWND() const;
	void SetFont( HFONT hFont );
	void PerformCreateWindowEx( DWORD dwExStyle, LPCTSTR lpWindowName, DWORD dwStyle, INT x, INT y, INT nWidth, int nHeight, HWND hWndParent, HMENU hMenu, HINSTANCE hInstance );
	void SetRedraw( UBOOL Redraw );
};

/*-----------------------------------------------------------------------------
	WControl.
-----------------------------------------------------------------------------*/

// A control which exists inside an owner window.
class WINDOW_API WControl : public WWindow
{
	W_DECLARE_ABSTRACT_CLASS(WControl,WWindow,CLASS_Transient);

	// Variables.
	WNDPROC WindowDefWndProc;

	// Structors.
	WControl();
	WControl( WWindow* InOwnerWindow, INT InId, WNDPROC InSuperProc );
#if WIN_OBJ
	void Destroy();
#else
	~WControl();
#endif

	// WWindow interface.
	virtual INT CallDefaultProc( UINT Message, UINT wParam, LONG lParam );
	static WNDPROC RegisterWindowClass( const TCHAR* Name, const TCHAR* WinBaseClass );
};

/*-----------------------------------------------------------------------------
	WLabel.
-----------------------------------------------------------------------------*/

// A non-interactive label control.
class WINDOW_API WLabel : public WControl
{
	W_DECLARE_CLASS(WLabel,WControl,CLASS_Transient)
	DECLARE_WINDOWSUBCLASS(WLabel,WControl,Window)

	// Constructor.
	WLabel();
	WLabel( WWindow* InOwner, INT InId=0, WNDPROC InSuperProc=NULL );

	// WWindow interface.
	void OpenWindow( UBOOL Visible );
};

/*-----------------------------------------------------------------------------
	WButton.
-----------------------------------------------------------------------------*/

// A button.
class WINDOW_API WButton : public WControl
{
	W_DECLARE_CLASS(WButton,WControl,CLASS_Transient);
	DECLARE_WINDOWSUBCLASS(WButton,WControl,Window)

	// Delegates.
	FDelegate ClickDelegate;
	FDelegate DoubleClickDelegate;
	FDelegate PushDelegate;
	FDelegate UnPushDelegate;
	FDelegate SetFocusDelegate;
	FDelegate KillFocusDelegate;

	// Constructor.
	WButton();
	WButton( WWindow* InOwner, INT InId=0, FDelegate InClicked=FDelegate(), WNDPROC InSuperProc=NULL );

	// WWindow interface.
	void OpenWindow( UBOOL Visible, INT X, INT Y, INT XL, INT YL, const TCHAR* Text );
	void SetVisibleText( const TCHAR* Text );

	// WControl interface.
	UBOOL InterceptControlCommand( UINT Message, UINT wParam, LONG lParam );
};

/*-----------------------------------------------------------------------------
	WCoolButton.
-----------------------------------------------------------------------------*/

// Frame showing styles.
enum EFrameFlags
{
	CBFF_ShowOver	= 0x01,
	CBFF_ShowAway	= 0x02,
	CBFF_DimAway    = 0x04,
	CBFF_UrlStyle	= 0x08,
	CBFF_NoCenter   = 0x10
};

// A coolbar-style button.
class WINDOW_API WCoolButton : public WButton
{
	W_DECLARE_CLASS(WCoolButton,WButton,CLASS_Transient);
	DECLARE_WINDOWSUBCLASS(WCoolButton,WButton,Window)

	// Variables.
	static WCoolButton* GlobalCoolButton;
	HICON hIcon;
	DWORD FrameFlags;

	// Constructor.
	WCoolButton();
	WCoolButton( WWindow* InOwner, INT InId=0, FDelegate InClicked=FDelegate(), DWORD InFlags=CBFF_ShowOver|CBFF_DimAway );

	// WWindow interface.
	void OpenWindow( UBOOL Visible, INT X, INT Y, INT XL, INT YL, const TCHAR* Text );
	void OnDestroy();
	void OnCreate();

	// WCoolButton interface.
	void UpdateHighlight( UBOOL TurnOff );
	void OnTimer();
	INT OnSetCursor();

	// WWindow interface.
	void OnDrawItem( DRAWITEMSTRUCT* Item );
};

/*-----------------------------------------------------------------------------
	WUrlButton.
-----------------------------------------------------------------------------*/

// A URL button.
class WINDOW_API WUrlButton : public WCoolButton
{
	W_DECLARE_CLASS(WUrlButton,WCoolButton,CLASS_Transient);
	DECLARE_WINDOWSUBCLASS(WUrlButton,WCoolButton,Window)

	// Variables.
	FString URL;

	// Constructor.
	WUrlButton();
	WUrlButton( WWindow* InOwner, const TCHAR* InURL, INT InId=0 );

	// WUrlButton interface.
	void OnClick();
};

/*-----------------------------------------------------------------------------
	WComboBox.
-----------------------------------------------------------------------------*/

// A combo box control.
class WINDOW_API WComboBox : public WControl
{
	W_DECLARE_CLASS(WComboBox,WControl,CLASS_Transient);
	DECLARE_WINDOWSUBCLASS(WComboBox,WControl,Window)

	// Delegates.
	FDelegate DoubleClickDelegate;
	FDelegate DropDownDelegate;
	FDelegate CloseComboDelegate;
	FDelegate EditChangeDelegate;
	FDelegate EditUpdateDelegate;
	FDelegate SetFocusDelegate;
	FDelegate KillFocusDelegate;
	FDelegate SelectionChangeDelegate;
	FDelegate SelectionEndOkDelegate;
	FDelegate SelectionEndCancelDelegate;
 
	// Constructor.
	WComboBox();
	WComboBox( WWindow* InOwner, INT InId=0, WNDPROC InSuperProc=NULL );

	// WWindow interface.
	void OpenWindow( UBOOL Visible );
	virtual LONG WndProc( UINT Message, UINT wParam, LONG lParam );

	// WControl interface.
	UBOOL InterceptControlCommand( UINT Message, UINT wParam, LONG lParam );

	// WComboBox interface.
	virtual void AddString( const TCHAR* Str );
	virtual FString GetString( INT Index );
	virtual INT GetCount();
	virtual void SetCurrent( INT Index );
	virtual INT GetCurrent();
	virtual INT FindString( const TCHAR* String );
};

/*-----------------------------------------------------------------------------
	WEdit.
-----------------------------------------------------------------------------*/

// A single-line or multiline edit control.
class WINDOW_API WEdit : public WControl
{
	W_DECLARE_CLASS(WEdit,WControl,CLASS_Transient);
	DECLARE_WINDOWSUBCLASS(WEdit,WControl,Window)

	// Variables.
	FDelegate ChangeDelegate;

	// Constructor.
	WEdit();
	WEdit( WWindow* InOwner, INT InId=0, WNDPROC InSuperProc=NULL );

	// WWindow interface.
	void OpenWindow( UBOOL Visible, UBOOL Multiline, UBOOL ReadOnly );
	UBOOL InterceptControlCommand( UINT Message, UINT wParam, LONG lParam );

	// WEdit interface.
	UBOOL GetReadOnly();
	void SetReadOnly( UBOOL ReadOnly );
	INT GetLineCount();
	INT GetLineIndex( INT Line );
	void GetSelection( INT& Start, INT& End );
	void SetSelection( INT Start, INT End );
	void SetSelectedText( const TCHAR* Text );
	UBOOL GetModify();
	void SetModify( UBOOL Modified );
	void ScrollCaret();
};

/*-----------------------------------------------------------------------------
	WTerminal.
-----------------------------------------------------------------------------*/

// Base class of terminal edit windows.
class WINDOW_API WTerminalBase : public WWindow
{
	W_DECLARE_ABSTRACT_CLASS(WTerminalBase,WWindow,CLASS_Transient);
	DECLARE_WINDOWCLASS(WTerminalBase,WWindow,Window)

	// Constructor.
	WTerminalBase();
	WTerminalBase( FName InPersistentName, WWindow* InOwnerWindow );

	// WTerminalBase interface.
	virtual void TypeChar( TCHAR Ch )=0;
	virtual void Paste()=0;
};

// A terminal edit window.
class WINDOW_API WEditTerminal : public WEdit
{
	W_DECLARE_ABSTRACT_CLASS(WEditTerminal,WEdit,CLASS_Transient)
	DECLARE_WINDOWCLASS(WEditTerminal,WEdit,Window)

	// Variables.
	WTerminalBase* OwnerTerminal;

	// Constructor.
	WEditTerminal( WTerminalBase* InOwner=NULL );

	// WWindow interface.
	void OnChar( TCHAR Ch );
	void OnRightButtonDown();
	void OnPaste();
	void OnUndo();
};

// A terminal window.
class WINDOW_API WTerminal : public WTerminalBase, public FOutputDevice
{
	W_DECLARE_CLASS(WTerminal,WTerminalBase,CLASS_Transient);
	DECLARE_WINDOWCLASS(WTerminal,WTerminalBase,Window)

	// Variables.
	WEditTerminal Display;
	FExec* Exec;
	INT MaxLines, SlackLines;
	TCHAR Typing[256];
	UBOOL Shown;

	// Structors.
	WTerminal();
	WTerminal( FName InPersistentName, WWindow* InOwnerWindow );

	// FOutputDevice interface.
	void Serialize( void* Data, INT Count, EName MsgType );

	// WWindow interface.
	void OnShowWindow( UBOOL bShow );
	void OnCreate();
	void OpenWindow( UBOOL bMdi=0, UBOOL AppWindow=0 );
	void OnSetFocus( HWND hWndLoser );
	void OnSize( DWORD Flags, INT NewX, INT NewY );

	// WTerminalBase interface.
	void Paste();
	void TypeChar( TCHAR Ch );

	// WTerminal interface.
	void SelectTyping();
	void UpdateTyping();
	void SetExec( FExec* InExec );
};

/*-----------------------------------------------------------------------------
	WLog.
-----------------------------------------------------------------------------*/

// A log window.
class WINDOW_API WLog : public WTerminal
{
	W_DECLARE_CLASS(WLog,WTerminal,CLASS_Transient);
	DECLARE_WINDOWCLASS(WLog,WTerminal,Window)

	// Variables.
	UINT NidMessage;
	FArchive*& LogAr;
	FString LogFilename;

	// Functions.
	WLog();
	WLog( FName InPersistentName, WWindow* InOwnerWindow=NULL );
	void SetText( const TCHAR* Text );
	void OnShowWindow( UBOOL bShow );
	void OpenWindow( UBOOL bShow, UBOOL bMdi );
	void OnDestroy();
	void OnCopyData( HWND hWndSender, COPYDATASTRUCT* CD );
	void OnClose();
	void OnCommand( INT Command );
	LONG WndProc( UINT Message, UINT wParam, LONG lParam );
};

/*-----------------------------------------------------------------------------
	WDialog.
-----------------------------------------------------------------------------*/

// A dialog window, always based on a Visual C++ dialog template.
class WINDOW_API WDialog : public WWindow
{
	W_DECLARE_ABSTRACT_CLASS(WDialog,WWindow,CLASS_Transient);

	// Constructors.
	WDialog();
	WDialog( FName InPersistentName, INT InDialogId, WWindow* InOwnerWindow=NULL );

	// WDialog interface.
	INT CallDefaultProc( UINT Message, UINT wParam, LONG lParam );
	virtual INT DoModal( HINSTANCE hInst=hInstanceWindow );
	void OpenChildWindow( INT InControlId, UBOOL Visible );
	static BOOL CALLBACK LocalizeTextEnumProc( HWND hInWmd, LPARAM lParam );
	virtual void LocalizeText( const TCHAR* Section, const TCHAR* Package=GPackage );
	virtual void OnInitDialog();
	void EndDialog( INT Result );
	void EndDialogTrue();
	void EndDialogFalse();
};

/*-----------------------------------------------------------------------------
	WPasswordDialog.
-----------------------------------------------------------------------------*/

// A password dialog box.
class WINDOW_API WPasswordDialog : public WDialog
{
	W_DECLARE_CLASS(WPasswordDialog,WDialog,CLASS_Transient);
	DECLARE_WINDOWCLASS(WPasswordDialog,WDialog,Window)

	// Controls.
	WCoolButton OkButton;
	WCoolButton CancelButton;
	WEdit Name;
	WEdit Password;
	WLabel Prompt;

	// Output.
	FString ResultName;
	FString ResultPassword;

	// Constructor.
	WPasswordDialog();

	// WDialog interface.
	void OnInitDialog();
	void OnDestroy();
};

/*-----------------------------------------------------------------------------
	WTextScrollerDialog.
-----------------------------------------------------------------------------*/

// A text scroller dialog box.
class WINDOW_API WTextScrollerDialog : public WDialog
{
	W_DECLARE_CLASS(WTextScrollerDialog,WDialog,CLASS_Transient);
	DECLARE_WINDOWCLASS(WTextScrollerDialog,WDialog,Window)

	// Controls.
	WEdit TextEdit;
	WCoolButton OkButton;
	FString Caption, Message;

	// Constructor.
	WTextScrollerDialog();
	WTextScrollerDialog( const TCHAR* InCaption, const TCHAR* InMessage );

	// WDialog interface.
	void OnInitDialog();
};

/*-----------------------------------------------------------------------------
	WTrackBar.
-----------------------------------------------------------------------------*/

// A non-interactive label control.
class WINDOW_API WTrackBar : public WControl
{
	W_DECLARE_CLASS(WTrackBar,WControl,CLASS_Transient)
	DECLARE_WINDOWSUBCLASS(WTrackBar,WControl,Window)

	// Delegates.
	FDelegate ThumbTrackDelegate;
	FDelegate ThumbPositionDelegate;

	// Constructor.
	WTrackBar();
	WTrackBar( WWindow* InOwner, INT InId=0, WNDPROC InSuperProc=NULL );

	// WWindow interface.
	void OpenWindow( UBOOL Visible );

	// WControl interface.
	UBOOL InterceptControlCommand( UINT Message, UINT wParam, LONG lParam );

	// WTrackBar interface.
	void SetTicFreq( INT TicFreq );
	void SetRange( INT Min, INT Max );
	void SetPos( INT Pos );
	INT GetPos();
};

/*-----------------------------------------------------------------------------
	WTrackBar.
-----------------------------------------------------------------------------*/

// A non-interactive label control.
class WINDOW_API WProgressBar : public WControl
{
	W_DECLARE_CLASS(WProgressBar,WControl,CLASS_Transient);
	DECLARE_WINDOWSUBCLASS(WProgressBar,WControl,Window)

	// Variables.
	INT Percent;

	// Constructor.
	WProgressBar();
	WProgressBar( WWindow* InOwner, INT InId=0, WNDPROC InSuperProc=NULL );

	// WWindow interface.
	void OpenWindow( UBOOL Visible );

	// WProgressBar interface.
	void SetProgress( INT InCurrent, INT InMax );
};

/*-----------------------------------------------------------------------------
	WListBox.
-----------------------------------------------------------------------------*/

// A list box.
class WINDOW_API WListBox : public WControl
{
	W_DECLARE_CLASS(WListBox,WControl,CLASS_Transient);
	DECLARE_WINDOWSUBCLASS(WListBox,WControl,Window)

	// Delegates.
	FDelegate DoubleClickDelegate;
	FDelegate SelectionChangeDelegate;
	FDelegate SelectionCancelDelegate;
	FDelegate SetFocusDelegate;
	FDelegate KillFocusDelegate;

	// Constructor.
	WListBox();
	WListBox( WWindow* InOwner, INT InId=0, WNDPROC InSuperProc=NULL );

	// WWindow interface.
	void OpenWindow( UBOOL Visible, UBOOL Integral, UBOOL MultiSel, UBOOL OwnerDrawVariable );

	// WControl interface.
	UBOOL InterceptControlCommand( UINT Message, UINT wParam, LONG lParam );

	// WListBox interface.
	FString GetString( INT Index );
	void* GetItemData( INT Index );
	void SetItemData( INT Index, void* Value );
	INT GetCurrent();
	void SetCurrent( INT Index, UBOOL bScrollIntoView );
	INT GetTop();
	void SetTop( INT Index );
	void DeleteString( INT Index );
	INT GetCount();
	INT GetItemHeight( INT Index );
	INT ItemFromPoint( FPoint P );
	FRect GetItemRect( INT Index );
	void Empty();
	UBOOL GetSelected( INT Index );

	// Accessing as strings.
	INT AddString( const TCHAR* C );
	void InsertString( INT Index, const TCHAR* C );
	INT FindString( const TCHAR* C );
	INT FindStringChecked( const TCHAR* C );
	void InsertStringAfter( const TCHAR* Existing, const TCHAR* New );

	// Accessing as pointers.
	INT AddItem( const void* C );
	void InsertItem( INT Index, const void* C );
	INT FindItem( const void* C );
	INT FindItemChecked( const void* C );
	void InsertItemAfter( const void* Existing, const void* New );
};

/*-----------------------------------------------------------------------------
	FTreeItemBase.
-----------------------------------------------------------------------------*/

class WINDOW_API FTreeItemBase : public FCommandTarget, public FControlSnoop
{
public:
	virtual void Draw( HDC hDC )=0;
	virtual INT GetHeight()=0;
	virtual void SetSelected( UBOOL NewSelected )=0;
};

/*-----------------------------------------------------------------------------
	WItemBox.
-----------------------------------------------------------------------------*/

// A list box contaning list items.
class WINDOW_API WItemBox : public WListBox
{
	W_DECLARE_CLASS(WItemBox,WListBox,CLASS_Transient);
	DECLARE_WINDOWCLASS(WItemBox,WListBox,Window)

	// Constructors.
	WItemBox();
	WItemBox( WWindow* InOwner, INT InId=0);

	// WWindow interface.
	virtual LONG WndProc( UINT Message, UINT wParam, LONG lParam );
	void OnDrawItem( DRAWITEMSTRUCT* Info );
	void OnMeasureItem( MEASUREITEMSTRUCT* Info );
};

/*-----------------------------------------------------------------------------
	WPropertiesBase.
-----------------------------------------------------------------------------*/

class WINDOW_API WPropertiesBase : public WWindow, public FControlSnoop
{
	W_DECLARE_ABSTRACT_CLASS(WPropertiesBase,WWindow,CLASS_Transient);

	// Variables.
	UBOOL ShowTreeLines;
	WItemBox List;
	class FTreeItem* FocusItem;

	// Structors.
	WPropertiesBase();
	WPropertiesBase( FName InPersistentName, WWindow* InOwnerWindow );

	// WPropertiesBase interface.
	virtual FTreeItem* GetRoot()=0;
	virtual INT GetDividerWidth()=0;
	virtual void ResizeList()=0;
	virtual void SetItemFocus( UBOOL FocusCurrent )=0;
	virtual void ForceRefresh()=0;
	virtual class FTreeItem* GetListItem( INT i );
};

/*-----------------------------------------------------------------------------
	WDragInterceptor.
-----------------------------------------------------------------------------*/

// Splitter drag handler.
class WINDOW_API WDragInterceptor : public WWindow
{
	W_DECLARE_CLASS(WDragInterceptor,WWindow,CLASS_Transient);
	DECLARE_WINDOWCLASS(WDragInterceptor,WWindow,Window)

	// Variables.
	FPoint		OldMouseLocation;
	FPoint		DragIndices;
	FPoint		DragPos;
	FPoint		DragStart;
	FPoint		DrawWidth;
	FRect		DragClamp;
	UBOOL		Success;

	// Constructor.
	WDragInterceptor();
	WDragInterceptor( WWindow* InOwner, FPoint InDragIndices, FRect InDragClamp, FPoint InDrawWidth );

	// Functions.
	virtual void OpenWindow();
	virtual void ToggleDraw( HDC hInDC );
	void OnKeyDown( TCHAR Ch );
	void OnMouseMove( DWORD Flags, FPoint MouseLocation );
	void OnReleaseCapture();
	void OnLeftButtonUp();
};

/*-----------------------------------------------------------------------------
	FTreeItem.
-----------------------------------------------------------------------------*/

// Base class of list items.
class WINDOW_API FTreeItem : public FTreeItemBase
{
public:
	// Variables.
	class WPropertiesBase*	OwnerProperties;
	FTreeItem*				Parent;
	UBOOL					Expandable;
	UBOOL					Expanded;
	UBOOL					Sorted;
	UBOOL					Selected;
	INT						ButtonWidth;
	TArray<WCoolButton*>	Buttons;
	TArray<FTreeItem*>		Children;

	// Structors.
	FTreeItem();
	FTreeItem( WPropertiesBase* InOwnerProperties, FTreeItem* InParent, UBOOL InExpandable );
	virtual ~FTreeItem();

	// FTreeItem interface.
	virtual HBRUSH GetBackgroundBrush( UBOOL Selected );
	virtual COLORREF GetTextColor( UBOOL Selected );
	virtual void Serialize( FArchive& Ar );
	void EmptyChildren();
	virtual FRect GetRect();
	virtual void Redraw();
	virtual void OnItemSetFocus();
	virtual void OnItemKillFocus( UBOOL Abort );
	virtual void AddButton( const TCHAR* Text, FDelegate Action );
	virtual void OnItemLeftMouseDown( FPoint P );
	virtual void OnItemRightMouseDown( FPoint P );
	INT GetIndent();
	INT GetUnitIndentPixels();
	virtual INT GetIndentPixels( UBOOL Text );
	virtual FRect GetExpanderRect();
	virtual UBOOL GetSelected();
	void SetSelected( UBOOL InSelected );
	virtual void DrawTreeLines( HDC hDC, FRect Rect );
	virtual void Collapse();
	virtual void Expand();
	virtual void ToggleExpansion();
	virtual void OnItemDoubleClick();
	virtual BYTE* GetReadAddress( UProperty* Property, INT Offset );
	virtual void SetProperty( UProperty* Property, INT Offset, const TCHAR* Value );
	virtual void GetStates( TArray<FName>& States );
	virtual UBOOL AcceptFlags( DWORD InFlags );
	virtual QWORD GetId() const=0;
	virtual FString GetCaption() const=0;
	virtual void OnItemHelp();
	virtual void SetFocusToItem();
	virtual void SetValue( const TCHAR* Value );

	// FControlSnoop interface.
	void SnoopChar( WWindow* Src, INT Char );
	void SnoopKeyDown( WWindow* Src, INT Char );
};

// Property list item.
class WINDOW_API FPropertyItem : public FTreeItem
{
public:
	// Variables.
	UProperty*      Property;
	INT				Offset;
	INT				ArrayIndex;
	WEdit*			EditControl;
	WTrackBar*		TrackControl;
	WComboBox*		ComboControl;
	WLabel*			HolderControl;
	UBOOL			ComboChanged;
	FName			Name;

	// Constructors.
	FPropertyItem( WPropertiesBase* InOwnerProperties, FTreeItem* InParent, UProperty* InProperty, FName InName, INT InOffset, INT InArrayIndex );

	// FTreeItem interface.
	void Serialize( FArchive& Ar );
	QWORD GetId() const;
	FString GetCaption() const;
	virtual void OnItemLeftMouseDown( FPoint P );
	void GetPropertyText( TCHAR* Str, BYTE* ReadValue );
	void SetValue( const TCHAR* Value );
	void Draw( HDC hDC );
	INT GetHeight();
	void SetFocusToItem();
	void OnItemDoubleClick();
	void OnItemSetFocus();
	void OnItemKillFocus( UBOOL Abort );
	void Expand();
	void Collapse();

	// FControlSnoop interface.
	void SnoopChar( WWindow* Src, INT Char );
	void ComboSelectionEndCancel();
	void ComboSelectionEndOk();
	void OnTrackBarThumbTrack();
	void OnTrackBarThumbPosition();
	void OnChooseColorButton();
	void OnArrayAdd();
	void OnArrayEmpty();
	void OnArrayInsert();
	void OnArrayDelete();
	void OnBrowseButton();
	void OnUseCurrentButton();
	void OnClearButton();

	// FPropertyItem interface.
	virtual void Advance();
	virtual void SendToControl();
	virtual void ReceiveFromControl();
};

// An abstract list header.
class WINDOW_API FHeaderItem : public FTreeItem
{
public:
	// Constructors.
	FHeaderItem();
	FHeaderItem( WPropertiesBase* InOwnerProperties, FTreeItem* InParent, UBOOL InExpandable );

	// FTreeItem interface.
	void Draw( HDC hDC );
	INT GetHeight();
};

// An category header list item.
class WINDOW_API FCategoryItem : public FHeaderItem
{
public:
	// Variables.
	FName Category;
	UClass* BaseClass;

	// Constructors.
	FCategoryItem( WPropertiesBase* InOwnerProperties, FTreeItem* InParent, UClass* InBaseClass, FName InCategory, UBOOL InExpandable );

	// FTreeItem interface.
	void Serialize( FArchive& Ar );
	QWORD GetId() const;
	virtual FString GetCaption() const;
	void Expand();
	void Collapse();
};

/*-----------------------------------------------------------------------------
	WProperties.
-----------------------------------------------------------------------------*/

// General property editing control.
class WINDOW_API WProperties : public WPropertiesBase
{
	W_DECLARE_ABSTRACT_CLASS(WProperties,WPropertiesBase,CLASS_Transient);
	DECLARE_WINDOWCLASS(WProperties,WWindow,Window)

	// Variables.
	TArray<QWORD>		Remembered;
	QWORD				SavedTop, SavedCurrent;
	WDragInterceptor*	DragInterceptor;
	INT					DividerWidth;
	static TArray<WProperties*> PropertiesWindows;

	// Structors.
	WProperties();
	WProperties( FName InPersistentName, WWindow* InOwnerWindow=NULL );

	// WWindow interface.
	void Serialize( FArchive& Ar );
	void DoDestroy();
	void OnDestroy();
	void OpenChildWindow( INT InControlId );
	void OpenWindow( HWND hWndParent=NULL );
	void OnActivate( UBOOL Active );
	void OnSize( DWORD Flags, INT NewX, INT NewY );
	void OnPaint();

	// Delegates.
	void OnListDoubleClick();
	void OnListSelectionChange();

	// FControlSnoop interface.
	void SnoopLeftMouseDown( WWindow* Src, FPoint P );
	void SnoopRightMouseDown( WWindow* Src, FPoint P );
	void SnoopChar( WWindow* Src, INT Char );
	void SnoopKeyDown( WWindow* Src, INT Char );

	// WPropertiesBase interface.
	INT GetDividerWidth();

	// WProperties interface.
	virtual void SetValue( const TCHAR* Value );
	virtual void SetItemFocus( UBOOL FocusCurrent );
	virtual void ResizeList();
	virtual void ForceRefresh();
};

/*-----------------------------------------------------------------------------
	FPropertyItemBase.
-----------------------------------------------------------------------------*/

class WINDOW_API FPropertyItemBase : public FHeaderItem
{
public:
	// Variables.
	FString Caption;
	DWORD FlagMask;
	UClass* BaseClass;

	// Structors.
	FPropertyItemBase();
	FPropertyItemBase( WPropertiesBase* InOwnerProperties, FTreeItem* InParent, DWORD InFlagMask, const TCHAR* InCaption );

	// FTreeItem interface.
	void Serialize( FArchive& Ar );

	// FHeaderItem interface.
	UBOOL AcceptFlags( DWORD InFlags );
	void GetStates( TArray<FName>& States );
	void Collapse();
	FString GetCaption() const;
	QWORD GetId() const;
};

/*-----------------------------------------------------------------------------
	WObjectProperties.
-----------------------------------------------------------------------------*/

// Object properties root.
class WINDOW_API FObjectsItem : public FPropertyItemBase
{
public:
	// Variables.
	UBOOL ByCategory;
	TArray<UObject*> _Objects;

	// Structors.
	FObjectsItem();
	FObjectsItem( WPropertiesBase* InOwnerProperties, FTreeItem* InParent, DWORD InFlagMask, const TCHAR* InCaption, UBOOL InByCategory );

	// FTreeItem interface.
	void Serialize( FArchive& Ar );
	BYTE* GetAddress( UProperty* Property, BYTE* Base, INT Offset );
	BYTE* GetReadAddress( UProperty* Property, INT Offset );
	void SetProperty( UProperty* Property, INT Offset, const TCHAR* Value );
	void Expand();

	// FHeaderItem interface.
	FString GetCaption() const;

	// FObjectsItem interface.
	virtual void SetObjects( UObject** InObjects, INT Count );
};

// Multiple selection object properties.
class WINDOW_API WObjectProperties : public WProperties
{
	W_DECLARE_CLASS(WObjectProperties,WProperties,CLASS_Transient);
	DECLARE_WINDOWCLASS(WObjectProperties,WProperties,Window)

	// Variables.
	FObjectsItem Root;

	// Structors.
	WObjectProperties();
	WObjectProperties( FName InPersistentName, DWORD InFlagMask, const TCHAR* InCaption, WWindow* InOwnerWindow, UBOOL InByCategory );

	// WPropertiesBase interface.
	FTreeItem* GetRoot();
};

/*-----------------------------------------------------------------------------
	WClassProperties.
-----------------------------------------------------------------------------*/

// Object properties root.
class WINDOW_API FClassItem : public FPropertyItemBase
{
public:
	// Structors.
	FClassItem();
	FClassItem( WPropertiesBase* InOwnerProperties, FTreeItem* InParent, DWORD InFlagMask, const TCHAR* InCaption, UClass* InBaseClass );

	// FTreeItem interface.
	BYTE* GetReadAddress( UProperty* Property, INT Offset );
	void SetProperty( UProperty* Property, INT Offset, const TCHAR* Value );
	void Expand();
};

// Multiple selection object properties.
class WINDOW_API WClassProperties : public WProperties
{
	W_DECLARE_CLASS(WClassProperties,WProperties,CLASS_Transient);
	DECLARE_WINDOWCLASS(WClassProperties,WProperties,Window)

	// Variables.
	FClassItem Root;

	// Structors.
	WClassProperties();
	WClassProperties( FName InPersistentName, DWORD InFlagMask, const TCHAR* InCaption, UClass* InBaseClass );

	// WPropertiesBase interface.
	FTreeItem* GetRoot();
};

/*-----------------------------------------------------------------------------
	WConfigProperties.
-----------------------------------------------------------------------------*/

// Object configuration header.
class WINDOW_API FObjectConfigItem : public FPropertyItemBase
{
public:
	// Variables.
	FString  ClassName;
	FName    CategoryFilter;
	UClass*  Class;
	UBOOL	 Failed;
	UBOOL    Immediate;

	// Structors.
	FObjectConfigItem( WPropertiesBase* InOwnerProperties, FTreeItem* InParent, const TCHAR* InCaption, const TCHAR* InClass, UBOOL InImmediate, FName InCategoryFilter );

	// FTreeItem interface.
	BYTE* GetReadAddress( UProperty* Property, INT Offset );
	void SetProperty( UProperty* Property, INT Offset, const TCHAR* Value );
	void OnResetToDefaultsButton();
	void OnItemSetFocus();
	void Expand();

	// FObjectConfigItem interface.
	void LazyLoadClass();

	// FTreeItem interface.
	void Serialize( FArchive& Ar );
};

// An configuration list item.
class WINDOW_API FConfigItem : public FHeaderItem
{
public:
	// Variables.
	FPreferencesInfo Prefs;

	// Constructors.
	FConfigItem();
	FConfigItem( const FPreferencesInfo& InPrefs, WPropertiesBase* InOwnerProperties, FTreeItem* InParent );

	// FTreeItem interface.
	QWORD GetId() const;
	virtual FString GetCaption() const;
	void Expand();
	void Collapse();
};

// Configuration properties.
class WINDOW_API WConfigProperties : public WProperties
{
	W_DECLARE_CLASS(WConfigProperties,WProperties,CLASS_Transient);
	DECLARE_WINDOWCLASS(WConfigProperties,WProperties,Window)

	// Variables.
	FConfigItem Root;

	// Structors.
	WConfigProperties();
	WConfigProperties( FName InPersistentName, const TCHAR* InTitle );

	// WPropertiesBase interface.
	FTreeItem* GetRoot();
};

/*-----------------------------------------------------------------------------
	WWizardPage.
-----------------------------------------------------------------------------*/

// A wizard page.
class WINDOW_API WWizardPage : public WDialog
{
	W_DECLARE_ABSTRACT_CLASS(WWizardPage,WDialog,CLASS_Transient);
	DECLARE_WINDOWCLASS(WWizardPage,WDialog,Window)

	// Variables.
	WWizardDialog* Owner;

	// Constructor.
	WWizardPage();
	WWizardPage( const TCHAR* PageName, INT ControlId, WWizardDialog* InOwner );

	// WWizardPage interface.
	virtual void OnCurrent();
	virtual WWizardPage* GetNext();
	virtual const TCHAR* GetBackText();
	virtual const TCHAR* GetNextText();
	virtual const TCHAR* GetFinishText();
	virtual const TCHAR* GetCancelText();
	virtual UBOOL GetShow();
	virtual void OnCancel();
};

/*-----------------------------------------------------------------------------
	WWizardDialog.
-----------------------------------------------------------------------------*/

// The wizard frame dialog.
class WINDOW_API WWizardDialog : public WDialog
{
	W_DECLARE_CLASS(WWizardDialog,WDialog,CLASS_Transient);
	DECLARE_WINDOWCLASS(WWizardDialog,WDialog,Window)

	// Variables.
	WCoolButton BackButton;
	WCoolButton NextButton;
	WCoolButton FinishButton;
	WCoolButton CancelButton;
	WLabel PageHolder;
	TArray<WWizardPage*> Pages;
	WWizardPage* CurrentPage;

	// Constructor.
	WWizardDialog();

	// WDialog interface.
	void OnInitDialog();

	// WWizardDialog interface.
	virtual void Advance( WWizardPage* NewPage );
	virtual void RefreshPage();
	virtual void OnDestroy();
	virtual void OnBack();
	virtual void OnNext();
	virtual void OnFinish();
	virtual void OnCancel();
	void OnClose();
};

/*-----------------------------------------------------------------------------
	The End.
-----------------------------------------------------------------------------*/
