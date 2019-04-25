/*=============================================================================
	UnConn.h: Unreal network connection base class.
	Copyright 1997-1999 Epic Games, Inc. All Rights Reserved.

	Revision history:
		* Created by Tim Sweeney
=============================================================================*/

class UNetDriver;

/*-----------------------------------------------------------------------------
	UNetConnection.
-----------------------------------------------------------------------------*/

//
// Whether to support net lag and packet loss testing.
//
#define DO_ENABLE_NET_TEST 1

//
// State of a connection.
//
enum EConnectionState
{
	USOCK_Invalid   = 0, // Connection is invalid, possibly uninitialized.
	USOCK_Closed    = 1, // Connection permanently closed.
	USOCK_Pending	= 2, // Connection is awaiting connection.
	USOCK_Open      = 3, // Connection is open.
};

//
// A network connection.
//
class ENGINE_API UNetConnection : public UPlayer
{
	DECLARE_ABSTRACT_CLASS(UNetConnection,UPlayer,CLASS_Transient|CLASS_Config)

	// Constants.
	enum{ MAX_PROTOCOL_VERSION = 1     };	// Maximum protocol version supported.
	enum{ MIN_PROTOCOL_VERSION = 1     };	// Minimum protocol version supported.
	enum{ MAX_CHANNELS         = 511  };	// Maximum channels.

	// Connection information.
	UNetDriver*		Driver;					// Owning driver.
	EConnectionState State;					// State this connection is in.
	FURL			URL;					// URL of the other side.

	// Negotiated parameters.
	INT				ProtocolVersion;		// Protocol version we're communicating with (<=PROTOCOL_VERSION).
	INT				MaxPacket;				// Maximum packet size.
	INT       Rate;

	FPackageMap		PackageMap;				// Package map between local and remote.

	INT				IdealPacket;			// Ideal packet size.
	INT				PacketOverhead;			// Bytes overhead per packet sent.
	UBOOL			InternalAck;			// Internally ack all packets, for 100% reliable connections.
	INT				ConnectionNetSpeed;		// Maximum bytes per second.
	INT				Challenge;				// Server-generated challenge.
	INT				NegotiatedVer;			// Negotiated version for new channels.
	FStringNoInit	RequestURL;				// URL requested by client

	// Internal.
	DOUBLE			LastReceiveTime;		// Last time a packet was received, for timeout checking.
	DOUBLE			LastSendTime;			// Last time a packet was sent, for keepalives.
	DOUBLE			LastTickTime;			// Last time of polling.
	INT				QueuedBytes;			// Bytes assumed to be queued up.

	// Merge info.
	FBitWriterMark  LastStart;				// Most recently sent bunch start.
	FBitWriterMark  LastEnd;				// Most recently sent bunch end.
	UBOOL			AllowMerge;				// Whether to allow merging.
	UBOOL			TimeSensitive;			// Whether contents are time-sensitive.
	FOutBunch*		LastOutBunch;			// Most recent outgoing bunch.
	FOutBunch		LastOut;

	// Stat display.
	DOUBLE			StatUpdateTime;			// Time of last stat update.
	FLOAT			StatPeriod;				// Interval between gathering stats.
	FLOAT			InRate,    OutRate;		// Rate for last interval.
	FLOAT			InPackets, OutPackets;	// Packet counts.
	FLOAT			InBunches, OutBunches;	// Bunch counts.
	FLOAT			InLoss,    OutLoss;		// Packet loss percent.
	FLOAT			InOrder,   OutOrder;	// Out of order incoming packets.
	FLOAT			BestLag;				// Lag.

	// Stat accumulators.
	FLOAT			LagAcc;					// Previous msec lag.
	INT				InLossAcc, OutLossAcc;	// Packet loss accumulator.
	INT				InPktAcc,  OutPktAcc;	// Packet accumulator.
	INT				InBunAcc,  OutBunAcc;	// Bunch accumulator.
	INT				InByteAcc, OutByteAcc;	// Byte accumulator.
	INT				InOrdAcc,  OutOrdAcc;	// Out of order accumulator.
	INT				HighLossCount;			// Counts high packet loss.

	// Packet.
	FBitWriter		Out;					// Outgoing packet.
	DOUBLE			OutLagTime    [256];	// For lag measuring.
	SQWORD			InPacketFullId;			// Full incoming packet index.
	_WORD			OutLagPacketId[256];	// For lag measuring.
	_WORD			OutPacketId;			// Most recently sent packet.
	_WORD			OutAckPacketId;			// Most recently acked outgoing packet.
	_WORD			InPacketId;				// Most recently received packet.

	// Acknowledgement tables.
	BYTE  OutAcked[ 65536/8 ], OutNaked[ 65536/8 ];
	UBOOL GetOutBit  ( BYTE* Tbl, INT i ) {return (Tbl[i>>3] &  (1<<(i&7)))!=0;}
	void  SetOutBit  ( BYTE* Tbl, INT i ) {Tbl[i>>3]        |=  (1<<(i&7));    }
	void  ClearOutBit( BYTE* Tbl, INT i ) {Tbl[i>>3]        &= ~(1<<(i&7));    }

	// Channel table.
	FChannel*  Channels     [ MAX_CHANNELS ];
	_WORD      OutReliable  [ MAX_CHANNELS ];
	_WORD      InReliable   [ MAX_CHANNELS ];
	TMap<AActor*,INT> ActorChannels;
	TArray<AActor*> SentTemporaries;

#if DO_ENABLE_NET_TEST
	// For development.
	INT				PktLoss;
	INT				PktOrder;
	INT				PktDup;
	TArray<TArray<BYTE>	> Delayed;
#endif

	// Constructors and destructors.
	UNetConnection();
	UNetConnection( UNetDriver* Driver );

	// UObject interface.
	void Serialize( FArchive& Ar );
	void Destroy();

	// UPlayer interface.
	void ReadInput( FLOAT DeltaSeconds );

	// FArchive interface.
	void Serialize( void* Data, INT Count, EName MsgType );

	// FExec interface.
	UBOOL Exec( const TCHAR* Cmd, FOutputDevice& Ar=GOut );

	// UNetConnection interface.
	virtual TCHAR* Describe( TCHAR* )=0;
	virtual void FlushNet();
	virtual void Tick();
	virtual void AssertValid();
	virtual UBOOL IsNetReady();

	// Functions.
	void SendAck( _WORD PacketId );
	void PreSend( INT SizeBits );
	void PostSend();
	_WORD SendRawBunch( FOutBunch& Bunch, UBOOL InAllowMerge );
	UNetDriver* GetDriver() {return Driver;}
	FChannel* CreateChannel( enum EChannelType Type, UBOOL bOpenedLocally, _WORD ChannelIndex=MAXWORD );
	void ReceivedPacket( FBitReader& Reader );
	class FActorChannel* GetActorChannel( AActor* Actor );
	void ReceiveFile( INT PackageIndex );
	void SlowAssertValid()
	{
#if DO_GUARD_SLOW
		AssertValid();
#endif
	}
};

/*-----------------------------------------------------------------------------
	Channel iterators.
-----------------------------------------------------------------------------*/

//
// Simple channel iterator.
//
class FChannelIterator
{
public:
	// Functions.
	FChannelIterator( UNetConnection* InConn )
	:	Conn( InConn), Index( -1 )
	{
		++*this;
	}
	void operator++()
	{
		checkSlow(Index<UNetConnection::MAX_CHANNELS);
		while( ++Index<UNetConnection::MAX_CHANNELS && !Conn->Channels[Index] );
	}
	operator UBOOL()
	{
		return Index<UNetConnection::MAX_CHANNELS;
	}
	INT GetIndex()
	{
		return Index;
	}
	FChannel* operator* ()
	{
		checkSlow(Index<UNetConnection::MAX_CHANNELS);
		return Conn->Channels[Index];
	}
	FChannel* operator-> ()
	{
		checkSlow(Index<UNetConnection::MAX_CHANNELS);
		return Conn->Channels[Index];
	}
private:
	// Variables.
	INT Index;
	UNetConnection* Conn;
};

//
// Typed channel iterator.
//
template<class T> class TTypedChannelIterator
{
public:
	// Functions.
	TTypedChannelIterator( UNetConnection* InConn )
	:	Conn( InConn), Index( -1 )
	{
		++*this;
	}
	void operator++()
	{
		checkSlow(Index<UNetConnection::MAX_CHANNELS);
		while
		(	++Index<UNetConnection::MAX_CHANNELS
		&&	(!Conn->Channels[Index] || Conn->Channels[Index]->ChType!=T::ChannelType) );
	}
	operator UBOOL()
	{
		return Index<UNetConnection::MAX_CHANNELS;
	}
	INT GetIndex()
	{
		return Index;
	}
	T* operator* ()
	{
		checkSlow(Index<UNetConnection::MAX_CHANNELS);
		checkSlow(Conn->Channels[Index])
		return (T*)Conn->Channels[Index];
	}
	T* operator-> ()
	{
		checkSlow(Index<UNetConnection::MAX_CHANNELS);
		checkSlow(Conn->Channels[Index])
		return (T*)Conn->Channels[Index];
	}
private:
	// Variables.
	INT Index;
	UNetConnection* Conn;
};

/*-----------------------------------------------------------------------------
	The End.
-----------------------------------------------------------------------------*/
