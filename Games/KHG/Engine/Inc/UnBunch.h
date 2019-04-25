/*=============================================================================
	UnBunch.h: Unreal bunch class.
	Copyright 1997-1999 Epic Games, Inc. All Rights Reserved.

	Revision history:
		* Created by Tim Sweeney
=============================================================================*/

//
// A bunch of data to send.
//
class ENGINE_API FOutBunch : public FBitWriter
{
public:
	// Variables.
	FOutBunch*		Next;
	FChannel*		Channel;
	DOUBLE			Time;
	UBOOL			ReceivedAck;
	_WORD			ChIndex;
	_WORD			ChSequence;
	_WORD			ChType;
	_WORD			PacketId;
	BYTE			bOpen;
	BYTE			bClose;
	BYTE			bReliable;

	// Functions.
	FOutBunch();
	FOutBunch( FChannel* InChannel, UBOOL bClose );
	UBOOL SendProperty( UObject* Object, UProperty* Property, INT ArrayIndex, BYTE* Data, BYTE* Defaults );
	UBOOL SendObject( UObject* Object );
	FArchive& operator<<( FName& Name );
	FArchive& operator<<( UObject*& Object );
};

//
// A bunch of data received from a channel.
//
class ENGINE_API FInBunch : public FBitReader
{
public:
	// Variables.
	SQWORD			PacketFullId;
	FInBunch*		Next;
	UNetConnection*	Connection;
	_WORD			ChIndex;
	_WORD			ChSequence;
	_WORD			ChType;
	BYTE			bOpen;
	BYTE			bClose;
	BYTE			bReliable;

	// Functions.
	FInBunch( UNetConnection* InConnection )
	:	FBitReader	()
	,	Next		( NULL )
	,	Connection  ( InConnection )
	{}
	UBOOL ReceiveProperty( UProperty* Property, INT Element, BYTE* Data, BYTE* Recent );
	FArchive& operator<<( FName& Name );
	FArchive& operator<<( UObject*& Object );
};

/*-----------------------------------------------------------------------------
	The End.
-----------------------------------------------------------------------------*/
