/*=============================================================================
	UnFile.h: General-purpose file utilities.
	Copyright 1997-1999 Epic Games, Inc. All Rights Reserved.
=============================================================================*/

/*-----------------------------------------------------------------------------
	Global variables.
-----------------------------------------------------------------------------*/

// Global variables.
CORE_API extern DWORD GCRCTable[];

/*----------------------------------------------------------------------------
	Byte order conversion.
----------------------------------------------------------------------------*/

// Bitfields.
#ifndef NEXT_BITFIELD
	#if __INTEL_BYTE_ORDER__
		#define NEXT_BITFIELD(b) ((b)<<1)
		#define FIRST_BITFIELD   (1)
		#define INTEL_ORDER(x)   (x)
	#else
		#define NEXT_BITFIELD(b) ((b)>>1)
		#define FIRST_BITFIELD   (0x80000000)
		#define INTEL_ORDER(x)   (((x)>>24) + (((x)>>8)&0xff00) + (((x)<<8)&0xff0000) + ((x)<<24))
	#endif
#endif

/*-----------------------------------------------------------------------------
	Stats.
-----------------------------------------------------------------------------*/

#if STATS
	#define STAT(x) x
#else
	#define STAT(x)
#endif

/*-----------------------------------------------------------------------------
	Global init and exit.
-----------------------------------------------------------------------------*/

CORE_API void appInit();
CORE_API void appPreExit();
CORE_API void appExit();

/*-----------------------------------------------------------------------------
	Logging and critical errors.
-----------------------------------------------------------------------------*/

CORE_API void appRequestExit();
CORE_API void VARARGS appFailAssert( const ANSICHAR* Expr, const ANSICHAR* File, INT Line );
CORE_API void VARARGS appUnwindf( const TCHAR* Fmt, ... );
CORE_API void appHandleError();

#define debugf				GOut.Logf
#define appErrorf			GError.Logf

#if DO_GUARD_SLOW
	#define debugfSlow		GOut.Logf
	#define appErrorfSlow	GError.Logf
#else
	#define debugfSlow		GNull.Logf
	#define appErrorfSlow	GNull.Logf
#endif

/*-----------------------------------------------------------------------------
	Misc.
-----------------------------------------------------------------------------*/

CORE_API void* appGetDllHandle( const TCHAR* DllName );
CORE_API void appFreeDllHandle( void* DllHandle );
CORE_API void* appGetDllExport( void* DllHandle, const TCHAR* ExportName );
CORE_API void appLaunchURL( const TCHAR* URL, const TCHAR* Parms=NULL, FString* Error=NULL );
CORE_API void appEnableFastMath( UBOOL Enable );
CORE_API class FGuid appCreateGuid();
CORE_API void appCreateTempFilename( const TCHAR* Path, TCHAR* Result256 );
CORE_API void appCleanFileCache();
CORE_API UBOOL appFindPackageFile( const TCHAR* In, const FGuid* Guid, TCHAR* Out );

/*-----------------------------------------------------------------------------
	Clipboard.
-----------------------------------------------------------------------------*/

CORE_API void ClipboardCopy( const TCHAR* Str );
CORE_API void ClipboardPaste( FString& Str );

/*-----------------------------------------------------------------------------
	Files.
-----------------------------------------------------------------------------*/

struct FFileMapInfo;

CORE_API INT appFSize( const TCHAR* Filename );
CORE_API UBOOL appMkdir( const TCHAR* Path );
CORE_API UBOOL appMkdirTree( const TCHAR* Path );
CORE_API UBOOL appMoveFile( const TCHAR* Dest, const TCHAR* Src );
CORE_API DWORD appMsize( void *Data );
CORE_API UBOOL appMapReadOnlyFile( const TCHAR* Filename, FFileMapInfo& FileMap );
CORE_API UBOOL appUnlink( const TCHAR* Filename );
CORE_API void appCloseFileMap( FFileMapInfo& FileMap );
CORE_API UBOOL appChdir( const TCHAR* Path );
CORE_API void appCleanFileCache();
CORE_API UBOOL appFtell(struct _iobuf *);
CORE_API UBOOL appFwrite(void const *,int,int,struct _iobuf *);
CORE_API void appHeapCheck();
CORE_API UBOOL appCopyFile( const TCHAR* Dest, const TCHAR* Src );;

/*-----------------------------------------------------------------------------
	Config.
-----------------------------------------------------------------------------*/

CORE_API UBOOL GetConfigBool( const TCHAR* Section, const TCHAR* Key, UBOOL& Value, const TCHAR* Filename=NULL );
CORE_API UBOOL GetConfigFloat( const TCHAR* Section, const TCHAR* Key, FLOAT& Value, const TCHAR* Filename=NULL );
CORE_API UBOOL GetConfigInt( const TCHAR* Section, const TCHAR* Key, INT& Value, const TCHAR* Filename=NULL );
CORE_API UBOOL GetConfigSection( const TCHAR* Section, TCHAR* Value, INT Size, const TCHAR* Filename=NULL );
CORE_API const TCHAR* GetConfigStr( const TCHAR* Section, const TCHAR* Key, const TCHAR* Filename=NULL );
CORE_API UBOOL GetConfigString( const TCHAR* Section, const TCHAR* Key, class FString& Str, const TCHAR* Filename=NULL );
CORE_API UBOOL GetConfigString( const TCHAR* Section, const TCHAR* Key, TCHAR* Value, INT Size, const TCHAR* Filename=NULL );

CORE_API void SetConfigBool( const TCHAR* Section, const TCHAR* Key, UBOOL Value, const TCHAR* Filename=NULL );
CORE_API void SetConfigFloat( const TCHAR* Section, const TCHAR* Key, FLOAT Value, const TCHAR* Filename=NULL );
CORE_API void SetConfigInt( const TCHAR* Section, const TCHAR* Key, INT Value, const TCHAR* Filename=NULL );
CORE_API void SetConfigString( const TCHAR* Section, const TCHAR* Key, const TCHAR* Value, const TCHAR* Filename=NULL );

CORE_API void EmptyConfigSection( const TCHAR* Section, const TCHAR* Filename=NULL );
CORE_API void ConfigFlush( UBOOL Read );

CORE_API void appDumpAllocs();

/*-----------------------------------------------------------------------------
	Guard macros for call stack display.
-----------------------------------------------------------------------------*/

//
// guard/unguardf/unguard macros.
// For showing calling stack when errors occur in major functions.
// Meant to be enabled in release builds.
//
#if defined(_KHGDEBUG) || !DO_GUARD
	#define guard(func)			{static const TCHAR __FUNC_NAME__[]=TEXT(#func);
	#define unguard				}
	#define unguardf(msg)		}
#else
	#define guard(func)			{static const TCHAR __FUNC_NAME__[]=TEXT(#func); try{
	#define unguard				}catch(TCHAR*Err){throw Err;}catch(...){appUnwindf(TEXT("%s"),__FUNC_NAME__); throw;}}
	#define unguardf(msg)		}catch(TCHAR*Err){throw Err;}catch(...){appUnwindf(TEXT("%s"),__FUNC_NAME__); appUnwindf msg; throw;}}
#endif

//
// guardSlow/unguardfSlow/unguardSlow macros.
// For showing calling stack when errors occur in performance-critical functions.
// Meant to be disabled in release builds.
//
#if defined(_KHGDEBUG) || !DO_GUARD || !DO_GUARD_SLOW
	#define guardSlow(func)		{
	#define unguardfSlow(msg)	}
	#define unguardSlow			}
	#define unguardfSlow(msg)	}
#else
	#define guardSlow(func)		guard(func)
	#define unguardSlow			unguard
	#define unguardfSlow(msg)	unguardf(msg)
#endif

//
// For throwing string-exceptions which safely propagate through guard/unguard.
//
CORE_API void VARARGS appThrowf( const TCHAR* Fmt, ... );

/*-----------------------------------------------------------------------------
	Check macros for assertions.
-----------------------------------------------------------------------------*/

//
// "check" expressions are only evaluated if enabled.
// "verify" expressions are always evaluated, but only cause an error if enabled.
//
#if DO_CHECK
	#define check(expr)  {if(!(expr)) appFailAssert( #expr, __FILE__, __LINE__ );}
	#define verify(expr) {if(!(expr)) appFailAssert( #expr, __FILE__, __LINE__ );}
#else
	#define check(expr)
	#define verify(expr) expr
#endif

//
// Check for development only.
//
#if DO_GUARD_SLOW
	#define checkSlow(expr)  {if(!(expr)) appFailAssert( #expr, __FILE__, __LINE__ );}
	#define verifySlow(expr) {if(!(expr)) appFailAssert( #expr, __FILE__, __LINE__ );}
#else
	#define checkSlow(expr)
	#define verifySlow(expr) if(expr){}
#endif

/*-----------------------------------------------------------------------------
	Timing macros.
-----------------------------------------------------------------------------*/

//
// Normal timing.
//
#define clock(Timer)   {Timer -= appCycles();}
#define unclock(Timer) {Timer += appCycles()-34;}

//
// Performance critical timing.
//
#if DO_CLOCK_SLOW
	#define clockSlow(Timer)   {Timer-=appCycles();}
	#define unclockSlow(Timer) {Timer+=appCycles();}
#else
	#define clockSlow(Timer)
	#define unclockSlow(Timer)
#endif

/*-----------------------------------------------------------------------------
	Localization.
-----------------------------------------------------------------------------*/

CORE_API const TCHAR* Localize( const TCHAR* Section, const TCHAR* Key, const TCHAR* Package=GPackage, const TCHAR* LangExt=NULL, UBOOL Optional=0 );
CORE_API const TCHAR* LocalizeError( const TCHAR* Key, const TCHAR* Package=GPackage, const TCHAR* LangExt=NULL );
CORE_API const TCHAR* LocalizeProgress( const TCHAR* Key, const TCHAR* Package=GPackage, const TCHAR* LangExt=NULL );
CORE_API const TCHAR* LocalizeQuery( const TCHAR* Key, const TCHAR* Package=GPackage, const TCHAR* LangExt=NULL );
CORE_API const TCHAR* LocalizeGeneral( const TCHAR* Key, const TCHAR* Package=GPackage, const TCHAR* LangExt=NULL );

#if UNICODE
	CORE_API const TCHAR* Localize( const ANSICHAR* Section, const ANSICHAR* Key, const TCHAR* Package=GPackage, const TCHAR* LangExt=NULL, UBOOL Optional=0 );
	CORE_API const TCHAR* LocalizeError( const ANSICHAR* Key, const TCHAR* Package=GPackage, const TCHAR* LangExt=NULL );
	CORE_API const TCHAR* LocalizeProgress( const ANSICHAR* Key, const TCHAR* Package=GPackage, const TCHAR* LangExt=NULL );
	CORE_API const TCHAR* LocalizeQuery( const ANSICHAR* Key, const TCHAR* Package=GPackage, const TCHAR* LangExt=NULL );
	CORE_API const TCHAR* LocalizeGeneral( const ANSICHAR* Key, const TCHAR* Package=GPackage, const TCHAR* LangExt=NULL );
#endif

/*-----------------------------------------------------------------------------
	File functions.
-----------------------------------------------------------------------------*/

// File utilities.
CORE_API const TCHAR* appFExt( const TCHAR* Filename );

/*-----------------------------------------------------------------------------
	OS functions.
-----------------------------------------------------------------------------*/

CORE_API const TCHAR* appCmdLine();
CORE_API const TCHAR* appBaseDir();
CORE_API const TCHAR* appPackage();

/*-----------------------------------------------------------------------------
	Timing functions.
-----------------------------------------------------------------------------*/

#if !DEFINED_appCycles
CORE_API DWORD appCycles();
#endif

#if !DEFINED_appSeconds
CORE_API DOUBLE appSeconds();
#endif

CORE_API void appSystemTime( INT& Year, INT& Month, INT& DayOfWeek, INT& Day, INT& Hour, INT& Min, INT& Sec, INT& MSec );
CORE_API const TCHAR* appTimestamp();
CORE_API DOUBLE appSecondsSlow();

/*-----------------------------------------------------------------------------
	Character type functions.
-----------------------------------------------------------------------------*/

inline TCHAR appToUpper( TCHAR c )
{
	return (c<'a' || c>'z') ? (c) : (c+'A'-'a');
}
inline TCHAR appToLower( TCHAR c )
{
	return (c<'A' || c>'Z') ? (c) : (c+'a'-'A');
}
inline UBOOL appIsAlpha( TCHAR c )
{
	return (c>='a' && c<='z') || (c>='A' && c<='Z');
}
inline UBOOL appIsDigit( TCHAR c )
{
	return c>='0' && c<='9';
}
inline UBOOL appIsAlnum( TCHAR c )
{
	return (c>='a' && c<='z') || (c>='A' && c<='Z') || (c>='0' && c<='9');
}

/*-----------------------------------------------------------------------------
	String functions.
-----------------------------------------------------------------------------*/

CORE_API const ANSICHAR* appToAnsi( const TCHAR* Str );
CORE_API const UNICHAR* appToUnicode( const TCHAR* Str );
CORE_API const TCHAR* appFromAnsi( const ANSICHAR* Str );
CORE_API const TCHAR* appFromUnicode( const UNICHAR* Str );

CORE_API TCHAR* appStrcpy( TCHAR* Dest, const TCHAR* Src );
CORE_API INT appStrlen( const TCHAR* String );
CORE_API TCHAR* appStrstr( const TCHAR* String, const TCHAR* Find );
CORE_API TCHAR* appStrchr( const TCHAR* String, INT c );
CORE_API TCHAR* appStrcat( TCHAR* Dest, const TCHAR* Src );
CORE_API INT appStrcmp( const TCHAR* String1, const TCHAR* String2 );
CORE_API INT appStricmp( const TCHAR* String1, const TCHAR* String2 );
CORE_API TCHAR* appStaticString1024();

CORE_API const TCHAR* appSpc( int Num );
CORE_API TCHAR* appStrncpy( TCHAR* Dest, const TCHAR* Src, int Max);
CORE_API TCHAR* appStrncat( TCHAR* Dest, const TCHAR* Src, int Max);
CORE_API TCHAR* appStrupr( TCHAR* String );
CORE_API const TCHAR* appStrfind(const TCHAR* Str, const TCHAR* Find);
CORE_API DWORD appStrCrc( const TCHAR* Data );
CORE_API DWORD appStrCrcCaps( const TCHAR* Data );
CORE_API INT appAtoi( const TCHAR* Str );
CORE_API FLOAT appAtof( const TCHAR* Str );
CORE_API INT appStrtoi( const TCHAR* Start, TCHAR** End, INT Base );
CORE_API INT appStrnicmp( const TCHAR* A, const TCHAR* B, INT Count );
CORE_API INT appSprintf( TCHAR* Dest, const TCHAR* Fmt, ... );
CORE_API INT appGetVarArgs( TCHAR* Dest, const TCHAR*& Fmt );

typedef int QSORT_RETURN;
typedef QSORT_RETURN(CDECL* QSORT_COMPARE)( const void* A, const void* B );
CORE_API void appQsort( void* Base, INT Num, INT Width, QSORT_COMPARE Compare );

//
// Case insensitive string hash function.
//
inline DWORD appStrihash( const TCHAR* Data )
{
	DWORD Hash=0;
	while( *Data )
	{
		TCHAR Ch = appToUpper(*Data++);
		BYTE  B  = Ch;
		Hash     = ((Hash >> 8) & 0x00FFFFFF) ^ GCRCTable[(Hash ^ B) & 0x000000FF];
#if UNICODE
		B        = Ch>>8;
		Hash     = ((Hash >> 8) & 0x00FFFFFF) ^ GCRCTable[(Hash ^ B) & 0x000000FF];
#endif
	}
	return Hash;
}

/*-----------------------------------------------------------------------------
	Parsing functions.
-----------------------------------------------------------------------------*/

CORE_API UBOOL ParseCommand( const TCHAR** Stream, const TCHAR* Match );
CORE_API UBOOL Parse( const TCHAR* Stream, const TCHAR* Match, class FName& Name );
CORE_API UBOOL Parse( const TCHAR* Stream, const TCHAR* Match, DWORD& Value );
CORE_API UBOOL Parse( const TCHAR* Stream, const TCHAR* Match, class FGuid& Guid );
CORE_API UBOOL Parse( const TCHAR* Stream, const TCHAR* Match, TCHAR* Value, INT MaxLen );
CORE_API UBOOL Parse( const TCHAR* Stream, const TCHAR* Match, BYTE& Value );
CORE_API UBOOL Parse( const TCHAR* Stream, const TCHAR* Match, SBYTE& Value );
CORE_API UBOOL Parse( const TCHAR* Stream, const TCHAR* Match, _WORD& Value );
CORE_API UBOOL Parse( const TCHAR* Stream, const TCHAR* Match, SWORD& Value );
CORE_API UBOOL Parse( const TCHAR* Stream, const TCHAR* Match, FLOAT& Value );
CORE_API UBOOL Parse( const TCHAR* Stream, const TCHAR* Match, INT& Value );
CORE_API UBOOL Parse( const TCHAR* Stream, const TCHAR* Match, QWORD& Value );
CORE_API UBOOL Parse( const TCHAR* Stream, const TCHAR* Match, SQWORD& Value );
CORE_API UBOOL ParseUBOOL( const TCHAR* Stream, const TCHAR* Match, UBOOL& OnOff );
CORE_API UBOOL ParseObject( const TCHAR* Stream, const TCHAR* Match, class UClass* Type, class UObject*& DestRes, class UObject* InParent );
CORE_API UBOOL ParseLine( const TCHAR** Stream, TCHAR* Result, INT MaxLen, UBOOL Exact=0 );
CORE_API UBOOL ParseLine( const TCHAR** Stream, FString& Resultd, UBOOL Exact=0 );
CORE_API UBOOL ParseToken( const TCHAR*& Str, TCHAR* Result, INT MaxLen, UBOOL UseEscape );
CORE_API UBOOL ParseToken( const TCHAR*& Str, FString& Arg, UBOOL UseEscape );
CORE_API FString ParseToken( const TCHAR*& Str, UBOOL UseEscape );
CORE_API void ParseNext( const TCHAR** Stream );
CORE_API UBOOL ParseParam( const TCHAR* Stream, const TCHAR* Param );

/*-----------------------------------------------------------------------------
	Math functions.
-----------------------------------------------------------------------------*/

CORE_API DOUBLE appExp( DOUBLE Value );
CORE_API DOUBLE appLoge( DOUBLE Value );
CORE_API DOUBLE appFmod( DOUBLE A, DOUBLE B );
CORE_API DOUBLE appSin( DOUBLE Value );
CORE_API DOUBLE appCos( DOUBLE Value );
CORE_API DOUBLE appTan( DOUBLE Value );
CORE_API DOUBLE appAtan( DOUBLE Value );
CORE_API DOUBLE appAtan2( DOUBLE Y, FLOAT X );
CORE_API DOUBLE appSqrt( DOUBLE Value );
CORE_API DOUBLE appPow( DOUBLE A, DOUBLE B );
CORE_API UBOOL appIsNan( DOUBLE Value );
CORE_API INT appRand();
CORE_API FLOAT appFrand();

#if !DEFINED_appRound
CORE_API INT appRound( FLOAT Value );
#endif

#if !DEFINED_appFloor
CORE_API INT appFloor( FLOAT Value );
#endif

#if !DEFINED_appCeil
CORE_API INT appCeil( FLOAT Value );
#endif

/*-----------------------------------------------------------------------------
	Array functions.
-----------------------------------------------------------------------------*/

// Core functions depending on TArray and FString.
CORE_API UBOOL appLoadFileToArray( TArray<BYTE>& Result, const TCHAR* Filename );
CORE_API UBOOL appLoadFileToString( FString& Result, const TCHAR* Filename );
CORE_API UBOOL appSaveArrayToFile( const TArray<BYTE>& Array, const TCHAR* Filename );
CORE_API UBOOL appSaveStringToFile( const FString& String, const TCHAR* Filename );

/*-----------------------------------------------------------------------------
	Memory functions.
-----------------------------------------------------------------------------*/

CORE_API void* appMemmove( void* Dest, const void* Src, INT Count );
CORE_API INT appMemcmp( const void* Buf1, const void* Buf2, INT Count );
CORE_API UBOOL appMemIsZero( const void* V, int Count );
CORE_API DWORD appMemCrc( const void* Data, INT Length, DWORD CRC=0 );
CORE_API void appMemswap( void* Ptr1, void* Ptr2, DWORD Size );
CORE_API void appMemset( void* Dest, INT C, INT Count );

#ifndef DEFINED_appMemcpy
CORE_API void appMemcpy( void* Dest, const void* Src, INT Count );
#endif

#ifndef DEFINED_appMemzero
CORE_API void appMemzero( void* Dest, INT Count );
#endif

//
// C style memory allocation stubs.
//
CORE_API void* appMalloc( DWORD Count, const TCHAR* Tag );
CORE_API void appFree( void* Original );
CORE_API void* appRealloc( void* Original, DWORD Count, const TCHAR* Tag );

//
// C++ style memory allocation.
//
inline void* operator new( unsigned int Size, const TCHAR* Tag )
{
	//guardSlow(new);
	return appMalloc( Size, Tag );
	//unguardSlow;
}
inline void* operator new( unsigned int Size )
{
	//guardSlow(new);
	return appMalloc( Size, TEXT("new") );
	//unguardSlow;
}
inline void operator delete( void* Ptr )
{
	//guardSlow(delete);
	appFree( Ptr );
	//unguardSlow;
}

#if PLATFORM_NEEDS_ARRAY_NEW
inline void* operator new[]( unsigned int Size, const TCHAR* Tag )
{
	//guardSlow(new);
	return appMalloc( Size, Tag );
	//unguardSlow;
}
inline void* operator new[]( unsigned int Size )
{
	//guardSlow(new);
	return appMalloc( Size, TEXT("new") );
	//unguardSlow;
}
inline void operator delete[]( void* Ptr )
{
	//guardSlow(delete);
	appFree( Ptr );
	//unguardSlow;
}
#endif

/*-----------------------------------------------------------------------------
	The End.
-----------------------------------------------------------------------------*/
