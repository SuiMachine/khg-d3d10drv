/*=============================================================================
	UnCoreNet.h: Core networking support.
	Copyright 1997-1999 Epic Games, Inc. All Rights Reserved.

	Revision history:
		* Created by Tim Sweeney
=============================================================================*/

//
// Information about a field.
//
class CORE_API FFieldNetCache
{
public:
	UField* Field;
	INT RepIndex;
	INT ValueIndex;
	INT ConditionIndex;
	FFieldNetCache()
	{}
	FFieldNetCache( UField* InField, INT InRepIndex, INT InValueIndex, INT InConditionIndex )
	: Field(InField), RepIndex(InRepIndex), ValueIndex(InValueIndex), ConditionIndex(InConditionIndex)
	{}
	friend CORE_API FArchive& operator<<( FArchive& Ar, FFieldNetCache& F );
};

//
// Information about a class, cached for network coordination.
//
class CORE_API FClassNetCache
{
	friend class FPackageMap;
public:
	FClassNetCache();
	FClassNetCache( UClass* Class );
	INT GetMaxIndex();
	INT GetRepValueCount();
	FFieldNetCache* GetFromField( UField* Field );
	FFieldNetCache* GetFromIndex( INT Index );
	CORE_API friend FArchive& operator<<( FArchive& Ar, FClassNetCache& Cache );
private:
	INT RepValueCount;
	UClass* Class;
	TArray<FFieldNetCache> Fields;
	TMap<UField*,INT> FieldIndices;
};

//
// Ordered information of linker file requirements.
//
class CORE_API FPackageInfo
{
public:
	// Variables.
	FString			URL;				// URL of the package file we need to request.
	ULinkerLoad*	Linker;				// Pointer to the linker, if loaded.
	UObject*		Parent;				// The parent package.
	FGuid			Guid;				// Package identifier.
	INT				FileSize;			// File size.
	INT				ObjectBase;			// Net index of first object.
	INT				ObjectCount;		// Number of objects, defined by server.
	INT				NameBase;			// Net index of first name.
	INT				NameCount;			// Number of names, defined by server.
	INT				LocalGeneration;	// This machine's generation of the package.
	INT				RemoteGeneration;	// Remote machine's generation of the package.
	DWORD			PackageFlags;		// Package flags.

	// Functions.
	FPackageInfo( ULinkerLoad* InLinker=NULL );
	CORE_API friend FArchive& operator<<( FArchive& Ar, FPackageInfo& I );
};

//
// Maps objects and names to and from indices for network communication.
//
class CORE_API FPackageMap
{
public:
	// Functions.
	INT NameToIndex( FName Name );
	INT ObjectToIndex( UObject* Object );
	FName IndexToName( INT Index );
	UObject* IndexToObject( INT Index, UBOOL Load );
	INT AddLinker( ULinkerLoad* Linker );
	void Compute();
	INT GetMaxObjectIndex() {return MaxObjectIndex;}
	INT GetMaxNameIndex() {return MaxNameIndex;}
	FClassNetCache* GetClassNetCache( UClass* Class );
	CORE_API friend FArchive& operator<<( FArchive& Ar, FPackageMap& P );
	TArray<FPackageInfo> List;
private:
	static INT FPackageMap::NetFieldCompare( UField** PA, UField** PB );
	TMap<UClass*,FClassNetCache> ClassFieldIndices;
	TArray<INT> NameIndices;
	INT MaxObjectIndex;
	INT MaxNameIndex;
};

/*----------------------------------------------------------------------------
	The End.
----------------------------------------------------------------------------*/