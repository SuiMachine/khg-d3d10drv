/*=============================================================================
	OpenGL.cpp: Unreal OpenGL support implementation for Windows and Linux.
	Copyright 1999-2001 Epic Games, Inc. All Rights Reserved.
	
	Other URenderDevice subclasses include:
	* USoftwareRenderDevice: Software renderer.
	* UGlideRenderDevice: 3dfx Glide renderer.
	* UDirect3DRenderDevice: Direct3D renderer.
	* UOpenGLRenderDevice: OpenGL renderer.

	Revision history:
	* Created by Daniel Vogel based on XMesaGLDrv
	* Changes (John Fulmer, Jeroen Janssen)
	* Major changes (Daniel Vogel)
	* Ported back to Win32 (Fredrik Gustafsson)
	* Unification and addition of vertex arrays (Daniel Vogel)
	* Actor triangle caching (Steve Sinclair)
    * One pass fogging (Daniel Vogel)
	* Windows gamma support (Daniel Vogel)
	* 2X blending support (Daniel Vogel)
	* Better detail texture handling (Daniel Vogel)
	* Scaleability (Daniel Vogel)
	* Texture LOD bias (Daniel Vogel)	
	* RefreshRate support on Windows (Jason Dick)
	* Finer control over gamma (Daniel Vogel)
	* Removed FANCY_FISHEYE and TNT code (Sebastian Kaufel)
	* Editor fixes (Sebastian Kaufel)

	UseTrilinear      whether to use trilinear filtering			
	UseAlphaPalette   set to 0 for buggy drivers (GeForce)
	UseS3TC           whether to use compressed textures
	Use4444Textures   speedup for real low end cards (G200)
	MaxAnisotropy     maximum level of anisotropy used
	UseFilterSGIS     whether to use the SGIS filters
	MaxTMUnits        maximum number of TMUs UT will try to use
	DisableSpecialDT  disable the special detail texture approach
	LODBias           texture lod bias
	RefreshRate       requested refresh rate (Windows only)
	GammaOffset       offset for the gamma correction
	UseNoFiltering       uses GL_NEAREST as min/mag texture filters

TODO:
	- DOCUMENTATION!!! (especially all subtle assumptions)
	- get rid of some unnecessary state changes (#ifdef out)

=============================================================================*/

#include "OpenGLDrv.h"
#include <math.h>

/*-----------------------------------------------------------------------------
	Try different BlitFlags.

	enum EViewportBlitFlags
	{
		// Bitflags.
		BLIT_Fullscreen     = 0x0001, // Fullscreen.
		BLIT_Temporary      = 0x0002, // Temporary viewport.
		BLIT_DibSection     = 0x0004, // Create a DibSection for windowed rendering.
		BLIT_DirectDraw     = 0x0008, // Create Direct3D along with DirectDraw.
		BLIT_Direct3D       = 0x0010, // Create Direct3D along with DirectDraw.
		BLIT_NoWindowChange = 0x0020, // Don't resize existing window frame.
		BLIT_NoWindowFrame  = 0x0040, // Turn off the window frame.
		BLIT_Race			= 0x0080, // Work around Windows mouse race condition.
		BLIT_HardwarePaint  = 0x0100, // Window should be repainted in hardware when drawn.

		// Special.
		BLIT_ParameterFlags	= BLIT_NoWindowChange, // Only parameters to ResizeViewport, not permanent flags.
	};
-----------------------------------------------------------------------------*/

#define BLIT_OpenGL		BLIT_Race

/*-----------------------------------------------------------------------------
	Globals.
-----------------------------------------------------------------------------*/

#define DRAW_ARRAYS   1
#define MAX_TMUNITS   3			// vogel: maximum number of texture mapping units supported

#define VERTEX_ARRAY_SIZE 2000		// vogel: better safe than sorry

#ifdef WIN32
#define DYNAMIC_BIND 1			// If 0, must static link to OpenGL32, Gdi32
#define GL_DLL "OpenGL32.dll"
#endif
	
/*-----------------------------------------------------------------------------
	OpenGLDrv.
-----------------------------------------------------------------------------*/

//
// An OpenGL rendering device attached to a viewport.
//
class UOpenGLRenderDevice : public URenderDevice
{
	DECLARE_CLASS(UOpenGLRenderDevice,URenderDevice,CLASS_Config)

	// Information about a cached texture.
	struct FCachedTexture
	{
		GLuint Id;
		INT BaseMip;
		INT UBits, VBits;
		INT UCopyBits, VCopyBits;
		FPlane ColorNorm, ColorRenorm;
	};


	// Geometry 	
	struct FGLVertex
	{
		FLOAT x;
		FLOAT y;
		FLOAT z;
	} VertexArray[VERTEX_ARRAY_SIZE];


	// Texcoords	
	struct FGLTexCoord
	{
		struct FGLTMU {
			FLOAT u;
			FLOAT v;
		} TMU [MAX_TMUNITS];
	} TexCoordArray[VERTEX_ARRAY_SIZE];


	// Primary and secondary (specular) color
	struct FGLColor
	{
		INT color;
		INT specular;
	} ColorArray[VERTEX_ARRAY_SIZE];

	struct FGLMapDot
	{
		FLOAT u;
		FLOAT v;
	} MapDotArray [VERTEX_ARRAY_SIZE];
	

	// MultiPass rendering information
	struct FGLRenderPass
	{
		struct FGLSinglePass
		{
			INT Multi;
			FTextureInfo* Info;
			DWORD PolyFlags;
			FLOAT PanBias;
		} TMU[MAX_TMUNITS];

		FSavedPoly* Poly;
	} MultiPass;				// vogel: MULTIPASS!!! ;)

	struct FGammaRamp
	{
		_WORD red[256];
		_WORD green[256];
		_WORD blue[256];
	};

#ifdef WIN32
	// Permanent variables.
	HGLRC hRC;
	HWND hWnd;
	HDC hDC;

	// Gamma Ramp to restore at exit	
	FGammaRamp OriginalRamp; 
#endif

	UBOOL WasFullscreen;
	TMap<QWORD,FCachedTexture> LocalBindMap, *BindMap;
	TArray<FPlane> Modes;
	UViewport* Viewport;

	// Timing.
	DWORD BindCycles, ImageCycles, ComplexCycles, GouraudCycles, TileCycles;

	// Hardware constraints.
	FLOAT LODBias;
	FLOAT GammaOffset;
	INT MaxLogUOverV;
	INT MaxLogVOverU;
	INT MinLogTextureSize;
	INT MaxLogTextureSize;
	INT MaxAnisotropy;
	INT TMUnits;
	INT MaxTMUnits;
	INT RefreshRate;
	INT ColorDepth;
	UBOOL UsePrecache;
	UBOOL UseMultiTexture;
	UBOOL UsePalette;
	UBOOL UseShareLists;
	UBOOL AlwaysMipmap;
	UBOOL UseTrilinear;
	UBOOL UseVertexSpecular;
	UBOOL UseAlphaPalette;
	UBOOL UseS3TC;
	UBOOL Use4444Textures;
	UBOOL UseCVA;
	UBOOL UseFilterSGIS;	
	UBOOL UseDetailAlpha;
	UBOOL UseNoFiltering;
	UBOOL SupportsLazyTextures;
	UBOOL PrefersDeferredLoad;
	UBOOL UseDetailTextures;
	UBOOL SupportsTC;
	UBOOL PrecacheOnFlip;
#if ! DRAW_ARRAYS
	INT VertexIndexList[VERTEX_ARRAY_SIZE];
#endif
	UBOOL BufferActorTris;
	INT BufferedVerts;

	UBOOL ColorArrayEnabled;
	UBOOL RenderFog;
	UBOOL GammaFirstTime;

	INT AllocatedTextures;
	INT PassCount;

	// Hit info.
	BYTE* HitData;
	INT* HitSize;

	// Lock variables.
	FPlane FlashScale, FlashFog;
	FLOAT RProjZ, Aspect;
	DWORD CurrentPolyFlags;
	DWORD CurrentEnvFlags[MAX_TMUNITS];	
	TArray<INT> GLHitData;
	struct FTexInfo
	{
		QWORD CurrentCacheID;
		FLOAT UMult;
		FLOAT VMult;
		FLOAT UPan;
		FLOAT VPan;
		FPlane ColorNorm;
		FPlane ColorRenorm;
	} TexInfo[MAX_TMUNITS];
	FLOAT RFX2, RFY2;
	GLuint AlphaTextureId;
	GLuint NoTextureId;

	// Static variables.
	static TMap<QWORD,FCachedTexture> SharedBindMap;
	static INT NumDevices;
	static INT LockCount;

#ifdef WIN32
	static TArray<HGLRC> AllContexts;
	static HGLRC   hCurrentRC;
	static HMODULE hModuleGlMain;
	static HMODULE hModuleGlGdi;
#else
	static UBOOL GLLoaded;
#endif

	// GL functions.
	#define GL_EXT(name) static UBOOL SUPPORTS##name;
	#define GL_PROC(ext,ret,func,parms) static ret (STDCALL *func)parms;
	#include "OpenGLFuncs.h"
	#undef GL_EXT
	#undef GL_PROC
	

#ifdef RGBA_MAKE
#undef RGBA_MAKE
#endif

	inline int RGBA_MAKE( BYTE r, BYTE g, BYTE b, BYTE a)		// vogel: I hate macros...
	{
		return (a << 24) | (b <<16) | ( g<< 8) | r;
	}

	inline void DrawArrays( GLenum type, int start, int num )	// vogel: ... and macros hate me
	{
#if DRAW_ARRAYS	
		glDrawArrays( type, start, num );
#else
		glDrawElements( type, num, GL_UNSIGNED_INT, &VertexIndexList[start] );
#endif 
	}

	// Constructor.
	UOpenGLRenderDevice::UOpenGLRenderDevice()
	{
		AllocatedTextures = 0;
	}

	// UObject interface.
	static void StaticConstructor( UClass* Class )
	{
		guard(UOpenGLRenderDevice::StaticConstructor);

		Super::StaticConstructor( Class );

		if ( Class==StaticClass() )
		{
			guard(AddProperties);
			
			new(Class,TEXT("ColorDepth"),          RF_Public)UIntProperty  ( CPP_PROPERTY(ColorDepth          ), TEXT("Options"), CPF_Config );
			new(Class,TEXT("AlwaysMipmap"),        RF_Public)UBoolProperty ( CPP_PROPERTY(AlwaysMipmap        ), TEXT("Options"), CPF_Config );
			new(Class,TEXT("SupportsLazyTextures"),RF_Public)UBoolProperty ( CPP_PROPERTY(SupportsLazyTextures), TEXT("Options"), CPF_Config );
			new(Class,TEXT("PrefersDeferredLoad"), RF_Public)UBoolProperty ( CPP_PROPERTY(PrefersDeferredLoad ), TEXT("Options"), CPF_Config );
			new(Class,TEXT("RefreshRate"),         RF_Public)UIntProperty  ( CPP_PROPERTY(RefreshRate         ), TEXT("Options"), CPF_Config );
			new(Class,TEXT("MaxTMUnits"),          RF_Public)UIntProperty  ( CPP_PROPERTY(MaxTMUnits          ), TEXT("Options"), CPF_Config );
			new(Class,TEXT("MaxAnisotropy"),       RF_Public)UIntProperty  ( CPP_PROPERTY(MaxAnisotropy       ), TEXT("Options"), CPF_Config );
			new(Class,TEXT("MaxLogUOverV"),        RF_Public)UIntProperty  ( CPP_PROPERTY(MaxLogUOverV        ), TEXT("Options"), CPF_Config );
			new(Class,TEXT("MaxLogVOverU"),        RF_Public)UIntProperty  ( CPP_PROPERTY(MaxLogVOverU        ), TEXT("Options"), CPF_Config );
			new(Class,TEXT("MinLogTextureSize"),   RF_Public)UIntProperty  ( CPP_PROPERTY(MinLogTextureSize   ), TEXT("Options"), CPF_Config );
			new(Class,TEXT("MaxLogTextureSize"),   RF_Public)UIntProperty  ( CPP_PROPERTY(MaxLogTextureSize   ), TEXT("Options"), CPF_Config );
			new(Class,TEXT("UseNoFiltering"),      RF_Public)UBoolProperty ( CPP_PROPERTY(UseNoFiltering      ), TEXT("Options"), CPF_Config );
			new(Class,TEXT("UsePrecache"),         RF_Public)UBoolProperty ( CPP_PROPERTY(UsePrecache         ), TEXT("Options"), CPF_Config );
			new(Class,TEXT("UseDetailTextures"),   RF_Public)UBoolProperty ( CPP_PROPERTY(UseDetailTextures   ), TEXT("Options"), CPF_Config );
			new(Class,TEXT("UseShareLists"),       RF_Public)UBoolProperty ( CPP_PROPERTY(UseShareLists       ), TEXT("Options"), CPF_Config );
			new(Class,TEXT("UseMultiTexture"),     RF_Public)UBoolProperty ( CPP_PROPERTY(UseMultiTexture     ), TEXT("Options"), CPF_Config );
			new(Class,TEXT("UseTrilinear"),        RF_Public)UBoolProperty ( CPP_PROPERTY(UseTrilinear        ), TEXT("Options"), CPF_Config );	
			new(Class,TEXT("UsePalette"),          RF_Public)UBoolProperty ( CPP_PROPERTY(UsePalette          ), TEXT("Options"), CPF_Config );
			new(Class,TEXT("UseAlphaPalette"),     RF_Public)UBoolProperty ( CPP_PROPERTY(UseAlphaPalette     ), TEXT("Options"), CPF_Config );
			new(Class,TEXT("UseVertexSpecular"),   RF_Public)UBoolProperty ( CPP_PROPERTY(UseVertexSpecular   ), TEXT("Options"), CPF_Config );
			new(Class,TEXT("UseS3TC"),             RF_Public)UBoolProperty ( CPP_PROPERTY(UseS3TC             ), TEXT("Options"), CPF_Config );
			new(Class,TEXT("Use4444Textures"),     RF_Public)UBoolProperty ( CPP_PROPERTY(Use4444Textures     ), TEXT("Options"), CPF_Config );
			new(Class,TEXT("UseFilterSGIS"),       RF_Public)UBoolProperty ( CPP_PROPERTY(UseFilterSGIS       ), TEXT("Options"), CPF_Config );
			new(Class,TEXT("UseDetailAlpha"),      RF_Public)UBoolProperty ( CPP_PROPERTY(UseDetailAlpha      ), TEXT("Options"), CPF_Config );
			new(Class,TEXT("LODBias"),             RF_Public)UFloatProperty( CPP_PROPERTY(LODBias             ), TEXT("Options"), CPF_Config );
			new(Class,TEXT("GammaOffset"),         RF_Public)UFloatProperty( CPP_PROPERTY(GammaOffset         ), TEXT("Options"), CPF_Config );
			unguard;

#if 0
			guard(ModifyDefaultObject);
			((UOpenGLRenderDevice*)Class->GetDefaultObject())->SpanBased             = 0;
			((UOpenGLRenderDevice*)Class->GetDefaultObject())->SupportsFogMaps       = 1;
			((UOpenGLRenderDevice*)Class->GetDefaultObject())->SupportsDistanceFog   = 0;
			((UOpenGLRenderDevice*)Class->GetDefaultObject())->FullscreenOnly        = 0;
			((UOpenGLRenderDevice*)Class->GetDefaultObject())->SupportsLazyTextures  = 0;
			((UOpenGLRenderDevice*)Class->GetDefaultObject())->PrefersDeferredLoad   = 0;
			((UOpenGLRenderDevice*)Class->GetDefaultObject())->UseDetailAlpha				 = 1;
			((UOpenGLRenderDevice*)Class->GetDefaultObject())->UseMultiTexture			 = 1;
			unguard;
#endif
		}

		unguard;
	}
	
	// Implementation.	
	void SetGamma( FLOAT GammaCorrection )
	{
		if ( GammaCorrection <= 0.1f )
			GammaCorrection = 1.f;

		GammaCorrection += GammaOffset;

		FGammaRamp Ramp;
		for( INT i=0; i<256; i++ )
		{
			Ramp.red[i] = Ramp.green[i] = Ramp.blue[i] = appRound(appPow(i/255.f,1.0f/(2.5f*GammaCorrection))*65535.f);
		}
#ifdef __LINUX__
		// vogel: FIXME (talk to Sam)
		// SDL_SetGammaRamp( Ramp.red, Ramp.green, Ramp.blue );		
		FLOAT gamma = 0.4 + 2 * GammaCorrection; 
		SDL_SetGamma( gamma, gamma, gamma );
#else
		if ( GammaFirstTime )
		{
			//OutputDebugString(TEXT("GetDeviceGammaRamp in SetGamma"));
			GetDeviceGammaRamp( hDC, &OriginalRamp );
			GammaFirstTime = false;
		}
		//OutputDebugString(TEXT("SetDeviceGammaRamp in SetGamma"));
		SetDeviceGammaRamp( hDC, &Ramp );
#endif	       
	}

	UBOOL FindExt( const TCHAR* Name )
	{
		guard(UOpenGLRenderDevice::FindExt);
		UBOOL Result = strstr( (char*) glGetString(GL_EXTENSIONS), appToAnsi(Name) ) != NULL;
		if( Result && !GIsEditor )
			debugf( NAME_Init, TEXT("Device supports: %s"), Name );
		return Result;
		unguard;
	}
	void FindProc( void*& ProcAddress, char* Name, char* SupportName, UBOOL& Supports, UBOOL AllowExt )
	{
		guard(UOpenGLRenderDevice::FindProc);
#ifdef __LINUX__
		if( !ProcAddress )
			ProcAddress = (void*) SDL_GL_GetProcAddress( Name );
#else
#if DYNAMIC_BIND
		if( !ProcAddress )
			ProcAddress = GetProcAddress( hModuleGlMain, Name );
		if( !ProcAddress )
			ProcAddress = GetProcAddress( hModuleGlGdi, Name );
#endif
		if( !ProcAddress && Supports && AllowExt )
			ProcAddress = wglGetProcAddress( Name );
#endif
		if( !ProcAddress )
		{
			if( Supports )
				debugf( TEXT("   Missing function '%s' for '%s' support"), appFromAnsi(Name), appFromAnsi(SupportName) );
			Supports = 0;
		}
		unguard;
	}
	void FindProcs( UBOOL AllowExt )
	{
		guard(UOpenGLDriver::FindProcs);
		#define GL_EXT(name) if( AllowExt ) SUPPORTS##name = FindExt( TEXT(#name)+1 );
		#define GL_PROC(ext,ret,func,parms) FindProc( *(void**)&func, #func, #ext, SUPPORTS##ext, AllowExt );
		#include "OpenGLFuncs.h"
		#undef GL_EXT
		#undef GL_PROC
		unguard;
	}
	UBOOL FailedInitf( const TCHAR* Fmt, ... )
	{
		TCHAR TempStr[4096];
		GET_VARARGS( TempStr, Fmt );
		debugf( NAME_Init, TempStr );
		Exit();
		return 0;
	}
	void MakeCurrent()
	{
		guard(UOpenGLRenderDevice::MakeCurrent);
#ifdef WIN32
		check(hRC);
		check(hDC);
		if( hCurrentRC!=hRC )
		{
			verify(wglMakeCurrent(hDC,hRC));
			hCurrentRC = hRC;
		}
#endif
		unguard;
	}

	void Check( const TCHAR* Tag )
	{
		for ( GLenum Error = glGetError(); Error!=GL_NO_ERROR; Error = glGetError() )
		{
			const TCHAR* Msg;
			switch( Error )
			{
				case GL_INVALID_ENUM:                  Msg = TEXT("GL_INVALID_ENUM");                  break;
				case GL_INVALID_VALUE:                 Msg = TEXT("GL_INVALID_VALUE");                 break;
				case GL_INVALID_OPERATION:             Msg = TEXT("GL_INVALID_OPERATION");             break;
				//case GL_INVALID_FRAMEBUFFER_OPERATION: Msg = TEXT("GL_INVALID_FRAMEBUFFER_OPERATION"); break;
				case GL_OUT_OF_MEMORY:                 Msg = TEXT("GL_OUT_OF_MEMORY");                 break;
				case GL_STACK_UNDERFLOW:               Msg = TEXT("GL_STACK_UNDERFLOW");               break;
				case GL_STACK_OVERFLOW:                Msg = TEXT("GL_STACK_OVERFLOW");                break;
				default:                               Msg = TEXT("UNKNOWN");                          break;
			};
			//appErrorf( TEXT("OpenGL Error: %s (%s)"), Msg, Tag );
			debugf( TEXT("OpenGL Error: %s (%s)"), Msg, Tag );
		}
	}
	
	void SetNoTexture( INT Multi )
	{
		guard(UOpenGLRenderDevice::SetNoTexture);
		if( TexInfo[Multi].CurrentCacheID != NoTextureId )
		{
			// Set small white texture.
			clock(BindCycles);
			glBindTexture( GL_TEXTURE_2D, NoTextureId );
			TexInfo[Multi].CurrentCacheID = NoTextureId;
			unclock(BindCycles);
		}
		unguard;
	}

	void SetAlphaTexture( INT Multi )
	{
		guard(UOpenGLRenderDevice::SetAlphaTexture);
		if( TexInfo[Multi].CurrentCacheID != AlphaTextureId )
		{
			// Set alpha gradient texture.
			clock(BindCycles);
			glBindTexture( GL_TEXTURE_2D, AlphaTextureId );
			TexInfo[Multi].CurrentCacheID = AlphaTextureId;
			unclock(BindCycles);
		}
		unguard;
	}

	void SetTexture( INT Multi, FTextureInfo& Info, DWORD PolyFlags, FLOAT PanBias )
	{
		guard(UOpenGLRenderDevice::SetTexture);
		// Set panning.
		FTexInfo& Tex = TexInfo[Multi];
		Tex.UPan      = Info.Pan.X + PanBias*Info.UScale;
		Tex.VPan      = Info.Pan.Y + PanBias*Info.VScale;

		// Find in cache.
		if( Info.CacheID==Tex.CurrentCacheID && !(Info.TextureFlags & TF_RealtimeChanged) )
			return;

		// Make current.
		clock(BindCycles);
		Tex.CurrentCacheID = Info.CacheID;
		FCachedTexture *Bind=BindMap->Find(Info.CacheID), *ExistingBind=Bind;
		if( !Bind )
		{			
			// Figure out OpenGL-related scaling for the texture.
			Bind            = &BindMap->Set( Info.CacheID, FCachedTexture() );
			glGenTextures( 1, &Bind->Id );
			AllocatedTextures++;
			Bind->BaseMip   = Min(0,Info.NumMips-1);
			Bind->UCopyBits = 0;
			Bind->VCopyBits = 0;
			Bind->UBits     = Info.Mips[Bind->BaseMip]->UBits;
			Bind->VBits     = Info.Mips[Bind->BaseMip]->VBits;			
			if( Bind->UBits-Bind->VBits > MaxLogUOverV )
			{
				Bind->VCopyBits += (Bind->UBits-Bind->VBits)-MaxLogUOverV;
				Bind->VBits      = Bind->UBits-MaxLogUOverV;
			}
			if( Bind->VBits-Bind->UBits > MaxLogVOverU )
			{
				Bind->UCopyBits += (Bind->VBits-Bind->UBits)-MaxLogVOverU;
				Bind->UBits      = Bind->VBits-MaxLogVOverU;
			}
			if( Bind->UBits < MinLogTextureSize )
			{
				Bind->UCopyBits += MinLogTextureSize - Bind->UBits;
				Bind->UBits     += MinLogTextureSize - Bind->UBits;
			}
			if( Bind->VBits < MinLogTextureSize )
			{
				Bind->VCopyBits += MinLogTextureSize - Bind->VBits;
				Bind->VBits     += MinLogTextureSize - Bind->VBits;
			}
			if( Bind->UBits > MaxLogTextureSize )
			{			
				Bind->BaseMip += Bind->UBits-MaxLogTextureSize;
				Bind->VBits   -= Bind->UBits-MaxLogTextureSize;
				Bind->UBits    = MaxLogTextureSize;
				if( Bind->VBits < 0 )
				{
					Bind->VCopyBits = -Bind->VBits;
					Bind->VBits     = 0;
				}
			}
			if( Bind->VBits > MaxLogTextureSize )
			{			
				Bind->BaseMip += Bind->VBits-MaxLogTextureSize;
				Bind->UBits   -= Bind->VBits-MaxLogTextureSize;
				Bind->VBits    = MaxLogTextureSize;
				if( Bind->UBits < 0 )
				{
					Bind->UCopyBits = -Bind->UBits;
					Bind->UBits     = 0;
				}
			}
		}
		glBindTexture( GL_TEXTURE_2D, Bind->Id );
		unclock(BindCycles);

		// Account for all the impact on scale normalization.
		Tex.UMult = 1.0 / (Info.UScale * (Info.USize << Bind->UCopyBits));
		Tex.VMult = 1.0 / (Info.VScale * (Info.VSize << Bind->VCopyBits));

		// Upload if needed.
		if( !ExistingBind || (Info.TextureFlags & TF_RealtimeChanged) )
		{
			// Cleanup texture flags.
			//if ( SupportsLazyTextures )
				//Info.Load();
			Info.TextureFlags &= ~TF_RealtimeChanged;

			// Set maximum color.
			// Info.CacheMaxColor();
			*Info.MaxColor = FColor(255,255,255,255); // Brandon's color hack.
			Bind->ColorNorm = Info.MaxColor->Plane();
			Bind->ColorNorm.W = 1;

			Bind->ColorRenorm = Bind->ColorNorm;

			// Generate the palette.
			FColor LocalPal[256], *NewPal=Info.Palette, TempColor(0,0,0,0);	
			UBOOL Paletted;
			if ( UseAlphaPalette )
				Paletted = UsePalette && Info.Palette;
			else
				Paletted = UsePalette && Info.Palette && !(PolyFlags & PF_Masked) && Info.Palette[0].A==255;

			if( Info.Palette )
			{
				TempColor = Info.Palette[0];
				if( PolyFlags & PF_Masked )
					Info.Palette[0] = FColor(0,0,0,0);
				NewPal = LocalPal;
				for( INT i=0; i<256; i++ )
				{
					FColor& Src = Info.Palette[i];
					NewPal[i].R = Src.R;
					NewPal[i].G = Src.G;
					NewPal[i].B = Src.B;
					NewPal[i].A = Src.A;
				}	
				if( Paletted )
					glColorTableEXT( GL_TEXTURE_2D, GL_RGBA, 256, GL_RGBA, GL_UNSIGNED_BYTE, NewPal );
			}

			// Download the texture.
			clock(ImageCycles);
			FMemMark Mark(GMem);
			BYTE* Compose  = New<BYTE>( GMem, (1<<(Bind->UBits+Bind->VBits))*4 );
			UBOOL SkipMipmaps = Info.NumMips==1 && !AlwaysMipmap;
		
			INT MaxLevel = Min(Bind->UBits,Bind->VBits) - MinLogTextureSize;

			for( INT Level=0; Level<=MaxLevel; Level++ )
			{
				// Convert the mipmap.
				INT MipIndex=Bind->BaseMip+Level, StepBits=0;
				if( MipIndex>=Info.NumMips )
				{
					StepBits = MipIndex - (Info.NumMips - 1);
					MipIndex = Info.NumMips - 1;
				}
				FMipmapBase* Mip      = Info.Mips[MipIndex];
				DWORD        Mask     = Mip->USize-1;			
				BYTE* 	     Src      = (BYTE*)Compose;
				GLuint       SourceFormat = GL_RGBA, InternalFormat = GL_RGBA8;
				if( Mip->DataPtr )
				{					
					if( (Info.Format == TEXF_DXT1) && SupportsTC )
					{
						guard(CompressedTexture_S3TC);
						InternalFormat = GL_COMPRESSED_RGB_S3TC_DXT1_EXT;				       
						unguard;
					}
					else if( Paletted )
					{
						guard(ConvertP8_P8);
						SourceFormat   = GL_COLOR_INDEX;
						InternalFormat = GL_COLOR_INDEX8_EXT;
						BYTE* Ptr      = (BYTE*)Compose;
						for( INT i=0; i<(1<<Max(0,Bind->VBits-Level)); i++ )
						{
							BYTE* Base = (BYTE*)Mip->DataPtr + ((i<<StepBits)&(Mip->VSize-1))*Mip->USize;
							for( INT j=0; j<(1<<Max(0,Bind->UBits-Level+StepBits)); j+=(1<<StepBits) )
								*Ptr++ = Base[j&Mask];
						}
						unguard;
					}
					else if( Info.Palette )
					{
						guard(ConvertP8_RGBA8888);					     
						SourceFormat   = GL_RGBA;
						if ( Use4444Textures )
							InternalFormat = GL_RGBA4; // vogel: will cause banding in menus
						else
							InternalFormat = GL_RGBA8;
						FColor* Ptr    = (FColor*)Compose;
						for( INT i=0; i<(1<<Max(0,Bind->VBits-Level)); i++ )
						{
							BYTE* Base = (BYTE*)Mip->DataPtr + ((i<<StepBits)&(Mip->VSize-1))*Mip->USize;
							for( INT j=0; j<(1<<Max(0,Bind->UBits-Level+StepBits)); j+=(1<<StepBits) )
								*Ptr++ = NewPal[Base[j&Mask]];						
						}
						unguard;
					}		
					else
					{
						guard(ConvertBGRA7777_RGBA8888);
						SourceFormat   = GL_RGBA;
						InternalFormat = GL_RGBA8;
						FColor* Ptr    = (FColor*)Compose;
						for( INT i=0; i<(1<<Max(0,Bind->VBits-Level)); i++ )
						{
							FColor* Base = (FColor*)Mip->DataPtr + Min<DWORD>((i<<StepBits)&(Mip->VSize-1),Info.VClamp-1)*Mip->USize;
							for( INT j=0; j<(1<<Max(0,Bind->UBits-Level+StepBits)); j+=(1<<StepBits) )
							{
								FColor& Src = Base[Min<DWORD>(j&Mask,Info.UClamp-1)];
								// vogel: optimize it.
								Ptr->R      = 2 * Src.B;
								Ptr->G      = 2 * Src.G;
								Ptr->B      = 2 * Src.R;
								Ptr->A      = 2 * Src.A; // because of 7777

								Ptr++;
							}
						}
						unguard;
					}
				}
				if( ExistingBind )
				{
					if ( (Info.Format == TEXF_DXT1) && SupportsTC )
					{
						guard(glCompressedTexSubImage2D);
						glCompressedTexSubImage2DARB( 
							GL_TEXTURE_2D, 
							Level, 
							0, 
							0, 
							1<<Max(0,Bind->UBits-Level), 
							1<<Max(0,Bind->VBits-Level), 
							InternalFormat,
							(1<<Max(0,Bind->UBits-Level)) * (1<<Max(0,Bind->VBits-Level)) / 2,
							Mip->DataPtr );
						unguard;
					}
					else
					{
						guard(glTexSubImage2D);
						glTexSubImage2D( 
							GL_TEXTURE_2D, 
							Level, 
							0, 
							0, 
							1<<Max(0,Bind->UBits-Level), 
							1<<Max(0,Bind->VBits-Level), 
							SourceFormat, 
							GL_UNSIGNED_BYTE, 
							Src );
						unguard;
					}
				}
				else
				{
					if ( (Info.Format == TEXF_DXT1) && SupportsTC )
					{
						guard(glCompressedTexImage2D);					
						glCompressedTexImage2DARB(
							GL_TEXTURE_2D, 
							Level, 
							InternalFormat, 
							1<<Max(0,Bind->UBits-Level), 
							1<<Max(0,Bind->VBits-Level), 
							0,
							(1<<Max(0,Bind->UBits-Level)) * (1<<Max(0,Bind->VBits-Level)) / 2,
							Mip->DataPtr );
						unguard;					
       					}
					else
					{
						guard(glTexImage2D);
						glTexImage2D( 
							GL_TEXTURE_2D, 
							Level, 
							InternalFormat, 
							1<<Max(0,Bind->UBits-Level), 
							1<<Max(0,Bind->VBits-Level), 
							0, 
							SourceFormat, 
							GL_UNSIGNED_BYTE, 
							Src );
						unguard;
					}	
				}
				if( SkipMipmaps )
					break;
			}

			Mark.Pop();
			unclock(ImageCycles);

			// Set texture state.
			if( UseNoFiltering )
			{
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			}
			else if( !(PolyFlags & PF_NoSmooth) )
			{
				if ( UseFilterSGIS && !SkipMipmaps )
				{
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_FILTER4_SGIS);
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_FILTER4_SGIS);
				}
				else
				{
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, SkipMipmaps ? GL_LINEAR : 
						(UseTrilinear ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR_MIPMAP_NEAREST) );
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
				}
				if ( MaxAnisotropy )
					glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, MaxAnisotropy );

			}
			else
			{
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, SkipMipmaps ? GL_NEAREST : GL_NEAREST_MIPMAP_NEAREST );
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			}

			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, MaxLevel < 0 ? 0 : MaxLevel);

			// Cleanup.
			if( Info.Palette )
				Info.Palette[0] = TempColor;
			//if( SupportsLazyTextures )
				//Info.Unload();
		}

		// Copy color norm.
		Tex.ColorNorm   = Bind->ColorNorm;
		Tex.ColorRenorm = Bind->ColorRenorm;

		unguard;
	}
	
	void SetBlend( DWORD PolyFlags )
	{
		guard(UOpenGLRenderDevice::SetBlend);

		if ( BufferedVerts > 0 )
			EndBuffering(); // flushes the vertex array!
		
		// Adjust PolyFlags according to Unreal's precedence rules.
		// Allows gouraud-polygonal fog only if specular is supported (1-pass fogging).
		if( (PolyFlags & (PF_RenderFog|PF_Translucent|PF_Modulated))!=PF_RenderFog || !UseVertexSpecular )
			PolyFlags &= ~PF_RenderFog;

		if( !(PolyFlags & (PF_Translucent|PF_Modulated)) )
			PolyFlags |= PF_Occlude;
		else if( PolyFlags & PF_Translucent )
			PolyFlags &= ~PF_Masked;

		// Detect changes in the blending modes.
		DWORD Xor = CurrentPolyFlags^PolyFlags;
		if( Xor & (PF_Translucent|PF_Modulated|PF_Invisible|PF_Occlude|PF_Masked|PF_Highlighted|PF_NoSmooth|PF_RenderFog|PF_Memorized) )
		{
			if( Xor & PF_Masked )
			{
				if ( PolyFlags & PF_Masked )
				{
					glEnable( GL_ALPHA_TEST );
				}
				else
				{
					glDisable( GL_ALPHA_TEST );
				}
			}
			if( Xor&(PF_Invisible|PF_Translucent|PF_Modulated|PF_Highlighted) )
			{
				if( !(PolyFlags & (PF_Invisible|PF_Translucent|PF_Modulated|PF_Highlighted)) )
				{
					glDisable( GL_BLEND );			
				}
				else if( PolyFlags & PF_Invisible )
				{
					glEnable( GL_BLEND );
					glBlendFunc( GL_ZERO, GL_ONE );			
			       	}
				else if( PolyFlags & PF_Translucent )
				{
					glEnable( GL_BLEND );
					glBlendFunc( GL_ONE, GL_ONE_MINUS_SRC_COLOR );				
				}
				else if( PolyFlags & PF_Modulated )
				{
					glEnable( GL_BLEND );
					glBlendFunc( GL_DST_COLOR, GL_SRC_COLOR );			
				}
				else if( PolyFlags & PF_Highlighted )
				{
					glEnable( GL_BLEND );
					glBlendFunc( GL_ONE, GL_ONE_MINUS_SRC_ALPHA );				
				}
			}
			if( Xor & PF_Invisible )
			{
				if ( PolyFlags & PF_Invisible )
					glColorMask( GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE );
				else
					glColorMask( GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE );		
			}
			if( Xor & PF_Occlude )
			{
				if ( PolyFlags & PF_Occlude )
					glDepthMask( GL_TRUE );
				else
					glDepthMask( GL_FALSE );
			}
			if( Xor & PF_NoSmooth )
			{
				//Direct3DDevice7->SetTextureStageState( 0, D3DTSS_MAGFILTER, (PolyFlags & PF_NoSmooth) ? D3DTFG_POINT : D3DTFG_LINEAR );
				//Direct3DDevice7->SetTextureStageState( 0, D3DTSS_MINFILTER, (PolyFlags & PF_NoSmooth) ? D3DTFN_POINT : D3DTFN_LINEAR );
			}
			if( Xor & PF_RenderFog )
			{
				//Direct3DDevice7->SetRenderState( D3DRENDERSTATE_SPECULARENABLE, (PolyFlags&PF_RenderFog)!=0 );
			}
			if( Xor & PF_Memorized )
			{
				// Switch back and forth from multitexturing.
				//Direct3DDevice7->SetTextureStageState( 1, D3DTSS_COLOROP, (PolyFlags&PF_Memorized) ? D3DTOP_MODULATE   : D3DTOP_DISABLE );
				//Direct3DDevice7->SetTextureStageState( 1, D3DTSS_ALPHAOP, (PolyFlags&PF_Memorized) ? D3DTOP_SELECTARG2 : D3DTOP_DISABLE );
			}
			CurrentPolyFlags = PolyFlags;
		}
		unguard;
	}

	void SetTexEnv( INT Multi, DWORD PolyFlags )
	{
		guard(UOpenGLRenderDevice::SetTexEnv);

		DWORD Xor = /*CurrentEnvFlags[Multi]^*/PolyFlags;

		if ( Xor & PF_Modulated )
		{
			if ( SUPPORTS_GL_EXT_texture_env_combine && (Multi!=0) )
			{
				glTexEnvf( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE_EXT );

				glTexEnvf( GL_TEXTURE_ENV, GL_COMBINE_RGB_EXT, GL_MODULATE );
				glTexEnvf( GL_TEXTURE_ENV, GL_COMBINE_ALPHA_EXT, GL_MODULATE );

				glTexEnvf( GL_TEXTURE_ENV, GL_SOURCE0_RGB_EXT, GL_TEXTURE );
				glTexEnvf( GL_TEXTURE_ENV, GL_SOURCE1_RGB_EXT, GL_PREVIOUS_EXT );
				//glTexEnvf( GL_TEXTURE_ENV, GL_SOURCE2_RGB_EXT, GL_TEXTURE );

				glTexEnvf( GL_TEXTURE_ENV, GL_SOURCE0_ALPHA_EXT, GL_TEXTURE );
				glTexEnvf( GL_TEXTURE_ENV, GL_SOURCE1_ALPHA_EXT, GL_PREVIOUS_EXT );
				//glTexEnvf( GL_TEXTURE_ENV, GL_SOURCE2_ALPHA_EXT, GL_TEXTURE );

				glTexEnvf( GL_TEXTURE_ENV, GL_RGB_SCALE_EXT, 2.0 );
			}
			else
			{
				glTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE );
			}
		}
		if ( Xor & PF_Highlighted )
		{
			if ( SUPPORTS_GL_NV_texture_env_combine4 )
			{
				// vogel: TODO: verify if NV_texture_env_combine4 is really needed or whether there is a better solution!
				// vogel: UPDATE: ATIX_texture_env_combine3's MODULATE_ADD should work too
				appErrorf( TEXT("IMPLEMENT ME") );
			}
			else
			{
				appErrorf( TEXT("don't call SetTexEnv with PF_Highlighted if NV_texture_env_combine4 is not supported") );
			}
		}		
		if ( Xor & PF_Memorized )	// Abused for detail texture approach
		{
			if ( UseDetailAlpha )
			{	
				glTexEnvf( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE_EXT );

				glTexEnvf( GL_TEXTURE_ENV, GL_COMBINE_RGB_EXT, GL_INTERPOLATE_EXT );	
				glTexEnvf( GL_TEXTURE_ENV, GL_COMBINE_ALPHA_EXT, GL_REPLACE );
	
				glTexEnvf( GL_TEXTURE_ENV, GL_SOURCE0_RGB_EXT, GL_TEXTURE );
				glTexEnvf( GL_TEXTURE_ENV, GL_SOURCE1_RGB_EXT, GL_PRIMARY_COLOR_EXT );
				glTexEnvf( GL_TEXTURE_ENV, GL_SOURCE2_RGB_EXT, GL_PREVIOUS_EXT );
	
				glTexEnvf( GL_TEXTURE_ENV, GL_SOURCE0_ALPHA_EXT, GL_TEXTURE );
				//glTexEnvf( GL_TEXTURE_ENV, GL_SOURCE1_ALPHA_EXT, GL_PRIMARY_COLOR_EXT );
				glTexEnvf( GL_TEXTURE_ENV, GL_SOURCE2_ALPHA_EXT, GL_PREVIOUS_EXT );
				glTexEnvf( GL_TEXTURE_ENV, GL_RGB_SCALE_EXT, 1.0 );
			}
			else
			{
				glTexEnvf( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE_EXT );

				glTexEnvf( GL_TEXTURE_ENV, GL_COMBINE_RGB_EXT, GL_INTERPOLATE_EXT );
				glTexEnvf( GL_TEXTURE_ENV, GL_COMBINE_ALPHA_EXT, GL_REPLACE );

				glTexEnvf( GL_TEXTURE_ENV, GL_SOURCE0_RGB_EXT, GL_TEXTURE );
				glTexEnvf( GL_TEXTURE_ENV, GL_SOURCE1_RGB_EXT, GL_PRIMARY_COLOR_EXT );
				glTexEnvf( GL_TEXTURE_ENV, GL_SOURCE2_RGB_EXT, GL_PRIMARY_COLOR_EXT );

				glTexEnvf( GL_TEXTURE_ENV, GL_SOURCE0_ALPHA_EXT, GL_TEXTURE );
				//glTexEnvf( GL_TEXTURE_ENV, GL_SOURCE1_ALPHA_EXT, GL_PRIMARY_COLOR_EXT );
				glTexEnvf( GL_TEXTURE_ENV, GL_SOURCE2_ALPHA_EXT, GL_PRIMARY_COLOR_EXT );
				glTexEnvf( GL_TEXTURE_ENV, GL_RGB_SCALE_EXT, 1.0 );
                        }       
		}

		CurrentEnvFlags[Multi] = PolyFlags;
		unguard;
	}

#ifdef __LINUX__
	// URenderDevice interface.
	UBOOL Init( UViewport* InViewport, INT NewX, INT NewY, INT NewColorBytes, UBOOL Fullscreen )
	{
		guard(UOpenGLRenderDevice::Init);

		debugf( TEXT("Initializing OpenGLDrv...") );
		GammaFirstTime = 1;
		// Init global GL.
		if( NumDevices==0 )
		{
			// Bind the library.
			FString OpenGLLibName;
			FString Section = TEXT("OpenGLDrv.OpenGLRenderDevice");
			// Default to libGL.so.1 if not defined
			if (!GConfig->GetString( *Section, TEXT("OpenGLLibName"), OpenGLLibName ))
				OpenGLLibName = TEXT("libGL.so.1");

			if ( !GLLoaded )
			{
				// Only call it once as succeeding calls will 'fail'.
				debugf( TEXT("binding %s"), *OpenGLLibName );
				if ( SDL_GL_LoadLibrary( *OpenGLLibName ) == -1 )
					appErrorf( TEXT(SDL_GetError()) );
  				GLLoaded = true;
			}

			SUPPORTS_GL = 1;
			FindProcs( 0 );
			if( !SUPPORTS_GL )
				return 0;		
		}
		NumDevices++;

		BindMap = UseShareLists ? &SharedBindMap : &LocalBindMap;
		Viewport = InViewport;
#if ! DRAW_ARRAYS	
		for( INT i = 0; i < VERTEX_ARRAY_SIZE; i++ )
			VertexIndexList[i] = i;
#endif
		// Try to change resolution to desired.
		if( !SetRes( NewX, NewY, NewColorBytes, Fullscreen ) )
			return FailedInitf( LocalizeError("ResFailed") );

		return 1;
		unguard;
	}
	
	void UnsetRes()
	{
		guard(UOpenGLRenderDevice::UnsetRes);
		Flush( 1 );
		unguard;
	}
#else
	UBOOL Init( UViewport* InViewport, INT NewX, INT NewY, INT NewColorBytes, UBOOL Fullscreen )
	{
		guard(UOpenGLRenderDevice::Init);
		debugf( TEXT("Initializing OpenGLDrv...") );

		GammaFirstTime = 1;
		// Get list of device modes.
		for( INT i=0; ; i++ )
		{
#if UNICODE
			if( !GUnicodeOS )
			{
				DEVMODEA Tmp;
				appMemzero(&Tmp,sizeof(Tmp));
				Tmp.dmSize = sizeof(Tmp);
				if( !EnumDisplaySettingsA(NULL,i,&Tmp) )
					break;
				Modes.AddUniqueItem( FPlane(Tmp.dmPelsWidth,Tmp.dmPelsHeight,Tmp.dmBitsPerPel,Tmp.dmDisplayFrequency) );
			}
			else
#endif
			{
				DEVMODE Tmp;
				appMemzero(&Tmp,sizeof(Tmp));
				Tmp.dmSize = sizeof(Tmp);
				if( !EnumDisplaySettings(NULL,i,&Tmp) )
					break;
				Modes.AddUniqueItem( FPlane(Tmp.dmPelsWidth,Tmp.dmPelsHeight,Tmp.dmBitsPerPel,Tmp.dmDisplayFrequency) );
			}
		}

		// Init global GL.
		if( NumDevices==0 )
		{
#if DYNAMIC_BIND
			// Find DLL's.
			hModuleGlMain = LoadLibraryA( GL_DLL );
			if( !hModuleGlMain )
			{
				debugf( NAME_Init, LocalizeError("NoFindGL"), appFromAnsi(GL_DLL) );
				return 0;
			}
			hModuleGlGdi = LoadLibraryA( "GDI32.dll" );
			check(hModuleGlGdi);

			// Find functions.
			SUPPORTS_GL = 1;
			FindProcs( 0 );
			if( !SUPPORTS_GL )
				return 0;
#endif
			
		}
		NumDevices++;

		// Init this GL rendering context.
		BindMap = UseShareLists ? &SharedBindMap : &LocalBindMap;
		Viewport = InViewport;
		hWnd = (HWND)InViewport->GetWindow();
		check(hWnd);
		hDC = GetDC( hWnd );
		check(hDC);
#if 0 /* Print all PFD's exposed */
		INT Count = DescribePixelFormat( hDC, 0, 0, NULL );
		for( i=1; i<Count; i++ )
			PrintFormat( hDC, i );
#endif
		if( !SetRes( NewX, NewY, NewColorBytes, Fullscreen ) )
			return FailedInitf( LocalizeError("ResFailed") );

		return 1;
		unguard;
	}
	
	void UnsetRes()
	{
		guard(UOpenGLRenderDevice::UnsetRes);
		check(hRC)
		hCurrentRC = NULL;

		// kaufel: hack fix for unrealed crashing at exit.
		if ( GIsEditor )
			wglMakeCurrent( NULL, NULL );
		else
			verify(wglMakeCurrent( NULL, NULL ));

		verify(wglDeleteContext( hRC ));
		verify(AllContexts.RemoveItem(hRC)==1);
		hRC = NULL;
		if( WasFullscreen )
			TCHAR_CALL_OS(ChangeDisplaySettings(NULL,0),ChangeDisplaySettingsA(NULL,0));
		unguard;
	}

	void PrintFormat( HDC hDC, INT nPixelFormat )
	{
		guard(UOpenGLRenderDevice::PrintFormat);
		TCHAR Flags[1024]=TEXT("");
		PIXELFORMATDESCRIPTOR pfd;
		DescribePixelFormat( hDC, nPixelFormat, sizeof(pfd), &pfd );
		if( pfd.dwFlags & PFD_DRAW_TO_WINDOW )
			appStrcat( Flags, TEXT(" PFD_DRAW_TO_WINDOW") );
		if( pfd.dwFlags & PFD_DRAW_TO_BITMAP )
			appStrcat( Flags, TEXT(" PFD_DRAW_TO_BITMAP") );
		if( pfd.dwFlags & PFD_SUPPORT_GDI )
			appStrcat( Flags, TEXT(" PFD_SUPPORT_GDI") );
		if( pfd.dwFlags & PFD_SUPPORT_OPENGL )
			appStrcat( Flags, TEXT(" PFD_SUPPORT_OPENGL") );
		if( pfd.dwFlags & PFD_GENERIC_ACCELERATED )
			appStrcat( Flags, TEXT(" PFD_GENERIC_ACCELERATED") );
		if( pfd.dwFlags & PFD_GENERIC_FORMAT )
			appStrcat( Flags, TEXT(" PFD_GENERIC_FORMAT") );
		if( pfd.dwFlags & PFD_NEED_PALETTE )
			appStrcat( Flags, TEXT(" PFD_NEED_PALETTE") );
		if( pfd.dwFlags & PFD_NEED_SYSTEM_PALETTE )
			appStrcat( Flags, TEXT(" PFD_NEED_SYSTEM_PALETTE") );
		if( pfd.dwFlags & PFD_DOUBLEBUFFER )
			appStrcat( Flags, TEXT(" PFD_DOUBLEBUFFER") );
		if( pfd.dwFlags & PFD_STEREO )
			appStrcat( Flags, TEXT(" PFD_STEREO") );
		if( pfd.dwFlags & PFD_SWAP_LAYER_BUFFERS )
			appStrcat( Flags, TEXT("PFD_SWAP_LAYER_BUFFERS") );
		debugf( NAME_Init, TEXT("Pixel format %i:"), nPixelFormat );
		debugf( NAME_Init, TEXT("   Flags:%s"), Flags );
		debugf( NAME_Init, TEXT("   Pixel Type: %i"), pfd.iPixelType );
		debugf( NAME_Init, TEXT("   Bits: Color=%i R=%i G=%i B=%i A=%i"), pfd.cColorBits, pfd.cRedBits, pfd.cGreenBits, pfd.cBlueBits, pfd.cAlphaBits );
		debugf( NAME_Init, TEXT("   Bits: Accum=%i Depth=%i Stencil=%i"), pfd.cAccumBits, pfd.cDepthBits, pfd.cStencilBits );
		unguard;
	}
#endif

	UBOOL SetRes( INT NewX, INT NewY, INT NewColorBytes, UBOOL Fullscreen )
	{
		guard(UOpenGLRenderDevice::SetRes);
		
		FString	Section = TEXT("OpenGLDrv.OpenGLRenderDevice");

#ifdef __LINUX__
		UnsetRes();    

		INT MinDepthBits;
		
		// Minimum size of the depth buffer
		if (!GConfig->GetInt( *Section, TEXT("MinDepthBits"), MinDepthBits ))
			MinDepthBits = 16;
		//debugf( TEXT("MinDepthBits = %i"), MinDepthBits );
		// 16 is the bare minimum.
		if ( MinDepthBits < 16 )
			MinDepthBits = 16; 

		SDL_GL_SetAttribute( SDL_GL_RED_SIZE, NewColorBytes<=2 ? 5 : 8 );
		SDL_GL_SetAttribute( SDL_GL_GREEN_SIZE, NewColorBytes<=2 ? 5 : 8 );
		SDL_GL_SetAttribute( SDL_GL_BLUE_SIZE, NewColorBytes<=2 ? 5 : 8 );
		SDL_GL_SetAttribute( SDL_GL_DEPTH_SIZE, MinDepthBits );
		SDL_GL_SetAttribute( SDL_GL_DOUBLEBUFFER, 1 );

		// Change window size.
		Viewport->ResizeViewport( Fullscreen ? (BLIT_Fullscreen|BLIT_OpenGL) : (BLIT_HardwarePaint|BLIT_OpenGL), NewX, NewY, NewColorBytes );
#else
		debugf( TEXT("Enter SetRes( %i, %i, %i, %i )"), NewX, NewY, NewColorBytes, Fullscreen );

		// If not fullscreen, and color bytes hasn't changed, do nothing.
		if( hRC && !Fullscreen && !WasFullscreen && NewColorBytes==Viewport->ColorBytes )
		{
			if( !Viewport->ResizeViewport( BLIT_HardwarePaint|BLIT_OpenGL, NewX, NewY, NewColorBytes ) )
				return 0;
			glViewport( 0, 0, NewX, NewY );
			return 1;
		}

		// Exit res.
		if( hRC )
		{
			debugf( TEXT("UnSetRes() -> hRc != NULL") );
			UnsetRes();
		}

		// Change display settings.
		if( Fullscreen )
		{
			INT FindX=NewX, FindY=NewY, BestError = MAXINT;
			for( INT i=0; i<Modes.Num(); i++ )
			{
				if( Modes(i).Z==NewColorBytes*8 )
				{
					INT Error
					=	(Modes(i).X-FindX)*(Modes(i).X-FindX)
					+	(Modes(i).Y-FindY)*(Modes(i).Y-FindY);
					if( Error < BestError )
					{
						NewX      
						= Modes(i).X;
						NewY      = Modes(i).Y;
						BestError = Error;
					}
				}
			}
#if UNICODE
			if( !GUnicodeOS )
			{
				DEVMODEA dm;
				ZeroMemory( &dm, sizeof(dm) );
				dm.dmSize       = sizeof(dm);
				dm.dmPelsWidth  = NewX;
				dm.dmPelsHeight = NewY;
				//dm.dmBitsPerPel = NewColorBytes * 8;
				if ( RefreshRate )
				{
					dm.dmDisplayFrequency = RefreshRate;
					dm.dmFields = DM_PELSWIDTH | DM_PELSHEIGHT | DM_DISPLAYFREQUENCY;// | DM_BITSPERPEL;
				}
				else
				{
					dm.dmFields = DM_PELSWIDTH | DM_PELSHEIGHT;// | DM_BITSPERPEL;
				}
				if( ChangeDisplaySettingsA( &dm, CDS_FULLSCREEN )!=DISP_CHANGE_SUCCESSFUL )
				{
					debugf( TEXT("ChangeDisplaySettingsA failed: %ix%i"), NewX, NewY );
					return 0;
				}
			}
			else
#endif
			{
				DEVMODE dm;
				ZeroMemory( &dm, sizeof(dm) );
				dm.dmSize       = sizeof(dm);
				dm.dmPelsWidth  = NewX;
				dm.dmPelsHeight = NewY;
				dm.dmBitsPerPel = NewColorBytes * 8;
				if ( RefreshRate )
				{
					dm.dmDisplayFrequency = RefreshRate;
					dm.dmFields = DM_PELSWIDTH | DM_PELSHEIGHT | DM_DISPLAYFREQUENCY;// | DM_BITSPERPEL;
				}
				else
				{
					dm.dmFields = DM_PELSWIDTH | DM_PELSHEIGHT;// | DM_BITSPERPEL;
				}
				if( ChangeDisplaySettings( &dm, CDS_FULLSCREEN )!=DISP_CHANGE_SUCCESSFUL )
				{
					debugf( TEXT("ChangeDisplaySettings failed: %ix%i"), NewX, NewY );
					return 0;
				}
			}
		}

		// Change window size.
		UBOOL Result = Viewport->ResizeViewport( Fullscreen ? (BLIT_Fullscreen|BLIT_OpenGL) : (BLIT_HardwarePaint|BLIT_OpenGL), NewX, NewY, NewColorBytes );
		if( !Result )
		{
			if( Fullscreen )
				TCHAR_CALL_OS(ChangeDisplaySettings(NULL,0),ChangeDisplaySettingsA(NULL,0));
			return 0;
		}

		// Set res.
		INT DesiredColorBits   = NewColorBytes<=2 ? 16 : 24; // vogel: changed to saner values 
		INT DesiredStencilBits = 0;                          // NewColorBytes<=2 ? 0  : 8;
		INT DesiredDepthBits   = NewColorBytes<=2 ? 16 : 24; // NewColorBytes<=2 ? 16 : 32;
		PIXELFORMATDESCRIPTOR pfd =
		{
			sizeof(PIXELFORMATDESCRIPTOR),
			1,
			PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,
			PFD_TYPE_RGBA,
			DesiredColorBits,
			0,0,0,0,0,0,
			0,0,
			0,0,0,0,0,
			DesiredDepthBits,
			DesiredStencilBits,
			0,
			PFD_MAIN_PLANE,
			0,
			0,0,0
		};
		INT nPixelFormat = ChoosePixelFormat( hDC, &pfd );
		Parse( appCmdLine(), TEXT("PIXELFORMAT="), nPixelFormat );
		debugf( NAME_Init, TEXT("Using pixel format %i"), nPixelFormat );
		check(nPixelFormat);
		verify(SetPixelFormat( hDC, nPixelFormat, &pfd ));
		hRC = wglCreateContext( hDC );
		check(hRC);
		MakeCurrent();
		if( UseShareLists && AllContexts.Num() )
			verify(wglShareLists(AllContexts(0),hRC)==1);
		AllContexts.AddItem(hRC);
#endif

		// Get info and extensions.
	
		//PrintFormat( hDC, nPixelFormat );
		debugf( NAME_Init, TEXT("GL_VENDOR     : %s"), appFromAnsi((ANSICHAR*)glGetString(GL_VENDOR)) );
		debugf( NAME_Init, TEXT("GL_RENDERER   : %s"), appFromAnsi((ANSICHAR*)glGetString(GL_RENDERER)) );
		debugf( NAME_Init, TEXT("GL_VERSION    : %s"), appFromAnsi((ANSICHAR*)glGetString(GL_VERSION)) );

		if ( !GIsEditor )
		{
			ANSICHAR Temp[1024];
			INT TempLen;
			FString TempString;
			ANSICHAR* LastSpace;
			ANSICHAR* GLString = (ANSICHAR*)glGetString( GL_EXTENSIONS );
			INT GLStringLen = strlen( GLString );

			while ( GLStringLen > 0 )
			{
				strncpy(Temp,GLString,960);
				Temp[960]    = 0;
				TempLen      = strlen( Temp );
				if ( TempLen < 960 )
				{
					debugf( NAME_Init, TEXT("GL_EXTENSIONS : %s"), appFromAnsi( Temp ) );
					break;
				}
				else
				{
					LastSpace  = (ANSICHAR*)strrchr( Temp, ' ' );
					*LastSpace = 0;
					debugf( NAME_Init, TEXT("GL_EXTENSIONS : %s"), appFromAnsi( Temp ) );
					GLString		+= LastSpace-Temp+1;
					GLStringLen -= LastSpace-Temp+1;
				}
			}
		}

		FindProcs( 1 );

		// Set modelview.
		glMatrixMode( GL_MODELVIEW );
		glLoadIdentity();
		FLOAT Matrix[16] =
		{
			+1, +0, +0, +0,
			+0, -1, +0, +0,
			+0, +0, -1, +0,
			+0, +0, +0, +1,
		};
		glMultMatrixf( Matrix );

		SetGamma( Viewport->GetOuterUClient()->Brightness );

		UseVertexSpecular       = 1;
		AlwaysMipmap            = 0;
		SupportsTC              = UseS3TC;
		UseCVA	                = 1;

		SUPPORTS_GL_EXT_texture_env_combine |= SUPPORTS_GL_ARB_texture_env_combine;
			
		// Validate flags.
		if( !SUPPORTS_GL_ARB_multitexture )
			UseMultiTexture = 0;
		if( !SUPPORTS_GL_EXT_paletted_texture )
			UsePalette = 0;
		if( !SUPPORTS_GL_EXT_texture_compression_s3tc || !SUPPORTS_GL_ARB_texture_compression)
		        SupportsTC = 0;
		if( !SUPPORTS_GL_EXT_texture_env_combine )
			UseDetailTextures = 0;
		if( !SUPPORTS_GL_EXT_compiled_vertex_array )
			UseCVA = 0;
		if( !SUPPORTS_GL_EXT_secondary_color )
			UseVertexSpecular = 0;
		if( !SUPPORTS_GL_EXT_texture_filter_anisotropic )
			MaxAnisotropy  = 0;
		if( !SUPPORTS_GL_SGIS_texture_lod )
			UseFilterSGIS = 0;
		if( !SUPPORTS_GL_EXT_texture_env_combine )
			UseDetailAlpha = 0;
		if( !SUPPORTS_GL_EXT_texture_lod_bias )
			LODBias = 0;

		if ( !MaxTMUnits || (MaxTMUnits > MAX_TMUNITS) )
		{
			MaxTMUnits = MAX_TMUNITS;
		}

		if ( UseMultiTexture )
		{
			glGetIntegerv( GL_MAX_TEXTURE_UNITS_ARB, &TMUnits );
			debugf( TEXT("%i Texture Mapping Units found"), TMUnits );
			if (TMUnits > MaxTMUnits)
				TMUnits = MaxTMUnits;				
		}
		else			
		{
			TMUnits = 1;
			UseDetailAlpha = 0;			
		}
		
		if ( MaxAnisotropy )
		{
			INT tmp;
			glGetIntegerv( GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &tmp );
			debugf( TEXT("MaxAnisotropy = %i"), tmp ); 
			if ( MaxAnisotropy > tmp )
				MaxAnisotropy = tmp;

			UseTrilinear = true; // Anisotropic filtering doesn't make much sense without trilinear filtering
		}

		BufferActorTris = UseVertexSpecular; // Only buffer when we can use 1 pass fogging

		if ( SupportsTC )
			debugf( TEXT("Trying to use S3TC extension.") );

		// Special treatment for texture size stuff.
		if (!GetConfigInt( *Section, TEXT("MinLogTextureSize"), MinLogTextureSize ))
			MinLogTextureSize=0;
		if (!GetConfigInt( *Section, TEXT("MaxLogTextureSize"), MaxLogTextureSize ))
			MaxLogTextureSize=8;

		INT MaxTextureSize;
		glGetIntegerv(GL_MAX_TEXTURE_SIZE, &MaxTextureSize);		
		INT Dummy = -1;
		while (MaxTextureSize > 0)
		{
			MaxTextureSize >>= 1;
			Dummy++;
		}
		
		if ( (MaxLogTextureSize > Dummy) || (SupportsTC) )
			MaxLogTextureSize = Dummy;
		if ( (MinLogTextureSize < 2) || (SupportsTC) )
			MinLogTextureSize = 2;	

		if ( SupportsTC )
		{
			MaxLogUOverV = MaxLogTextureSize;
			MaxLogVOverU = MaxLogTextureSize;
		}
		else
		{
			MaxLogUOverV = 8;
			MaxLogVOverU = 8;
		}

		debugf( TEXT("MinLogTextureSize = %i"), MinLogTextureSize );
		debugf( TEXT("MaxLogTextureSize = %i"), MaxLogTextureSize );


		// Verify hardware defaults.
		check(MinLogTextureSize>=0);
		check(MaxLogTextureSize>=0);
		check(MinLogTextureSize<MaxLogTextureSize);
		check(MinLogTextureSize<=MaxLogTextureSize);

		// Flush textures.
		Flush(1);

		// Bind little white RGBA texture to ID 0.
		FColor Data[8*8];
		for( INT i=0; i<ARRAY_COUNT(Data); i++ )
			Data[i] = FColor(255,255,255,255);
		glGenTextures( 1, &NoTextureId );
		SetNoTexture( 0 );
		for( INT Level=0; 8>>Level; Level++ )
			glTexImage2D( GL_TEXTURE_2D, Level, 4, 8>>Level, 8>>Level, 0, GL_RGBA, GL_UNSIGNED_BYTE, Data );

		// Set permanent state.
		glEnable( GL_DEPTH_TEST );
		glShadeModel( GL_SMOOTH );
		glEnable( GL_TEXTURE_2D );
		glAlphaFunc( GL_GREATER, 0.5 );
		glDepthMask( GL_TRUE );
		glDepthFunc( GL_LEQUAL );
		glBlendFunc( GL_ONE, GL_ZERO );
		glEnable( GL_BLEND );
		glEnable( GL_DITHER );
		glPolygonOffset( -1.0f, -1.0 );
		if ( LODBias )
		{		
			glTexEnvf( GL_TEXTURE_FILTER_CONTROL_EXT, GL_TEXTURE_LOD_BIAS_EXT, LODBias );
			if ( SUPPORTS_GL_ARB_multitexture )
			{
				for (INT i=1; i<TMUnits; i++)
				{					
					glActiveTextureARB( GL_TEXTURE0_ARB + i);
					glTexEnvf( GL_TEXTURE_FILTER_CONTROL_EXT, GL_TEXTURE_LOD_BIAS_EXT, LODBias );
				}
				glActiveTextureARB( GL_TEXTURE0_ARB );	
			}
		}
		if ( UseDetailAlpha )			// vogel: alpha texture for better detail textures (no vertex alpha)
		{                                                               	
			BYTE AlphaData[256];
			for (INT ac=0; ac<256; ac++)
				AlphaData[ac] = 255 - ac;
			glGenTextures( 1, &AlphaTextureId );
			// vogel: could use 1D texture but opted against (for minor reasons)
			glBindTexture( GL_TEXTURE_2D, AlphaTextureId );
			glTexImage2D( GL_TEXTURE_2D, 0, GL_ALPHA, 256, 1, 0, GL_ALPHA, GL_UNSIGNED_BYTE, AlphaData );
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		}
		glEnableClientState( GL_VERTEX_ARRAY );
		glVertexPointer( 3, GL_FLOAT, sizeof(FGLVertex), &VertexArray[0].x );
		glEnableClientState( GL_TEXTURE_COORD_ARRAY );
		glTexCoordPointer( 2, GL_FLOAT, sizeof(FGLTexCoord), &TexCoordArray[0].TMU[0].u );
		if ( UseMultiTexture )
		{
			glClientActiveTextureARB( GL_TEXTURE1_ARB );
			glTexCoordPointer( 2, GL_FLOAT, sizeof(FGLTexCoord), &TexCoordArray[0].TMU[1].u );
			if ( TMUnits > 2 )
			{	
				glClientActiveTextureARB( GL_TEXTURE2_ARB );
				glTexCoordPointer( 2, GL_FLOAT, sizeof(FGLTexCoord), &TexCoordArray[0].TMU[2].u );
			}
			glClientActiveTextureARB( GL_TEXTURE0_ARB );			
		}
		glColorPointer( 4, GL_UNSIGNED_BYTE, sizeof(FGLColor), &ColorArray[0].color );
		if ( UseVertexSpecular )
			glSecondaryColorPointerEXT( 3, GL_UNSIGNED_BYTE, sizeof(FGLColor), &ColorArray[0].specular );

		// Init variables.
		BufferedVerts      = 0;
		ColorArrayEnabled  = 0;
		RenderFog          = 0;

		CurrentPolyFlags   = PF_Occlude;
		for (INT TMU=0; TMU<MAX_TMUNITS; TMU++)
			CurrentEnvFlags[TMU] = 0;

		// Remember fullscreenness.
		WasFullscreen = Fullscreen;
		return 1;

		unguard;
	}
	
	void Exit()
	{
		guard(UOpenGLRenderDevice::Exit);
		check(NumDevices>0);

		// Shut down RC.
		Flush( 0 );

#ifdef __LINUX__
		UnsetRes();

		// Shut down global GL.
		if( --NumDevices==0 )
		{
			SharedBindMap.~TMap<QWORD,FCachedTexture>();		
		}
#else
		if( hRC )
			UnsetRes();

		// vogel: UClient::Destroy is called before this gets called so hDC is invalid
		//OutputDebugString(TEXT("SetDeviceGammaRamp in Exit"));
		SetDeviceGammaRamp( GetDC( GetDesktopWindow() ), &OriginalRamp );

		// Shut down this GL context. May fail if window was already destroyed.
		if( hDC )
			ReleaseDC(hWnd,hDC);

		// Shut down global GL.
		if( --NumDevices==0 )
		{
#if DYNAMIC_BIND && 0 /* Broken on some drivers */
			// Free modules.
			if( hModuleGlMain )
				verify(FreeLibrary( hModuleGlMain ));
			if( hModuleGlGdi )
				verify(FreeLibrary( hModuleGlGdi ));
#endif
			SharedBindMap.~TMap<QWORD,FCachedTexture>();
			AllContexts.~TArray<HGLRC>();
		}
#endif
		unguard;
	}
	
	void ShutdownAfterError()
	{
		guard(UOpenGLRenderDevice::ShutdownAfterError);
		debugf( NAME_Exit, TEXT("UOpenGLRenderDevice::ShutdownAfterError") );
		//ChangeDisplaySettings( NULL, 0 );
		unguard;
	}

	void Flush()
	{
		Flush( 1 );
	}
	
	void Flush( UBOOL AllowPrecache )
	{
		guard(UOpenGLRenderDevice::Flush);
		TArray<GLuint> Binds, UniqueBinds;
		for( TMap<QWORD,FCachedTexture>::TIterator It(*BindMap); It; ++It )
		{
			UniqueBinds.AddUniqueItem( It->Value.Id );
			Binds.AddItem( It->Value.Id );
		}
		BindMap->Empty();

		// kaufel: check if using non addunique version makes sense this.
		guard(CheckBindsNum);
		check(UniqueBinds.Num()==Binds.Num());
		unguardf(( TEXT("Binds.Num() = %i, UniqueBinds.Num() = %i, AllocatedTextures = %i"), Binds.Num(), UniqueBinds.Num(), AllocatedTextures ));

		// vogel: FIXME: add 0 and AlphaTextureId
		if ( Binds.Num() )
			glDeleteTextures( Binds.Num(), (GLuint*)&Binds(0) );
		AllocatedTextures = 0;
		if( AllowPrecache && UsePrecache && !GIsEditor )
			PrecacheOnFlip = 1;
		SetGamma( Viewport->GetOuterUClient()->Brightness );
		unguardf(( TEXT("( AllowPrecache = %i)"), AllowPrecache ));
	}
	
	static QSORT_RETURN CDECL CompareRes( const FPlane* A, const FPlane* B )
	{
		return (QSORT_RETURN) ( (A->X-B->X)!=0.0 ? (A->X-B->X) : (A->Y-B->Y) );
	}
	
	UBOOL Exec( const TCHAR* Cmd, FOutputDevice& Ar )
	{
		guard(UOpenGLRenderDevice::Exec);
		if( ParseCommand(&Cmd,TEXT("DGL")) )
		{
			if( ParseCommand(&Cmd,TEXT("BUFFERTRIS")) ) 
			{
				BufferActorTris = !BufferActorTris;
				if ( !UseVertexSpecular )
					BufferActorTris = 0;
				debugf( TEXT("BUFFERTRIS [%i]"), BufferActorTris );
			}			
			if( ParseCommand(&Cmd,TEXT("CVA")) ) 
			{
				UseCVA = !UseCVA;
				if ( !SUPPORTS_GL_EXT_compiled_vertex_array )
					UseCVA = 0;				
				debugf( TEXT("CVA [%i]"), UseCVA );
			}
			if( ParseCommand(&Cmd,TEXT("TRILINEAR")) ) 
			{
				UseTrilinear = !UseTrilinear;
				debugf( TEXT("TRILINEAR [%i]"), UseTrilinear );
				Flush(1);
			}
			if( ParseCommand(&Cmd,TEXT("RGBA4444")) ) 
			{
				Use4444Textures = !Use4444Textures;
				debugf( TEXT("RGBA4444 [%i]"), Use4444Textures );
				Flush(1);
			}
			if( ParseCommand(&Cmd,TEXT("MULTITEX")) ) 
			{
				UseMultiTexture = !UseMultiTexture;				
				if( !SUPPORTS_GL_ARB_multitexture )
					UseMultiTexture = 0;
				if ( UseMultiTexture )
					glGetIntegerv( GL_MAX_TEXTURE_UNITS_ARB, &TMUnits );
				else
					TMUnits = 1;
				debugf( TEXT("MULTITEX [%i]"), UseMultiTexture );
			}
			if( ParseCommand(&Cmd,TEXT("DETAILTEX")) ) 
			{
				UseDetailTextures = !UseDetailTextures;
				if( !SUPPORTS_GL_EXT_texture_env_combine )
					UseDetailTextures = 0;
				debugf( TEXT("DETAILTEX [%i]"), UseDetailTextures );
			}
			if( ParseCommand(&Cmd,TEXT("DETAILALPHA")) ) 
			{
				UseDetailAlpha = !UseDetailAlpha;
				if( !SUPPORTS_GL_EXT_texture_env_combine )
					UseDetailAlpha = 0;
				debugf( TEXT("DETAILALPHA [%i]"), UseDetailAlpha );
			}
			if( ParseCommand(&Cmd,TEXT("BUILD")) ) 
			{
#ifdef __LINUX__
				debugf( TEXT("OpenGL renderer built: %s %s"), __DATE__, __TIME__ );
#else
				debugf( TEXT("OpenGL renderer built: %s"), appFromAnsi(__DATE__ " " __TIME__) );
#endif
			}
			return 1;	// vogel: FIXME
		}
		else if( ParseCommand(&Cmd,TEXT("GetRes")) )
		{
#ifdef __LINUX__
			// Changing Resolutions:
			// Entries in the resolution box in the console is
			// apparently controled building a string of relevant 
			// resolutions and sending it to the engine via Ar.Log()

			// Here I am querying SDL_ListModes for available resolutions,
			// and populating the dropbox with its output.
			FString Str = "";
			SDL_Rect **modes;
			INT i,j;

 			// Available fullscreen video modes
			modes = SDL_ListModes(NULL, SDL_FULLSCREEN);
                        
			if ( modes == (SDL_Rect **)0 ) 
			{
				debugf( NAME_Init, TEXT("No available fullscreen video modes") );
			} 
			else if ( modes == (SDL_Rect **)-1 ) 
			{
				debugf( NAME_Init, TEXT("No special fullscreen video modes") );
			} 
			else 
			{
				// count the number of available modes
				for ( i=0,j=0; modes[i]; ++i )	
					++j;
						
				// Load the string with resolutions from smallest to 
				// largest. SDL_ListModes() provides them from lg
				// to sm...
				for ( i=(j-1); i >= 0; --i )
					Str += FString::Printf( TEXT("%ix%i "), modes[i]->w, modes[i]->h);
			}
			
			// Send the resolution string to the engine.	
			Ar.Log( *Str.LeftChop(1) );
			return 1;
#else
			TArray<FPlane> Relevant;
			INT i;
			for( i=0; i<Modes.Num(); i++ )
				if( Modes(i).Z==Viewport->ColorBytes*8 )
					if
					(	(Modes(i).X!=320 || Modes(i).Y!=200)
					&&	(Modes(i).X!=640 || Modes(i).Y!=400) )
					Relevant.AddUniqueItem(FPlane(Modes(i).X,Modes(i).Y,0,0));
			appQsort( &Relevant(0), Relevant.Num(), sizeof(FPlane), (QSORT_COMPARE)CompareRes );
			FString Str;
			for( i=0; i<Relevant.Num(); i++ )
			{
				TCHAR Temp[4096];
				appSprintf( Temp, TEXT("%ix%i"), (INT)Relevant(i).X, (INT)Relevant(i).Y );
				Str += Temp;
				if ( i < Relevant.Num()-1 )
					Str += TEXT(" "); 
			}
			Ar.Log( *Str );
			return 1;
#endif
		}
		return 0;
		unguard;
	}

	
	void Lock( FPlane InFlashScale, FPlane InFlashFog, FPlane ScreenClear, DWORD RenderLockFlags, BYTE* InHitData, INT* InHitSize )
	{
		guard(UOpenGLRenderDevice::Lock);
		check(LockCount==0);
		BindCycles = ImageCycles = ComplexCycles = GouraudCycles = TileCycles = 0;
		++LockCount;

		// Make this context current.
		MakeCurrent();

		// Clear the Z buffer if needed.
		glClearColor( ScreenClear.X, ScreenClear.Y, ScreenClear.Z, ScreenClear.W );
		glClearDepth( 1.0 );
		glDepthRange( 0.0, 1.0 );
		SetBlend( PF_Occlude );
		glClear( GL_DEPTH_BUFFER_BIT|((RenderLockFlags&LOCKR_ClearScreen)?GL_COLOR_BUFFER_BIT:0) );

		glDepthFunc( GL_LEQUAL );

		// Remember stuff.
		FlashScale = InFlashScale;
		FlashFog   = InFlashFog;
		HitData    = InHitData;
		HitSize    = InHitSize;
		if( HitData )
		{
			*HitSize = 0;
			if( !GLHitData.Num() )
				GLHitData.Add( 16384 );
			glSelectBuffer( GLHitData.Num(), (GLuint*)&GLHitData(0) );
			glRenderMode( GL_SELECT );
			glInitNames();
		}
		unguard;
	}
	
	void SetSceneNode( FSceneNode* Frame )
	{
		guard(UOpenGLRenderDevice::SetSceneNode);

		EndBuffering();		// Flush vertex array before changing the projection matrix!

		// Precompute stuff.
		Aspect      = Frame->FY/Frame->FX;
		RProjZ      = appTan( Viewport->Actor->FovAngle * PI/360.0 );
		RFX2        = 2.0*RProjZ/Frame->FX;
		RFY2        = 2.0*RProjZ*Aspect/Frame->FY;

		// Set viewport.
		glViewport( Frame->XB, Viewport->SizeY-Frame->Y-Frame->YB, Frame->X, Frame->Y );

		// Set projection.
		glMatrixMode( GL_PROJECTION );
		glLoadIdentity();
#if 1
		if ( Frame->Viewport->IsOrtho() )
		{
			if ( GIsEditor )
				glOrtho( -RProjZ / 1.0, +RProjZ / 1.0, -Aspect*RProjZ / 1.0, +Aspect*RProjZ / 1.0, 1.0 / 1.0, 32768.0 );
			else
				glOrtho( -RProjZ / 2.0, +RProjZ / 2.0, -Aspect*RProjZ / 2.0, +Aspect*RProjZ / 2.0, 1.0 / 2.0, 32768.0 );
		}
		else
			glFrustum( -RProjZ / 2.0, +RProjZ / 2.0, -Aspect*RProjZ / 2.0, +Aspect*RProjZ / 2.0, 1.0 / 2.0, 32768.0 );
#else
		if ( Frame->Viewport->IsOrtho() )
			glOrtho( -RProjZ / 2.0, +RProjZ / 2.0, -Aspect*RProjZ / 2.0, +Aspect*RProjZ / 2.0, 1.0 / 2.0, 32768.0 );
		else			
			glFrustum( -RProjZ / 2.0, +RProjZ / 2.0, -Aspect*RProjZ / 2.0, +Aspect*RProjZ / 2.0, 1.0 / 2.0, 32768.0 );
#endif
		// Set clip planes.
		if( HitData )
		{
			FVector N[4];
			N[0] = (FVector((Viewport->HitX-Frame->FX2)*Frame->RProj.Z,0,1)^FVector(0,-1,0)).SafeNormal();
			N[1] = (FVector((Viewport->HitX+Viewport->HitXL-Frame->FX2)*Frame->RProj.Z,0,1)^FVector(0,+1,0)).SafeNormal();
			N[2] = (FVector(0,(Viewport->HitY-Frame->FY2)*Frame->RProj.Z,1)^FVector(+1,0,0)).SafeNormal();
			N[3] = (FVector(0,(Viewport->HitY+Viewport->HitYL-Frame->FY2)*Frame->RProj.Z,1)^FVector(-1,0,0)).SafeNormal();
			for( INT i=0; i<4; i++ )
			{
				double D0[4]={N[i].X,N[i].Y,N[i].Z,0};
				glClipPlane( (GLenum) (GL_CLIP_PLANE0+i), D0 );
				glEnable( (GLenum) (GL_CLIP_PLANE0+i) );
			}
		}
		unguard;
	}

	void Unlock( UBOOL Blit )
	{
		guard(UOpenGLRenderDevice::Unlock);
		EndBuffering();

		// Unlock and render.
		check(LockCount==1);
		//glFlush();
		if( Blit )
		{
			Check( TEXT("please report this bug") );
#ifdef __LINUX__
			SDL_GL_SwapBuffers();
#else
			verify(SwapBuffers( hDC ));
#endif
		}
		--LockCount;

		// Hits.
		if( HitData )
		{
			INT Records = glRenderMode( GL_RENDER );
			INT* Ptr = &GLHitData(0);
			DWORD BestDepth = MAXDWORD;
			for( INT i=0; i<Records; i++ )
			{
				INT   NameCount = *Ptr++;
				DWORD MinDepth  = *Ptr++;
				DWORD MaxDepth  = *Ptr++;
				if( MinDepth<=BestDepth )
				{
					BestDepth = MinDepth;
					*HitSize = 0;
					for( INT i=0; i<NameCount; )
					{
						INT Count = Ptr[i++];
						for( INT j=0; j<Count; j+=4 )
							*(INT*)(HitData+*HitSize+j) = Ptr[i++];
						*HitSize += Count;
					}
					check(i==NameCount);
				}
				Ptr += NameCount;
				(void)MaxDepth;
			}
			for( i=0; i<4; i++ )
				glDisable( GL_CLIP_PLANE0+i );
		}

		unguard;
	}

	void RenderPasses()
	{
		guard(UOpenGLRenderDevice::RenderPasses);
		if ( PassCount == 0 )
			return;

		FPlane Color( 1.0f, 1.0f, 1.0f, 1.0f );
		INT i;
	             		
		SetBlend( MultiPass.TMU[0].PolyFlags );

		for (i=0; i<PassCount; i++)
		{
			glActiveTextureARB( GL_TEXTURE0_ARB + i );
			glClientActiveTextureARB( GL_TEXTURE0_ARB + i );
			glEnable( GL_TEXTURE_2D );
			glEnableClientState(GL_TEXTURE_COORD_ARRAY );

			if ( i != 0 )
			 	SetTexEnv( i, MultiPass.TMU[i].PolyFlags );

			SetTexture( i, *MultiPass.TMU[i].Info, MultiPass.TMU[i].PolyFlags, MultiPass.TMU[i].PanBias );

			Color.X *= TexInfo[i].ColorRenorm.X;
			Color.Y *= TexInfo[i].ColorRenorm.Y;
			Color.Z *= TexInfo[i].ColorRenorm.Z;
		}			

		glColor4fv( &Color.X );

		INT Index = 0;
		for( FSavedPoly* Poly=MultiPass.Poly; Poly; Poly=Poly->Next )
		{
			for (i=0; i<Poly->NumPts; i++ )
			{
				FLOAT& U = MapDotArray[Index].u;
				FLOAT& V = MapDotArray[Index].v;

				for (INT t=0; t<PassCount; t++)
				{
					TexCoordArray[Index].TMU[t].u = (U-TexInfo[t].UPan)*TexInfo[t].UMult;
					TexCoordArray[Index].TMU[t].v = (V-TexInfo[t].VPan)*TexInfo[t].VMult;
				}	
				Index++;
			}			         
		}

		INT Start = 0;
		for( Poly=MultiPass.Poly; Poly; Poly=Poly->Next )
		{
			DrawArrays( GL_TRIANGLE_FAN, Start, Poly->NumPts );
			Start += Poly->NumPts;
		}

		for (i=1; i<PassCount; i++)
		{
			glActiveTextureARB( GL_TEXTURE0_ARB + i );
			glClientActiveTextureARB( GL_TEXTURE0_ARB + i );
			glDisable( GL_TEXTURE_2D );
			glDisableClientState(GL_TEXTURE_COORD_ARRAY );
		}
		glActiveTextureARB( GL_TEXTURE0_ARB );
		glClientActiveTextureARB( GL_TEXTURE0_ARB );

		//printf("PassCount == %i\n", PassCount );
		PassCount = 0;
		
		unguard;
	}

	inline void AddRenderPass( FTextureInfo* Info, DWORD PolyFlags, FLOAT PanBias, UBOOL& ForceSingle, UBOOL Masked )
	{
		MultiPass.TMU[PassCount].Multi     = PassCount;
		MultiPass.TMU[PassCount].Info      = Info;
		MultiPass.TMU[PassCount].PolyFlags = PolyFlags;
		MultiPass.TMU[PassCount].PanBias   = PanBias;	      
		
		PassCount++;
		if ( PassCount >= TMUnits || ForceSingle )
		{			
			if( ForceSingle && Masked )
				glDepthFunc( GL_EQUAL );
			RenderPasses();
			ForceSingle = true;
		}
	}

	inline void SetPolygon( FSavedPoly* Poly )
	{
		MultiPass.Poly = Poly;
	}

	void DrawComplexSurface( FSceneNode* Frame, FSurfaceInfo& Surface, FSurfaceFacet& Facet )
	{
		return;

		guard(UOpenGLRenderDevice::DrawComplexSurface);
		check(Surface.Texture);
		clock(ComplexCycles);
		FLOAT UDot = Facet.MapCoords.XAxis | Facet.MapCoords.Origin;
		FLOAT VDot = Facet.MapCoords.YAxis | Facet.MapCoords.Origin;
		INT Index  = 0;
		UBOOL ForceSingle = false;
		UBOOL Masked = Surface.PolyFlags & PF_Masked;
		UBOOL FlatShaded = (Surface.PolyFlags & PF_FlatShaded) && GIsEditor;

		EndBuffering();		// vogel: might have still been locked (can happen!)

		// Vanilla rendering.
		if ( !FlatShaded )
		{
			// Buffer "static" geometry.
			for( FSavedPoly* Poly=Facet.Polys; Poly; Poly=Poly->Next )
			{
				for( INT i=0; i<Poly->NumPts; i++ )
				{
					MapDotArray[Index].u = (Facet.MapCoords.XAxis | Poly->Pts[i]->Point) - UDot;
					MapDotArray[Index].v = (Facet.MapCoords.YAxis | Poly->Pts[i]->Point) - VDot;
					VertexArray[Index].x  = Poly->Pts[i]->Point.X;
					VertexArray[Index].y  = Poly->Pts[i]->Point.Y;
					VertexArray[Index].z  = Poly->Pts[i]->Point.Z;
					Index++;
				}
			}
			if ( UseCVA )
			{
				glDisableClientState( GL_TEXTURE_COORD_ARRAY );	
				glLockArraysEXT( 0, Index );
				glEnableClientState( GL_TEXTURE_COORD_ARRAY );
			}

			// Mutually exclusive effects.
			if( Surface.DetailTexture && Surface.FogMap )
				Surface.DetailTexture = NULL;
					
			SetPolygon( Facet.Polys );
			PassCount = 0;

			AddRenderPass( Surface.Texture, Surface.PolyFlags, 0, ForceSingle, Masked );
       
			if ( Surface.MacroTexture )
				AddRenderPass( Surface.MacroTexture, PF_Modulated, -0.5, ForceSingle, Masked );

			if ( Surface.LightMap )
				AddRenderPass( Surface.LightMap, PF_Modulated, -0.5, ForceSingle, Masked );
			
			// vogel: have to implement it in SetTexEnv
			if ( 1 || !SUPPORTS_GL_NV_texture_env_combine4 )
				RenderPasses();

			if ( Surface.FogMap )
				AddRenderPass( Surface.FogMap, PF_Highlighted, -0.5, ForceSingle, Masked );

			RenderPasses();

			if ( UseCVA )
			{
				glDisableClientState( GL_TEXTURE_COORD_ARRAY );	
				glUnlockArraysEXT();
				glEnableClientState( GL_TEXTURE_COORD_ARRAY );
			}

			if( Masked )
				glDepthFunc( GL_EQUAL );

			// Draw detail texture overlaid, in a separate pass.
			if( Surface.DetailTexture && UseDetailTextures )
			{
				FLOAT NearZ  = 380.0f;
				FLOAT RNearZ = 1.0f / NearZ;
				UBOOL IsDetailing = false;	
				Index = 0;

				for( FSavedPoly* Poly=Facet.Polys; Poly; Poly=Poly->Next )
				{
					INT Start = Index;
					UBOOL IsNear[32], CountNear=0;
					for( INT i=0; i<Poly->NumPts; i++ )
					{
						IsNear[i] = Poly->Pts[i]->Point.Z < NearZ ? 1 : 0;
						CountNear += IsNear[i];
					}
					if( CountNear )
					{
						if ( !IsDetailing )
						{
							SetBlend( PF_Modulated );						
							//glEnable( GL_POLYGON_OFFSET_FILL );
							if ( UseDetailAlpha )	
							{
								glColor4f( 0.5f, 0.5f, 0.5f, 1.0 );
								SetAlphaTexture( 0 );
								glTexEnvf( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE );

								glActiveTextureARB( GL_TEXTURE1_ARB );
								glClientActiveTextureARB( GL_TEXTURE1_ARB );
								glEnable( GL_TEXTURE_2D );						       
								glEnableClientState(GL_TEXTURE_COORD_ARRAY );

								SetTexture( 1, *Surface.DetailTexture, PF_Modulated, 0 );
								SetTexEnv( 1, PF_Memorized );
	  						}
							else
							{
								SetTexture( 0, *Surface.DetailTexture, PF_Modulated, 0 );
								SetTexEnv( 0, PF_Memorized );
								glEnableClientState( GL_COLOR_ARRAY );
							}
							IsDetailing = true;
						}
						for( INT i=0,j=Poly->NumPts-1; i<Poly->NumPts; j=i++ )
						{
							FLOAT U = Facet.MapCoords.XAxis | Poly->Pts[i]->Point;
							FLOAT V = Facet.MapCoords.YAxis | Poly->Pts[i]->Point;
							{				     	
								if ( UseDetailAlpha )	
								{	
									TexCoordArray[Index].TMU[0].u = Poly->Pts[i]->Point.Z * RNearZ;
									TexCoordArray[Index].TMU[0].v = Poly->Pts[i]->Point.Z * RNearZ;
									TexCoordArray[Index].TMU[1].u = (U-UDot-TexInfo[1].UPan)*TexInfo[1].UMult;
									TexCoordArray[Index].TMU[1].v = (V-VDot-TexInfo[1].VPan)*TexInfo[1].VMult;
								}
								else
								{
									TexCoordArray[Index].TMU[0].u = (U-UDot-TexInfo[0].UPan)*TexInfo[0].UMult;
									TexCoordArray[Index].TMU[0].v = (V-VDot-TexInfo[0].VPan)*TexInfo[0].VMult;
									// DWORD A = Min<DWORD>( appRound(100.f * (NearZ / Poly->Pts[i]->Point.Z - 1.f)), 255 );
									BYTE C = (BYTE) appRound((1 - Clamp( Poly->Pts[i]->Point.Z, 0.0f, NearZ ) / NearZ ) * 128);
									ColorArray[Index].color  = 0x00808080 | (C << 24); 
								}
								VertexArray[Index].x     = Poly->Pts[i]->Point.X;
								VertexArray[Index].y     = Poly->Pts[i]->Point.Y;
								VertexArray[Index].z     = Poly->Pts[i]->Point.Z;
								Index++;
							}
						}
						DrawArrays( GL_TRIANGLE_FAN, Start, Index - Start );
					}
				}
				if ( IsDetailing )
				{
					if ( UseDetailAlpha )
					{
						SetTexEnv( 1, PF_Modulated );
						glDisable( GL_TEXTURE_2D );
						glDisableClientState(GL_TEXTURE_COORD_ARRAY );
						glActiveTextureARB( GL_TEXTURE0_ARB );
						glClientActiveTextureARB( GL_TEXTURE0_ARB );
						glTexEnvf( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE );
					}
					else
					{
						glDisableClientState(GL_COLOR_ARRAY );
					}
					SetTexEnv( 0, PF_Modulated );
					//glDisable(GL_POLYGON_OFFSET_FILL);
				}
			}
		}
		// UnrealEd flat shading.
		else
		{
			if ( (Surface.PolyFlags & PF_FlatShaded) && GIsEditor  )
			{
				SetNoTexture( 0 );
				SetBlend( PF_FlatShaded );
				glColor3ubv( (BYTE*)&Surface.FlatColor );
				for( FSavedPoly* Poly=Facet.Polys; Poly; Poly=Poly->Next )
				{
					glBegin( GL_TRIANGLE_FAN );
					for( INT i=0; i<Poly->NumPts; i++ )
						glVertex3fv( &Poly->Pts[i]->Point.X );
					glEnd();
				}	
			}
		}

		// UnrealEd selection.
		if( (Surface.PolyFlags & PF_Selected) && GIsEditor )
		{
			SetNoTexture( 0 );
			SetBlend( PF_Highlighted );
			glColor4f( 0, 0, 0.5, 0.5 );
			for( FSavedPoly* Poly=Facet.Polys; Poly; Poly=Poly->Next )
			{
				glBegin( GL_TRIANGLE_FAN );
				for( INT i=0; i<Poly->NumPts; i++ )
					glVertex3fv( &Poly->Pts[i]->Point.X );
				glEnd();
			}
		}

		if( Masked && !FlatShaded )
			glDepthFunc( GL_LEQUAL );

		unclock(ComplexCycles);
		unguard;
	}
	
	void DrawGouraudPolygonOld( FSceneNode* Frame, FTextureInfo& Info, FTransTexture** Pts, INT NumPts, DWORD PolyFlags, FSpanBuffer* Span )
	{
		guard(UOpenGLRenderDevice::DrawGouraudPolygonOld);      
		clock(GouraudCycles);
	    
		INT Index     = 0;
		UBOOL Enabled = false;

		SetBlend( PolyFlags );
		SetTexture( 0, Info, PolyFlags, 0 );
		if( PolyFlags & PF_Modulated )
			glColor4f( TexInfo[0].ColorNorm.X, TexInfo[0].ColorNorm.Y, TexInfo[0].ColorNorm.Z, 1 );
		else
		{
			glEnableClientState( GL_COLOR_ARRAY );
			Enabled = true;
		}
		
		for( INT i=0; i<NumPts; i++ )
		{
			FTransTexture* P = Pts[i];
			TexCoordArray[Index].TMU[0].u = P->U*TexInfo[0].UMult;
			TexCoordArray[Index].TMU[0].v = P->V*TexInfo[0].VMult;
			if( Enabled )
				ColorArray[Index].color = RGBA_MAKE(
					appRound( 255 * P->Light.X*TexInfo[0].ColorNorm.X ), 
					appRound( 255 * P->Light.Y*TexInfo[0].ColorNorm.Y ), 
					appRound( 255 * P->Light.Z*TexInfo[0].ColorNorm.Z ),
					255);
			VertexArray[Index].x  = P->Point.X;
			VertexArray[Index].y  = P->Point.Y;
			VertexArray[Index].z  = P->Point.Z;
			Index++;
		}

		DrawArrays( GL_TRIANGLE_FAN, 0, Index );

		if( (PolyFlags & (PF_RenderFog|PF_Translucent|PF_Modulated))==PF_RenderFog )
		{	
			Index = 0;
			if ( !Enabled )
			{
				glEnableClientState( GL_COLOR_ARRAY );
				Enabled = true;	
			}	 		
			SetNoTexture( 0 );
			SetBlend( PF_Highlighted );
			for( INT i=0; i<NumPts; i++ )
			{
				FTransTexture* P = Pts[i];
		  		ColorArray[Index].color = RGBA_MAKE(
					appRound( 255 * P->Fog.X ), 
					appRound( 255 * P->Fog.Y ),
					appRound( 255 * P->Fog.Z ),
					appRound( 255 * P->Fog.W ));			
				Index++;
			}

			DrawArrays( GL_TRIANGLE_FAN, 0, Index );

		}
		
		if( Enabled )
			glDisableClientState( GL_COLOR_ARRAY );

		unclock(GouraudCycles);
		unguard;
	}

	void EndBuffering()
	{
		if ( BufferedVerts > 0 )
		{
			if ( RenderFog )
			{
				glEnableClientState( GL_SECONDARY_COLOR_ARRAY_EXT );
				glEnable( GL_COLOR_SUM_EXT );	
			}

			// Actually render the triangles.
			DrawArrays( GL_TRIANGLES, 0, BufferedVerts );

			if ( ColorArrayEnabled )
				glDisableClientState( GL_COLOR_ARRAY );
			if ( RenderFog )
			{
				glDisableClientState( GL_SECONDARY_COLOR_ARRAY_EXT );
				glDisable( GL_COLOR_SUM_EXT );
			}
			BufferedVerts = 0;
		}
	}

	void DrawGouraudPolygon( FSceneNode* Frame, FTextureInfo& Info, FTransTexture** Pts, INT NumPts, DWORD PolyFlags, FSpanBuffer* Span )
	{
		return;

		guard(UOpenGLRenderDevice::DrawGouraudPolygon);      
		if ( !BufferActorTris || NumPts != 3)
		{
			DrawGouraudPolygonOld( Frame, Info, Pts, NumPts, PolyFlags, Span );
			return;
		}
		clock(GouraudCycles);

		static DWORD lastPolyFlags = 0; // vogel: can't use CurrentFlags as SetBlend modifies it (especially PF_RenderFog)

		if ( ! ((lastPolyFlags==PolyFlags) && (TexInfo[0].CurrentCacheID == Info.CacheID) &&
			(BufferedVerts+NumPts < VERTEX_ARRAY_SIZE-1) && (BufferedVerts>0) ) )
		// flush drawing and set the state!
		{
			SetBlend( PolyFlags ); // SetBlend will call EndBuffering() to flush the vertex array
			SetTexture( 0, Info, PolyFlags, 0 );
			if( PolyFlags & PF_Modulated )
			{
				glColor4f( TexInfo[0].ColorNorm.X, TexInfo[0].ColorNorm.Y, TexInfo[0].ColorNorm.Z, 1 );
				ColorArrayEnabled = false;
			}
			else
			{
				glEnableClientState( GL_COLOR_ARRAY );
				ColorArrayEnabled = true;
			}
			if( ((PolyFlags & (PF_RenderFog|PF_Translucent|PF_Modulated))==PF_RenderFog) && UseVertexSpecular )
				RenderFog = true;
			else	
				RenderFog = false;

			lastPolyFlags = PolyFlags;
		}

		for( INT i=0; i<NumPts; i++ )
		{
			FTransTexture* P = Pts[i];
			TexCoordArray[BufferedVerts].TMU[0].u = P->U*TexInfo[0].UMult;
			TexCoordArray[BufferedVerts].TMU[0].v = P->V*TexInfo[0].VMult;
			if ( RenderFog  && ColorArrayEnabled )
			{
				ColorArray[BufferedVerts].color = RGBA_MAKE(
					appRound( 255 * P->Light.X*TexInfo[0].ColorNorm.X * (1 - P->Fog.W)), 
					appRound( 255 * P->Light.Y*TexInfo[0].ColorNorm.Y * (1 - P->Fog.W) ), 
					appRound( 255 * P->Light.Z*TexInfo[0].ColorNorm.Z * (1 - P->Fog.W) ),
					255);

				ColorArray[BufferedVerts].specular = RGBA_MAKE(
					appRound( 255 * P->Fog.X ), 
					appRound( 255 * P->Fog.Y ),
					appRound( 255 * P->Fog.Z ),
					0 );	
			}
			else if ( RenderFog )
			{
				ColorArray[BufferedVerts].color = RGBA_MAKE(
					appRound( 255 * TexInfo[0].ColorNorm.X * (1 - P->Fog.W)), 
					appRound( 255 * TexInfo[0].ColorNorm.Y * (1 - P->Fog.W) ), 
					appRound( 255 * TexInfo[0].ColorNorm.Z * (1 - P->Fog.W) ),
					255);

				ColorArray[BufferedVerts].specular = RGBA_MAKE(
					appRound( 255 * P->Fog.X ), 
					appRound( 255 * P->Fog.Y ),
					appRound( 255 * P->Fog.Z ),
					0 );
				ColorArrayEnabled = true;
				glEnableClientState( GL_COLOR_ARRAY );
			}
			else if ( ColorArrayEnabled )
			{
				ColorArray[BufferedVerts].color = RGBA_MAKE(
					appRound( 255 * P->Light.X*TexInfo[0].ColorNorm.X ), 
					appRound( 255 * P->Light.Y*TexInfo[0].ColorNorm.Y ), 
					appRound( 255 * P->Light.Z*TexInfo[0].ColorNorm.Z ),
					255);
			}				
			VertexArray[BufferedVerts].x  = P->Point.X;
			VertexArray[BufferedVerts].y  = P->Point.Y;
			VertexArray[BufferedVerts].z  = P->Point.Z;
			BufferedVerts++;
		}
		unclock(GouraudCycles);
		unguard;
	}
	
	void DrawTile( FSceneNode* Frame, FTextureInfo& Info, FLOAT X, FLOAT Y, FLOAT XL, FLOAT YL, FLOAT U, FLOAT V, FLOAT UL, FLOAT VL, class FSpanBuffer* Span, FLOAT Z, FPlane Color, FPlane Fog, DWORD PolyFlags )
	{
		guard(UOpenGLRenderDevice::DrawTile);
		//clock(TileCycles);

		PolyFlags = 0;

		//if( Info.Palette && Info.Palette[128].A!=255 && !(PolyFlags&PF_Translucent) )
			//PolyFlags |= PF_Highlighted;

		SetBlend( PolyFlags );
		SetTexture( 0, Info, PolyFlags, 0 );

		Color.X *= TexInfo[0].ColorNorm.X;
		Color.Y *= TexInfo[0].ColorNorm.Y;
		Color.Z *= TexInfo[0].ColorNorm.Z;
		Color.W  = 1;	

		//if ( PolyFlags & PF_Modulated )
			//glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );
		//else
			//glColor4fv( &Color.X );
		//glColor3fv( &Color.X );

		glColor4f( 1.0f, 0.25f, 0.25f, 0.5f );

#if 1
		//if( Frame->Viewport->IsOrtho() )
		{
			glBegin( GL_TRIANGLE_FAN );
			glTexCoord2f( (U   )*TexInfo[0].UMult, (V   )*TexInfo[0].VMult ); glVertex3f( RFX2*(X   -Frame->FX2), RFY2*(Y   -Frame->FY2), 1.0 );
			glTexCoord2f( (U+UL)*TexInfo[0].UMult, (V   )*TexInfo[0].VMult ); glVertex3f( RFX2*(X+XL-Frame->FX2), RFY2*(Y   -Frame->FY2), 1.0 );
			glTexCoord2f( (U+UL)*TexInfo[0].UMult, (V+VL)*TexInfo[0].VMult ); glVertex3f( RFX2*(X+XL-Frame->FX2), RFY2*(Y+YL-Frame->FY2), 1.0 );
			glTexCoord2f( (U   )*TexInfo[0].UMult, (V+VL)*TexInfo[0].VMult ); glVertex3f( RFX2*(X   -Frame->FX2), RFY2*(Y+YL-Frame->FY2), 1.0 );
			glEnd();
		}
		//else
		{
			glBegin( GL_TRIANGLE_FAN );
			glTexCoord2f( (U   )*TexInfo[0].UMult, (V   )*TexInfo[0].VMult ); glVertex3f( RFX2*Z*(X   -Frame->FX2), RFY2*Z*(Y   -Frame->FY2), Z );
			glTexCoord2f( (U+UL)*TexInfo[0].UMult, (V   )*TexInfo[0].VMult ); glVertex3f( RFX2*Z*(X+XL-Frame->FX2), RFY2*Z*(Y   -Frame->FY2), Z );
			glTexCoord2f( (U+UL)*TexInfo[0].UMult, (V+VL)*TexInfo[0].VMult ); glVertex3f( RFX2*Z*(X+XL-Frame->FX2), RFY2*Z*(Y+YL-Frame->FY2), Z );
			glTexCoord2f( (U   )*TexInfo[0].UMult, (V+VL)*TexInfo[0].VMult ); glVertex3f( RFX2*Z*(X   -Frame->FX2), RFY2*Z*(Y+YL-Frame->FY2), Z );
			glEnd();
		}
#else
		glBegin( GL_TRIANGLE_FAN );
		glTexCoord2f( (U   )*TexInfo[0].UMult, (V   )*TexInfo[0].VMult ); glVertex3f( RFX2*Z*(X   -Frame->FX2), RFY2*Z*(Y   -Frame->FY2), Z );
		glTexCoord2f( (U+UL)*TexInfo[0].UMult, (V   )*TexInfo[0].VMult ); glVertex3f( RFX2*Z*(X+XL-Frame->FX2), RFY2*Z*(Y   -Frame->FY2), Z );
		glTexCoord2f( (U+UL)*TexInfo[0].UMult, (V+VL)*TexInfo[0].VMult ); glVertex3f( RFX2*Z*(X+XL-Frame->FX2), RFY2*Z*(Y+YL-Frame->FY2), Z );
		glTexCoord2f( (U   )*TexInfo[0].UMult, (V+VL)*TexInfo[0].VMult ); glVertex3f( RFX2*Z*(X   -Frame->FX2), RFY2*Z*(Y+YL-Frame->FY2), Z );
		glEnd();
#endif
		//unclock(TileCycles);
		unguard;
	}
	
	void Draw2DLine( FSceneNode* Frame, FPlane Color, DWORD LineFlags, FVector P1, FVector P2 )
	{
		return;

		guard(UOpenGLRenderDevice::Draw2DLine);

		// kaufel: always clear z buffer for editor in non ortho view.
		if ( GIsEditor && !Frame->Viewport->IsOrtho() )
			ClearZ( Frame );

		SetNoTexture( 0 );
		SetBlend( PF_Highlighted );
		glColor3fv( &Color.X );
		glBegin( GL_LINES );
		glVertex3f( RFX2*P1.Z*(P1.X-Frame->FX2), RFY2*P1.Z*(P1.Y-Frame->FY2), P1.Z );
		glVertex3f( RFX2*P2.Z*(P2.X-Frame->FX2), RFY2*P2.Z*(P2.Y-Frame->FY2), P2.Z );
		glEnd();
		unguard;
	}
	
	void Draw3DLine( FSceneNode* Frame, FPlane Color, DWORD LineFlags, FVector P1, FVector P2 )
	{
		return;

		guard(UOpenGLRenderDevice::Draw3DLine);

		P1 = P1.TransformPointBy( Frame->Coords );
		P2 = P2.TransformPointBy( Frame->Coords );

		if( Frame->Viewport->IsOrtho() )
		{
			// Zoom.
			P1.X = (P1.X) / Frame->Zoom + Frame->FX2;
			P1.Y = (P1.Y) / Frame->Zoom + Frame->FY2;
			P2.X = (P2.X) / Frame->Zoom + Frame->FX2;
			P2.Y = (P2.Y) / Frame->Zoom + Frame->FY2;
			P1.Z = P2.Z = 1;

			// See if points form a line parallel to our line of sight (i.e. line appears as a dot).
			if( Abs(P2.X-P1.X)+Abs(P2.Y-P1.Y)>=0.2 )
				Draw2DLine( Frame, Color, LineFlags, P1, P2 );
			else if( Frame->Viewport->Actor->OrthoZoom < ORTHO_LOW_DETAIL )
				Draw2DPoint( Frame, Color, LINE_None, P1.X-1, P1.Y-1, P1.X+1, P1.Y+1, P1.Z );
		}
		else
		{
			// kaufel: always clear z buffer for editor in non ortho view.
			if ( GIsEditor )
				ClearZ( Frame );

			SetNoTexture( 0 );
			SetBlend( PF_Highlighted );
			glColor3fv( &Color.X );
			glBegin( GL_LINES );
			glVertex3fv( &P1.X );
			glVertex3fv( &P2.X );
			glEnd();
		}
		unguard;
	}
	
	void Draw2DPoint( FSceneNode* Frame, FPlane Color, DWORD LineFlags, FLOAT X1, FLOAT Y1, FLOAT X2, FLOAT Y2, FLOAT Z )
	{
		return;

		guard(UOpenGLRenderDevice::Draw2DPoint);
		SetBlend( PF_Highlighted );
		SetNoTexture( 0 );
		glColor4fv( &Color.X );
		//glColor3fv( &Color.X ); // vogel: was 4 - ONLY FOR UT!
#if 1
		if( Frame->Viewport->IsOrtho() )
		{
			glBegin( GL_TRIANGLE_FAN );
			glVertex3f( RFX2*(X1-Frame->FX2-0.5), RFY2*(Y1-Frame->FY2-0.5), 1.0 );
			glVertex3f( RFX2*(X2-Frame->FX2+0.5), RFY2*(Y1-Frame->FY2-0.5), 1.0 );
			glVertex3f( RFX2*(X2-Frame->FX2+0.5), RFY2*(Y2-Frame->FY2+0.5), 1.0 );
			glVertex3f( RFX2*(X1-Frame->FX2-0.5), RFY2*(Y2-Frame->FY2+0.5), 1.0 );
			glEnd();
		}
		else if ( GIsEditor )
		{
			// kaufel: always clear z buffer for editor in non ortho view.
			ClearZ( Frame );

			if ( Z < 0.0 )
				Z = -Z;

			glBegin( GL_TRIANGLE_FAN );
			glVertex3f( RFX2*Z*(X1-Frame->FX2-0.5), RFY2*Z*(Y1-Frame->FY2-0.5), Z );
			glVertex3f( RFX2*Z*(X2-Frame->FX2+0.5), RFY2*Z*(Y1-Frame->FY2-0.5), Z );
			glVertex3f( RFX2*Z*(X2-Frame->FX2+0.5), RFY2*Z*(Y2-Frame->FY2+0.5), Z );
			glVertex3f( RFX2*Z*(X1-Frame->FX2-0.5), RFY2*Z*(Y2-Frame->FY2+0.5), Z );
			glEnd();
		}
		else
		{
			glBegin( GL_TRIANGLE_FAN );
			glVertex3f( RFX2*Z*(X1-Frame->FX2), RFY2*Z*(Y1-Frame->FY2), 0.5 );
			glVertex3f( RFX2*Z*(X2-Frame->FX2), RFY2*Z*(Y1-Frame->FY2), 0.5 );
			glVertex3f( RFX2*Z*(X2-Frame->FX2), RFY2*Z*(Y2-Frame->FY2), 0.5 );
			glVertex3f( RFX2*Z*(X1-Frame->FX2), RFY2*Z*(Y2-Frame->FY2), 0.5 );
			glEnd();		
		}
#else
		glBegin( GL_TRIANGLE_FAN );
		glVertex3f( RFX2*Z*(X1-Frame->FX2), RFY2*Z*(Y1-Frame->FY2), 0.5 );
		glVertex3f( RFX2*Z*(X2-Frame->FX2), RFY2*Z*(Y1-Frame->FY2), 0.5 );
		glVertex3f( RFX2*Z*(X2-Frame->FX2), RFY2*Z*(Y2-Frame->FY2), 0.5 );
		glVertex3f( RFX2*Z*(X1-Frame->FX2), RFY2*Z*(Y2-Frame->FY2), 0.5 );
		glEnd();
#endif
		unguard;
	}
	
	void ClearZ( FSceneNode* Frame )
	{
		guard(UOpenGLRenderDevice::ClearZ);
		SetBlend( PF_Occlude );
		glClear( GL_DEPTH_BUFFER_BIT );
		unguard;
	}
	
	void PushHit( const BYTE* Data, INT Count )
	{
		guard(UOpenGLRenderDevice::PushHit);
		glPushName( Count );
		for( INT i=0; i<Count; i+=4 )
			glPushName( *(INT*)(Data+i) );
		unguard;
	}
	
	void PopHit( INT Count, UBOOL bForce )
	{
		guard(UOpenGLRenderDevice::PopHit);
		glPopName();
		for( INT i=0; i<Count; i+=4 )
			glPopName();
		//!!implement bforce
		unguard;
	}
	
	void GetStats( TCHAR* Result )
	{
		guard(UOpenGLRenderDevice::GetStats);
		appSprintf
		(
			Result,
			TEXT("OpenGL stats: Bind=%04.1f Image=%04.1f Complex=%04.1f Gouraud=%04.1f Tile=%04.1f"),
			GSecondsPerCycle*1000 * BindCycles,
			GSecondsPerCycle*1000 * ImageCycles,
			GSecondsPerCycle*1000 * ComplexCycles,
			GSecondsPerCycle*1000 * GouraudCycles,
			GSecondsPerCycle*1000 * TileCycles
		);
		unguard;
	}
	
	void ReadPixels( FColor* Pixels )
	{
		guard(UOpenGLRenderDevice::ReadPixels);
		glReadPixels( 0, 0, Viewport->SizeX, Viewport->SizeY, GL_RGBA, GL_UNSIGNED_BYTE, Pixels );
		for( INT i=0; i<Viewport->SizeY/2; i++ )
		{
			for( INT j=0; j<Viewport->SizeX; j++ )
			{
				Exchange( Pixels[j+i*Viewport->SizeX].R, Pixels[j+(Viewport->SizeY-1-i)*Viewport->SizeX].B );
				Exchange( Pixels[j+i*Viewport->SizeX].G, Pixels[j+(Viewport->SizeY-1-i)*Viewport->SizeX].G );
				Exchange( Pixels[j+i*Viewport->SizeX].B, Pixels[j+(Viewport->SizeY-1-i)*Viewport->SizeX].R );
			}
		}
  	unguard;
	}
	
	void EndFlash()
	{
		return;

		if( FlashScale!=FPlane(.5,.5,.5,0) || FlashFog!=FPlane(0,0,0,0) )
		{
			SetBlend( PF_Highlighted );
			SetNoTexture( 0 );
			glColor4f( FlashFog.X, FlashFog.Y, FlashFog.Z, 1.0-Min(FlashScale.X*2.f,1.f) );
			FLOAT RFX2 = 2.0*RProjZ       /Viewport->SizeX;
			FLOAT RFY2 = 2.0*RProjZ*Aspect/Viewport->SizeY;
			glBegin( GL_TRIANGLE_FAN );
				glVertex3f( RFX2*(-Viewport->SizeX/2.0), RFY2*(-Viewport->SizeY/2.0), 1.0 );
				glVertex3f( RFX2*(+Viewport->SizeX/2.0), RFY2*(-Viewport->SizeY/2.0), 1.0 );
				glVertex3f( RFX2*(+Viewport->SizeX/2.0), RFY2*(+Viewport->SizeY/2.0), 1.0 );
				glVertex3f( RFX2*(-Viewport->SizeX/2.0), RFY2*(+Viewport->SizeY/2.0), 1.0 );
			glEnd();
		}
	}
	
	void PrecacheTexture( FTextureInfo& Info, DWORD PolyFlags )
	{
		guard(UOpenGLRenderDevice::PrecacheTexture);
		SetTexture( 0, Info, PolyFlags, 0 );
		unguard;
	}
};
IMPLEMENT_CLASS(UOpenGLRenderDevice);

// Static variables.
INT		UOpenGLRenderDevice::NumDevices    = 0;
INT		UOpenGLRenderDevice::LockCount     = 0;
#ifdef WIN32
HGLRC		UOpenGLRenderDevice::hCurrentRC    = NULL;
HMODULE		UOpenGLRenderDevice::hModuleGlMain = NULL;
HMODULE		UOpenGLRenderDevice::hModuleGlGdi  = NULL;
TArray<HGLRC>	UOpenGLRenderDevice::AllContexts;
#else
UBOOL 		UOpenGLRenderDevice::GLLoaded	   = false;
#endif
TMap<QWORD,UOpenGLRenderDevice::FCachedTexture> UOpenGLRenderDevice::SharedBindMap;

// OpenGL function pointers.
#define GL_EXT(name) UBOOL UOpenGLRenderDevice::SUPPORTS##name=0;
#define GL_PROC(ext,ret,func,parms) ret (STDCALL *UOpenGLRenderDevice::func)parms;
#include "OpenGLFuncs.h"
#undef GL_EXT
#undef GL_PROC

/*-----------------------------------------------------------------------------
	The End.
-----------------------------------------------------------------------------*/

