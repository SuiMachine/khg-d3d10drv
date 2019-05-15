/*=============================================================================
	FOutputDeviceWindowsError.h: Windows error message outputter.
	Copyright 1997-1999 Epic Games, Inc. All Rights Reserved.

	Revision history:
		* Created by Tim Sweeney
=============================================================================*/

//
// Handle a critical error.
//
class FOutputDeviceWindowsError : public FErrorOut
{
public:
	void Serialize( const TCHAR* Msg, enum EName Event )
	{
#ifdef _KHGDEBUG
		// Just display info and break the debugger.
  		debugf( NAME_Critical, TEXT("appError called while debugging:") );
		debugf( NAME_Critical, Msg );
		UObject::StaticShutdownAfterError();
  		debugf( NAME_Critical, TEXT("Breaking debugger") );
		DebugBreak();
#else
		INT Error = GetLastError();
		if( !GIsCriticalError )
		{
			// First appError.
			GIsCriticalError = 1;
			debugf( NAME_Critical, TEXT("appError called:") );
			debugf( NAME_Critical, TEXT("%s"), Msg );

			// Windows error.
			//debugf( NAME_Critical, TEXT("Windows GetLastError: %s (%i)"), appGetSystemErrorMessage(Error), Error );
			debugf( NAME_Critical, TEXT("Windows GetLastError: (%i)"), Error );

			// Shut down.
			appStrncpy( GErrorHist, Msg, ARRAY_COUNT(GErrorHist) );
			appStrncat( GErrorHist, TEXT("\r\n\r\n"), ARRAY_COUNT(GErrorHist) );
			UObject::StaticShutdownAfterError();
			if( GIsGuarded )
			{
				appStrncat( GErrorHist, GIsGuarded ? LocalizeError("History",TEXT("Core")) : TEXT("History: "), ARRAY_COUNT(GErrorHist) );
				appStrncat( GErrorHist, TEXT(": "), ARRAY_COUNT(GErrorHist) );
			}
			else appHandleError();
		}
		else debugf( NAME_Critical, TEXT("Error reentered: %s"), Msg );

		// Propagate the error or exit.
		if( GIsGuarded )
			throw( 1 );
		else
			appRequestExit();
#endif
	}
};

/*-----------------------------------------------------------------------------
	The End.
-----------------------------------------------------------------------------*/
