/*=============================================================================
	UnCon.h: UConsole game-specific definition
	Copyright 1997-1999 Epic Games, Inc. All Rights Reserved.

	Contains routines for: Messages, menus, status bar
=============================================================================*/

/*------------------------------------------------------------------------------
	UConsole definition.
------------------------------------------------------------------------------*/

//
// Viewport console.
//
class ENGINE_API UConsole : public UObject, public FOutputDevice
{
	DECLARE_CLASS(UConsole,UObject,CLASS_Transient)

	// Constructor.
	UConsole();

	// UConsole interface.
	virtual void _Init( UViewport* Viewport );
	virtual void PreRender( FSceneNode* Frame );
	virtual void PostRender( FSceneNode* Frame );
	virtual void Serialize( void* Data, INT Count, EName MsgType );;
	virtual UBOOL GetDrawWorld();

	// Natives.
  void execConsoleCommand( FFrame& Stack, BYTE*& Result );

	// Script events.
  void eventPostRender(class UCanvas* C)
  {
      struct {class UCanvas* C; } Parms;
      Parms.C=C;
      ProcessEvent(FindFunctionChecked(ENGINE_PostRender),&Parms);
  }
  void eventPreRender(class UCanvas* C)
  {
      struct {class UCanvas* C; } Parms;
      Parms.C=C;
      ProcessEvent(FindFunctionChecked(ENGINE_PreRender),&Parms);
  }
  DWORD eventKeyEvent(BYTE Key, BYTE Action, FLOAT Delta)
  {
      struct {BYTE Key; BYTE Action; FLOAT Delta; DWORD ReturnValue; } Parms;
      Parms.Key=Key;
      Parms.Action=Action;
      Parms.Delta=Delta;
      Parms.ReturnValue=0;
      ProcessEvent(FindFunctionChecked(ENGINE_KeyEvent),&Parms);
      return Parms.ReturnValue;
  }
  DWORD eventKeyType(BYTE Key)
  {
      struct {BYTE Key; DWORD ReturnValue; } Parms;
      Parms.Key=Key;
      Parms.ReturnValue=0;
      ProcessEvent(FindFunctionChecked(ENGINE_KeyType),&Parms);
      return Parms.ReturnValue;
  }
  void eventMessage(const TCHAR* Msg, FName N)
  {
      struct {TCHAR Msg[240]; FName N; } Parms;
      appStrncpy(Parms.Msg,Msg,240);
      Parms.N=N;
      ProcessEvent(FindFunctionChecked(ENGINE_Message),&Parms);
  }
private:
	// Constants.
	enum {MAX_TEXTMSGSIZE = 128};
	enum {MAX_BORDER     = 6};
	enum {MAX_LINES		 = 64};
	enum {MAX_HISTORY	 = 16};

	// Variables.
	UViewport*		Viewport;
	INT				HistoryTop;
	INT				HistoryBot;
	INT				HistoryCur;
  TCHAR TypedStr[MAX_TEXTMSGSIZE];
  TCHAR History[MAX_HISTORY][MAX_TEXTMSGSIZE];
	INT				Scrollback;
	INT				NumLines;
	INT				TopLine;
	INT				TextLines;
  FName MsgType;
  FLOAT MsgTime;
  TCHAR MsgText[MAX_LINES][MAX_TEXTMSGSIZE];
	INT				BorderSize;
	INT				ConsoleLines;
	INT				BorderLines;
	INT				BorderPixels;
	FLOAT			ConsolePos;
	FLOAT			ConsoleDest;
	UTexture*		ConBackground;
	UTexture*		Border;
	BITFIELD		bNoStuff:1;
};

/*------------------------------------------------------------------------------
	The End.
------------------------------------------------------------------------------*/
