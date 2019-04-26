/*=============================================================================
	UnTex.h: Unreal texture related classes.
	Copyright 1997-1999 Epic Games, Inc. All Rights Reserved.

	Revision history:
		* Created by Tim Sweeney
=============================================================================*/

/*-----------------------------------------------------------------------------
	Constants.
-----------------------------------------------------------------------------*/

enum {NUM_PAL_COLORS=256};	// Number of colors in a standard palette.

// Constants.
enum
{
	EHiColor565_R = 0xf800,
	EHiColor565_G = 0x07e0,
	EHiColor565_B = 0x001f,

	EHiColor555_R = 0x7c00,
	EHiColor555_G = 0x03e0,
	EHiColor555_B = 0x001f,

	ETrueColor_R  = 0x00ff0000,
	ETrueColor_G  = 0x0000ff00,
	ETrueColor_B  = 0x000000ff,
};

/*-----------------------------------------------------------------------------
	UPalette.
-----------------------------------------------------------------------------*/

//
// A truecolor value.
//
#define GET_COLOR_DWORD(color) (*(DWORD*)&(color))
class ENGINE_API FColor
{
public:
	// Variables.
	union
	{
		struct
		{
#if __INTEL_BYTE_ORDER__
			BYTE R,G,B,A;
#else
			BYTE A,B,G,R;
#endif
		};
		DWORD D;
	};

	// Constructors.
	FColor() {}
	FColor( BYTE InR, BYTE InG, BYTE InB )
	:	R(InR), G(InG), B(InB) {}
	FColor( BYTE InR, BYTE InG, BYTE InB, BYTE InA )
	:	R(InR), G(InG), B(InB), A(InA) {}
	FColor( const FPlane& P )
	:	R(Clamp(appFloor(P.X*256),0,255))
	,	G(Clamp(appFloor(P.Y*256),0,255))
	,	B(Clamp(appFloor(P.Z*256),0,255))
	,	A(Clamp(appFloor(P.W*256),0,255))
	{}

	// Operators.
	// NOTE: These lack alpha channel!
	UBOOL operator==( const FColor &C ) const
	{
		return R==C.R && G==C.G && B==C.B;
	}
	UBOOL operator!=( const FColor &C ) const
	{
		return R!=C.R || G!=C.G || B!=C.B;
	}
	INT Brightness() const
	{
		return (2*(INT)R + 3*(INT)G + 1*(INT)B)>>3;
	}
	FLOAT FBrightness() const
	{
		return (2.0*R + 3.0*G + 1.0*B)/(6.0*256.0);
	}
	DWORD TrueColor() const
	{
		return ((D&0xff)<<16) + (D&0xff00) + ((D&0xff0000)>>16);
	}
	_WORD HiColor565() const
	{
		return ((D&0xf8) << 8) + ((D&0xfC00) >> 5) + ((D&0xf80000) >> 19);
	}
	_WORD HiColor555() const
	{
		return ((D&0xf8) << 7) + ((D&0xf800) >> 6) + ((D&0xf80000) >> 19);
	}
	FVector Plane() const
	{
		return FPlane(R/255.f,G/255.f,B/255.f,A/255.0);
	}
	FColor Brighten( INT Amount )
	{
		return FColor( Plane() * (1.0 - Amount/24.0) );
	}
};
extern ENGINE_API FPlane FGetHSV( BYTE H, BYTE S, BYTE V );

//
// A palette object.  Holds NUM_PAL_COLORS unique FColor values, 
// forming a 256-color palette which can be referenced by textures.
//
class ENGINE_API UPalette : public UObject
{
	DECLARE_CLASS(UPalette,UObject,CLASS_SafeReplace)

	// Variables.
	TArray<FColor> Colors;

	// Constructors.
	UPalette();

	// UObject interface.
	void Serialize( FArchive& Ar );

	// UPalette interface.
	BYTE BestMatch( FColor Color, enum EBestMatchRange Range );
	UPalette* ReplaceWithExisting();
	void FixPalette();
};

/*-----------------------------------------------------------------------------
	UTexture and FTextureInfo.
-----------------------------------------------------------------------------*/

// Texture level-of-detail sets.
enum ELODSet
{
	LODSET_None  = 0,  // No level of detail mipmap tossing.
	LODSET_World = 1,  // World level-of-detail set.
	LODSET_Skin  = 2,  // Skin level-of-detail set.
	LODSET_MAX   = 8,  // Maximum.
};

enum {MAX_TEXTURE_LOD=4};

//
// Base mipmap.
//
struct ENGINE_API FMipmapBase
{
public:
	BYTE*			DataPtr;		// Pointer to data, valid only when locked.
	INT				USize,  VSize;	// Power of two tile dimensions.
	BYTE			UBits,  VBits;	// Power of two tile bits.
	FMipmapBase( BYTE InUBits, BYTE InVBits )
	:	DataPtr		(0)
	,	USize		(1<<InUBits)
	,	VSize		(1<<InVBits)
	,	UBits		(InUBits)
	,	VBits		(InVBits)
	{}
	FMipmapBase()
	{}
};

//
// Texture mipmap.
//
struct ENGINE_API FMipmap : public FMipmapBase
{
public:
	TLazyArray<BYTE> DataArray; // Data.
	FMipmap()
	{}
	FMipmap( BYTE InUBits, BYTE InVBits )
	:	FMipmapBase( InUBits, InVBits )
	,	DataArray( USize * VSize )
	{}
};

//
// Texture clearing flags.
//
enum ETextureClear
{
	TCLEAR_Temporal	= 1,	// Clear temporal texture effects.
	TCLEAR_Bitmap   = 2,    // Clear the immediate bitmap.
};

//
// Texture formats.
//
enum ETextureFormat
{
	// Base types.
	TEXF_P8			= 0x00,
	//TEXF_RGB32		= 0x01,
	TEXF_RGBA7		= 0x01,
	TEXF_RGB16		= 0x02,
	TEXF_DXT1       = 0x03,
	TEXF_RGB8       = 0x04,
	TEXF_RGBA8      = 0x05,
	TEXF_MAX		= 0xff,
};

//
// A low-level bitmap.
//
class ENGINE_API UBitmap : public UObject
{
	DECLARE_ABSTRACT_CLASS(UBitmap,UObject,0)

	// General bitmap information.
	BYTE		Format;				// ETextureFormat.
	UPalette*	Palette;			// Palette if 8-bit palettized.
	BYTE		UBits, VBits;		// # of bits in USize, i.e. 8 for 256.
	INT			USize, VSize;		// Size, must be power of 2.
	INT			UClamp, VClamp;		// Clamped width, must be <= size.
	FColor		MipZero;			// Overall average color of texture.
	FColor		MaxColor;			// Maximum color for normalization.
	DOUBLE		LastUpdateTime;		// Last time texture was locked for rendering.

	// Static.
	static class UClient* __Client;

	// Constructor.
	UBitmap();

	// UBitmap interface.
	virtual void GetInfo( FTextureInfo& TextureInfo, DOUBLE Time )=0;
	virtual INT GetNumMips()=0;
	virtual FMipmapBase* GetMip( INT i )=0;
};

//
// A complex material texture.
//
class ENGINE_API UTexture : public UBitmap
{
	DECLARE_CLASS(UTexture,UBitmap,CLASS_SafeReplace)

	// Subtextures.
	UTexture*	BumpMap;       // Bump map to illuminate this texture with.
	UTexture*	DetailTexture; // Detail texture to apply.
	UTexture*	MacroTexture;	 // Macrotexture to apply, not currently used.

	// Surface properties.
	FLOAT		Diffuse;			// Diffuse lighting coefficient (0.0-1.0).
	FLOAT		Specular;			// Specular lighting coefficient (0.0-1.0).
	FLOAT		Alpha;				// Reflectivity (0.0-0.1).
	FLOAT   Scale;        // Scaling relative to parent, 1.0=normal.
	FLOAT		Friction;			// Surface friction coefficient, 1.0=none, 0.95=some.
	FLOAT		MipMult;			// Mipmap multiplier.

	// Sounds.
	USound*		FootstepSound;// Footstep sound.
	USound*		HitSound;			// Sound when the texture is hit with a projectile.

	// Flags.
#if 1
	DWORD		PolyFlags;			// Polygon flags to be applied to Bsp polys with texture (See PF_*).
#else
  BITFIELD bInvisible:1;
  BITFIELD bMasked:1;
  BITFIELD bTransparent:1;
  BITFIELD bNotSolid:1;
  BITFIELD bEnvironment:1;
  BITFIELD bSemisolid:1;
  BITFIELD bModulate:1;
  BITFIELD bFakeBackdrop:1;

  BITFIELD bTwoSided:1;
  BITFIELD bAutoUPan:1;
  BITFIELD bAutoVPan:1;
  BITFIELD bNoSmooth:1;
  BITFIELD bBigWavy:1;
  BITFIELD bSmallWavy:1;
  BITFIELD bWaterWavy:1;
  BITFIELD bLowShadowDetail:1;

  BITFIELD bNoMerge:1;
  BITFIELD bCloudWavy:1;
  BITFIELD bDirtyShadows:1;
  BITFIELD bHighLedge:1;
  BITFIELD bSpecialLit:1;
  BITFIELD bGouraud:1;
  BITFIELD bUnlit:1;
  BITFIELD bHighShadowDetail:1;

  BITFIELD bPortal:1;
  BITFIELD bMirrored:1;
  BITFIELD bX2:1;
  BITFIELD bX3:1;
  BITFIELD bX4:1;
  BITFIELD bX5:1;
  BITFIELD bX6:1;
  BITFIELD bX7:1;
#endif
  BITFIELD bNoTile:1;
  BITFIELD bBumpMap:1;
  BITFIELD bBlur:1;
  BITFIELD bRealtime:1;    // Texture changes in realtime.
  BITFIELD bParametric:1;  // Texture data need not be stored.

	// Animation related.
	UTexture*	AnimNext;			// Next texture in looped animation sequence.
	UTexture*	AnimCur;			// Current animation frame.
	BYTE		PrimeCount;			// Priming total for algorithmic textures.
	BYTE		PrimeCurrent;		// Priming current for algorithmic textures.
	FLOAT		MinFrameRate;		// Minimum animation rate in fps.
	FLOAT		MaxFrameRate;		// Maximum animation rate in fps.
	FLOAT		Accumulator;		// Frame accumulator.

	// Table of mipmaps.
	TArray<FMipmap> Mips;			// Mipmaps in native format.

	// Constructor.
	UTexture();

	// UObject interface.
	void Serialize( FArchive& Ar );

	// UBitmap interface.
	DWORD GetColorsIndex()
	{
		return Palette->GetIndex();
	}
	FColor* GetColors()
	{
		return Palette ? &Palette->Colors(0) : NULL;
	}
	INT GetNumMips()
	{
		return Mips.Num();
	}
	FMipmapBase* GetMip( INT i )
	{
		return &Mips(i);
	}
	void GetInfo( FTextureInfo& TextureInfo, DOUBLE Time );

	// UTexture interface.
	virtual void Clear( DWORD ClearFlags );
	virtual void Init( INT InUSize, INT InVSize );
	virtual void Tick( FLOAT DeltaSeconds );
	virtual void ConstantTimeTick();
	virtual void MousePosition( DWORD Buttons, FLOAT X, FLOAT Y );
	virtual void Click( DWORD Buttons, FLOAT X, FLOAT Y );
	virtual void Update( DOUBLE Time );

	// UTexture functions.
	void CreateMips( UBOOL FullMips, UBOOL Downsample );
	void CreateColorRange();

	// UTexture accessors.
	UTexture* Get( DOUBLE Time )
	{
		Update( Time );
		return AnimCur ? AnimCur : this;
	}
};

enum ETextureFlags
{
	// General info about the texture.
	TF_Realtime         = 0x00000008,   // Texture data (not animation) changes in realtime.
	TF_Parametric       = 0x00000010,   // Texture is parametric so data need not be saved.
	TF_RealtimeChanged  = 0x00000020,   // Realtime texture has changed since last lock.
	TF_RealtimePalette  = 0x00000040,	// Realtime palette.
};

//
// Information about a locked texture. Used for ease of rendering.
//
enum {MAX_MIPS=12};
struct FTextureInfo
{
	QWORD				CacheID;		// Unique cache ID.
	QWORD				PaletteCacheID;	// Unique cache ID of palette.
	FVector				Pan;			// Panning value relative to texture planes.
	FColor*				MaxColor;		// Maximum color in texture and all its mipmaps.
	ETextureFormat		Format;			// Texture format.
	FLOAT				UScale;			// U Scaling.
	FLOAT				VScale;			// V Scaling.
	INT					USize;			// Base U size.
	INT					VSize;			// Base V size.
	INT					UClamp;			// U clamping value, or 0 if none.
	INT					VClamp;			// V clamping value, or 0 if none.
	INT					NumMips;		// Number of mipmaps.
	FColor*				Palette;		// Palette colors.
	DWORD				TextureFlags;	// From ETextureFlags.
	FMipmap*			Mips[MAX_MIPS];	// Array of NumMips of mipmaps.
};

/*-----------------------------------------------------------------------------
	UFont.
-----------------------------------------------------------------------------*/

// Font constants.
enum {NUM_FONT_PAGES=256};
enum {NUM_FONT_CHARS=256};

//
// Information about one font glyph which resides in a texture.
//
struct ENGINE_API FFontCharacter
{
	// Variables.
	INT StartU, StartV;
	INT USize, VSize;
};

//
// A font page.
//
/*
struct ENGINE_API FFontPage
{
	// Variables.
	UTexture* Texture;
	TArray<FFontCharacter> Characters;

	// Serializer.
	friend FArchive& operator<<( FArchive& Ar, FFontPage& Ch );
};
*/

//
// A font object, containing information about a set of glyphs.
// The glyph bitmaps are stored in the contained textures, while
// the font database only contains the coordinates of the individual
// glyph.
//
class ENGINE_API UFont : public UTexture
{
	DECLARE_CLASS(UFont,UObject,0)

	// Variables.
	INT CharactersPerPage;
	FFontCharacter Pages[NUM_FONT_PAGES][NUM_FONT_CHARS];

	// Constructors.
	UFont();

	// UObject interface.
	void Serialize( FArchive& Ar );
};

/*----------------------------------------------------------------------------
	The End.
----------------------------------------------------------------------------*/
