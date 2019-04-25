/*=============================================================================
	UnNet.h: Unreal networking.
	Copyright 1997-1999 Epic Games, Inc. All Rights Reserved.

	Revision history:
		* Created by Tim Sweeney
=============================================================================*/

/*-----------------------------------------------------------------------------
	Forward declarations.
-----------------------------------------------------------------------------*/

class	FChannel;
class		FControlChannel;
class		FActorChannel;
class		FFileChannel;
class	FInBunch;
class	FOutBunch;
class	FChannelIterator;

class	UNetDriver;
class	UNetConnection;
class	UPendingLevel;
class		UNetPendingLevel;
class		UDemoPlayPendingLevel;

/*-----------------------------------------------------------------------------
	Types.
-----------------------------------------------------------------------------*/

// Up to this many reliable channel bunches may be buffered.
enum {RELIABLE_BUFFER=128};

// Types of channels.
enum EChannelType
{
	CHTYPE_None			= 0,  // Invalid type.
	CHTYPE_Control		= 1,  // Connection control.
	CHTYPE_Actor  		= 2,  // Actor-update channel.
	CHTYPE_File         = 3,  // Binary file transfer.
	CHTYPE_MAX          = 8,  // Maximum.
};

/*-----------------------------------------------------------------------------
	Includes.
-----------------------------------------------------------------------------*/

#include "UnNetDrv.h"		// Network driver class.
#include "UnBits.h"			// Bitstream archiver.
#include "UnBunch.h"		// Bunch class.
#include "UnConn.h"			// Connection class.
#include "UnChan.h"			// Channel class.
#include "UnPenLev.h"		// Pending levels.

/*-----------------------------------------------------------------------------
	The End.
-----------------------------------------------------------------------------*/
