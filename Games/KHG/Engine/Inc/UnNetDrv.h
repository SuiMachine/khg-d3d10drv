/*=============================================================================
	UnNetDrv.h: Unreal network driver base class.
	Copyright 1997-1999 Epic Games, Inc. All Rights Reserved.

	Revision history:
		* Created by Tim Sweeney
=============================================================================*/

/*-----------------------------------------------------------------------------
	UNetDriver.
-----------------------------------------------------------------------------*/

//
// Base class of a network driver attached to an active or pending level.
//
class ENGINE_API UNetDriver : public USubsystem
{
	DECLARE_ABSTRACT_CLASS(UNetDriver,USubsystem,CLASS_Transient|CLASS_Config)

	// Variables.
	TArray<UNetConnection*>	ClientConnections;
	UNetConnection*			ServerConnection;
	FNetworkNotify*			Notify;
	FPackageMap				MasterMap;
	FLOAT					ConnectionTimeout;
	FLOAT					InitialConnectTimeout;
	FLOAT					KeepAliveTime;
	FLOAT					DumbProxyTimeout;
	FLOAT					SimulatedProxyTimeout;
	FLOAT					SpawnPrioritySeconds;
	FLOAT					ServerTravelPause;
	INT						MaxClientRate;
	INT						MaxTicksPerSecond;

	// Constructors.
	UNetDriver();
	static void StaticConstructor( UClass* Class );

	// UObject interface.
	void Destroy();
	void Serialize( FArchive& Ar );

	// UNetDriver interface.
	virtual UBOOL Init( INT, FNetworkNotify* InNotify, FURL& InURL, TCHAR* Error );
	virtual void TickFlush();
	virtual void TickDispatch();
	virtual UBOOL IsInternet();
	virtual UBOOL Exec( const TCHAR* Cmd, FOutputDevice& Ar=GOut );
};

/*-----------------------------------------------------------------------------
	The End.
-----------------------------------------------------------------------------*/
