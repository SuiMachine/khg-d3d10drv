/*=============================================================================
	UnChan.h: Unreal datachannel class.
	Copyright 1997-1999 Epic Games, Inc. All Rights Reserved.

	Revision history:
		* Created by Tim Sweeney
=============================================================================*/

/*-----------------------------------------------------------------------------
	Channel type registry.
-----------------------------------------------------------------------------*/

// Global channel type functions.
BYTE GAutoRegisterChannel( INT ChType, FChannel* (*Constructor)( UNetConnection* InConnection, INT InChIndex, UBOOL InOpenedLocally ) );
UBOOL GIsKnownChannelType( INT Type );

// Goes inside class definition,
#define DECLARE_CHTYPE(chtype,chclass) \
public: \
	enum {ChannelType=chtype}; \
	static FChannel* Construct( UNetConnection* InConnection, INT InChIndex, UBOOL InOpenedLocally ) \
		{return new chclass( InConnection, InChIndex, InOpenedLocally );}

// Channel type autoregistration macro.
#define IMPLEMENT_CHTYPE(chclass) \
	BYTE autoregisterI##chclass = GAutoRegisterChannel( chclass::ChannelType, chclass::Construct );

// Channel table.
extern ENGINE_API FChannel* (*GChannelConstructors[CHTYPE_MAX])( UNetConnection* InConnection, INT InChIndex, UBOOL InOpenedLocally );

/*-----------------------------------------------------------------------------
	FChannel base class.
-----------------------------------------------------------------------------*/

//
// Base class of communication channels.
//
class ENGINE_API FChannel
{
public:
	// Variables.
	UNetConnection*	Connection;		// Owner connection.
	UBOOL			OpenAcked;		// Whether spawn has been acknowledged.
	UBOOL			Closing;		// State of the channel.
	INT             ChIndex;		// Index of this channel.
	INT				OpenedLocally;	// Whether channel was opened locally or by remote.
	INT				OpenPacketId;	// Packet the spawn message was sent in.
	UBOOL			OpenTemporary;	// Opened temporarily.
	EChannelType	ChType;			// Type of this channel.
	INT				NumInRec;		// Number of packets in InRec.
	INT				NumOutRec;		// Number of packets in OutRec.
	INT				NegotiatedVer;	// Negotiated version of engine = Min(client version, server version).
	FInBunch*		InRec;			// Incoming data with queued dependencies.
	FOutBunch*		OutRec;			// Outgoing reliable unacked data.

	// Constructor.
	FChannel( INT InChType, UNetConnection* InConnection, INT InChannelIndex, INT InOpenedLocally );
	virtual ~FChannel();

	// FChannel interface.
	virtual void SetClosingFlag();
	virtual void Close();
	virtual FString Describe();
	virtual void ReceivedBunch( FInBunch& Bunch )=0;
	virtual void Tick();

	// General channel functions.
	void ReceivedAcks();
	UBOOL ReceivedSequencedBunch( FInBunch& Bunch );
	void ReceivedRawBunch( FInBunch& Bunch );
	_WORD SendBunch( FOutBunch* Bunch, UBOOL Merge );
	UBOOL IsNetReady( UBOOL Saturate );
	void AssertInSequenced();
	INT MaxSendBytes();
};

/*-----------------------------------------------------------------------------
	FControlChannel base class.
-----------------------------------------------------------------------------*/

//
// A channel for exchanging text.
//
class ENGINE_API FControlChannel : public FChannel, public FOutputDevice
{
	DECLARE_CHTYPE(CHTYPE_Control,FControlChannel);

	// Constructor.
	FControlChannel( UNetConnection* InConnection, INT InChannelIndex, INT InOpenedLocally );
	~FControlChannel();

	// UChannel interface.
	void ReceivedBunch( FInBunch& Bunch );

	// FArchive interface.
	void Serialize( void* Data, INT Count, EName Event );

	// FControlChannel interface.
	FString Describe();
};

/*-----------------------------------------------------------------------------
	FActorChannel.
-----------------------------------------------------------------------------*/

//
// Information for tracking retirement and retransmission of a property.
//
struct FPropertyRetirement
{
	SQWORD InPacketFullId;	// Packet received on, INDEX_NONE=none.
	INT    OutPacketId;		// Packet sent on, INDEX_NONE=none.
	BYTE   Dirty;			// Whether it needs to be resent.
	BYTE   Reliable;		// Whether it was sent reliably.
	FPropertyRetirement()
	:	OutPacketId		( INDEX_NONE )
	,	InPacketFullId	( INDEX_NONE )
	,	Dirty			( 0 )
	{}
};

//
// A channel for exchanging actor properties.
//
class ENGINE_API FActorChannel : public FChannel
{
	DECLARE_CHTYPE(CHTYPE_Actor,FActorChannel);

	// Variables.
	ULevel*	Level;			// Level this actor channel is associated with.
	AActor* Actor;			// Actor this corresponds to.
	UClass* ActorClass;		// Class of the actor.
	DOUBLE	RelevantTime;	// Last time this actor was relevant to client.
	DOUBLE	LastUpdateTime;	// Last time this actor was replicated.
	UBOOL   SpawnAcked;	    // Whether spawn has been acknowledged.
	TArray<BYTE> Recent;	// Most recently sent values.
	TArray<FPropertyRetirement> Retirement; // Property retransmission.

	// Constructor.
	FActorChannel( UNetConnection* InConnection, INT InChannelIndex, INT InOpenedLocally );
	~FActorChannel();

	// UChannel interface.
	void SetClosingFlag();
	void ReceivedBunch( FInBunch& Bunch );
	void Close();
	void Tick();

	// FActorChannel interface and accessors.
	AActor* GetActor() {return Actor;}
	FString Describe();
	void ReplicateActor( UBOOL FullReplication );
	void SetChannelActor( AActor* InActor );
};

/*-----------------------------------------------------------------------------
	File transfer channel.
-----------------------------------------------------------------------------*/

//
// A channel for exchanging binary files.
//
class ENGINE_API FFileChannel : public FChannel
{
	DECLARE_CHTYPE(CHTYPE_File,FFileChannel);

	// Variables.
	TCHAR		Filename[256];	 // Filename being transfered.
	TCHAR		PrettyName[256]; // Pretty name of file.
	TCHAR		Error[256];		 // Error.
	FArchive*	FileAr;			 // File being transfered.
	INT			Transfered;		 // Bytes transfered.
	INT			PackageIndex;	 // Index of package in Map.

	// Constructor.
	FFileChannel( UNetConnection* InConnection, INT InChannelIndex, INT InOpenedLocally );
	~FFileChannel();

	// UChannel interface.
	void ReceivedBunch( FInBunch& Bunch );

	// FFileChannel interface.
	FString Describe();
	void Tick();
};

/*-----------------------------------------------------------------------------
	The End.
-----------------------------------------------------------------------------*/
