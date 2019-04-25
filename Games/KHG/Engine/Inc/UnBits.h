/*=============================================================================
	UnBits.h: Unreal bitstream manipulation classes.
	Copyright 1997-1999 Epic Games, Inc. All Rights Reserved.

	Revision history:
		* Created by Tim Sweeney
=============================================================================*/

/*-----------------------------------------------------------------------------
	FBitWriter.
-----------------------------------------------------------------------------*/

//
// Writes bitstreams.
//
struct ENGINE_API FBitWriter : public FArchive
{
	friend struct FBitWriterMark;
public:
	FBitWriter( INT InMaxBits )
	:	Num			( 0 )
	,	Max			( InMaxBits )
	,	Buffer		( (InMaxBits+7)/8 )
	,	Overflowed	( 0 )
	{
		guard(FBitWriter::FBitWriter);
		ArIsNet = ArIsPersistent = ArIsSaving = 1;
		unguard;
	}
	void SerializeBits( void* Src, INT LengthBits )
	{
		guard(FBitWriter::SerializeBits);
		if( Num+LengthBits<=Max )
		{
			for( INT i=0; i<LengthBits; i++,Num++ )
				if( ((BYTE*)Src)[i/8]&(1<<(i&7)) )
					Buffer(Num/8) |= (1<<(Num&7));
				else
					Buffer(Num/8) &= ~(1<<(Num&7));
		}
		else Overflowed = 1;
		unguard;
	}
	void WriteInt( DWORD Result, DWORD Max )
	{
		Result = INTEL_ORDER(Result);
		check(0);
		//SerializeBits( &Result, appCeilLogTwo(Max) );
		SerializeBits( &Result, 32 );
	}
	void WriteBit( BYTE In )
	{
		guard(FBitWriter::WriteBit);
		SerializeBits( &In, 1 );
		unguard;
	}
	void Serialize( void* Src, INT LengthBytes )
	{
		guard(FBitWriter::Serialize);
		SerializeBits( Src, LengthBytes*8 );
		unguard;
	}
	INT GetNumBytes()
	{
		return (Num+7)/8;
	}
	INT GetNumBits()
	{
		return Num;
	}
	UBOOL GetOverflowed()
	{
		return Overflowed;
	}
	void SetOverflowed()
	{
		Overflowed = 1;
	}
	BYTE* GetData()
	{
		return &Buffer(0);
	}
private:
	TArray<BYTE> Buffer;
	INT   Num;
	INT   Max;
	UBOOL Overflowed;
};

//
// For pushing and popping FBitWriter positions.
//
struct FBitWriterMark
{
public:
	FBitWriterMark()
	:	Overflowed  ( 0 )
	,	Num         ( 0 )
	{}
	FBitWriterMark( FBitWriter& Writer )
	:	Overflowed	( Writer.Overflowed )
	,	Num			( Writer.Num )
	{}
	INT GetNumBits()
	{
		return Num;
	}
	void Pop( FBitWriter& Writer )
	{
		Writer.Overflowed = Overflowed;
		Writer.Num        = Num;
	}
private:
	UBOOL			Overflowed;
	INT				Num;
};

/*-----------------------------------------------------------------------------
	FBitReader.
-----------------------------------------------------------------------------*/

//
// Reads bitstreams.
//
struct ENGINE_API FBitReader : public FArchive
{
public:
	FBitReader( BYTE* Src=NULL, INT CountBits=0 )
	:	Num			( CountBits )
	,	Buffer		( (CountBits+7)/8 )
	,	Pos			( 0 )
	,	Overflowed	( 0 )
	{
		guard(FBitReader::FBitReader);
		ArIsNet = ArIsPersistent = ArIsLoading = 1;
		if( Src )
			appMemcpy( &Buffer(0), Src, (CountBits+7)/8 );
		unguard;
	}
	void SetData( FBitReader& Src, INT CountBits )
	{
		guard(FBitReader::SetData);
		Num        = CountBits;
		Pos        = 0;
		Overflowed = 0;
		Buffer.Empty();
		Buffer.Add( (CountBits+7)/8 );
		Src.SerializeBits( &Buffer(0), CountBits );
		unguard;
	}
	void SerializeBits( void* Dest, INT LengthBits )
	{
		guard(FBitReader::SerializeBits);
		if( Pos+LengthBits<=Num )
		{
			for( INT i=0; i<LengthBits; i++,Pos++ )
				if( Buffer(Pos/8)&(1<<(Pos&7)) )
					((BYTE*)Dest)[i/8] |= (1<<(i&7));
				else
					((BYTE*)Dest)[i/8] &= ~(1<<(i&7));
		}
		else SetOverflowed();
		unguard;
	}
	DWORD ReadInt( DWORD Max )
	{
		INT Result=0;
		check(0);
		//SerializeBits( &Result, appCeilLogTwo(Max) );
		SerializeBits( &Result, 32 );
		return INTEL_ORDER(Result);
	}
	BYTE ReadBit()
	{
		BYTE Bit=0;
		SerializeBits( &Bit, 1 );
		return Bit;
	}
	void Serialize( void* Dest, INT LengthBytes )
	{
		guard(FBitReader::Serialize);
		SerializeBits( Dest, LengthBytes*8 );
		unguard;
	}
	UBOOL AtEnd()
	{
		return Overflowed || Pos==Num;
	}
	UBOOL GetOverflowed()
	{
		return Overflowed;
	}
	void SetOverflowed()
	{
		Overflowed = 1;
	}
	BYTE* GetData()
	{
		return &Buffer(0);
	}
	INT GetNumBytes()
	{
		return (Num+7)/8;
	}
	INT GetNumBits()
	{
		return Num;
	}
	INT GetPosBits()
	{
		return Pos;
	}
private:
	TArray<BYTE> Buffer;
	INT   Num;
	INT   Pos;
	UBOOL Overflowed;
};

/*-----------------------------------------------------------------------------
	The End.
-----------------------------------------------------------------------------*/
