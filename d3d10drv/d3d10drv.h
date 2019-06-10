/**
\file d3d10drv.h
*/

#pragma once
#include "Engine.h"
#include "UnRender.h"
#include "d3d.h"

#define SAFE_RELEASE(p)      { if(p) { (p)->Release(); (p)=NULL; } }
#define CLAMP(p,min,max)	{ if(p < min) p = min; else if (p>max) p = max; }


class DLL_EXPORT UD3D10RenderDevice:public URenderDevice
{

DECLARE_CLASS(UD3D10RenderDevice,URenderDevice,CLASS_Config)

private:
	D3D::Options D3DOptions;
	/** User configurable options */
	struct
	{
		int precache; /**< Turn on precaching */
		int autoFOV; /**< Turn on auto field of view setting */
		int FPSLimit; /**< 60FPS frame limiter */
		int unlimitedViewDistance; /**< Set frustum to max map size */
	} options;

	//Idk
	UBOOL PrecacheOnFlip;


public:
	/**@name Helpers */
	//@{	
	static void debugs(TCHAR*s);
	static int getOption(TCHAR* name, int defaultVal, bool isBool);
	static int getOption(UClass* Class, TCHAR* name,int defaultVal, bool isBool);
	//@}
	
	/**@name Abstract in parent class */
	//@{	
	UBOOL Init(UViewport *InViewport,INT NewX, INT NewY, INT NewColorBytes, UBOOL Fullscreen);
	UBOOL SetRes(INT NewX, INT NewY, INT NewColorBytes, UBOOL Fullscreen);
	//UBOOL UnSetRes(const TCHAR* Msg, HRESULT h);
	void ShutdownAfterError();
	void Exit();

	void Flush();
	void Flush(UBOOL AllowPrecache);
	void Lock(FPlane FlashScale, FPlane FlashFog, FPlane ScreenClear, DWORD RenderLockFlags, BYTE* HitData, INT* HitSize );
	void Unlock(UBOOL Blit );
	void DrawComplexSurface(FSceneNode* Frame, FSurfaceInfo& Surface, FSurfaceFacet& Facet );
	void DrawGouraudPolygon( FSceneNode* Frame, FTextureInfo& Info, FTransTexture** Pts, int NumPts, DWORD PolyFlags, FSpanBuffer* Span );
	void DrawTile( FSceneNode* Frame, FTextureInfo& Info, FLOAT X, FLOAT Y, FLOAT XL, FLOAT YL, FLOAT U, FLOAT V, FLOAT UL, FLOAT VL, class FSpanBuffer* Span, FLOAT Z, FPlane Color, FPlane Fog, DWORD PolyFlags );
	void Draw2DLine( FSceneNode* Frame, FPlane Color, DWORD LineFlags, FVector P1, FVector P2 );
	void Draw2DPoint( FSceneNode* Frame, FPlane Color, DWORD LineFlags, FLOAT X1, FLOAT Y1, FLOAT X2, FLOAT Y2, FLOAT Z );
	void ClearZ( FSceneNode* Frame );
	void PushHit( const BYTE* Data, INT Count );
	void PopHit( INT Count, UBOOL bForce );
	void GetStats( TCHAR* Result );
	void ReadPixels( FColor* Pixels );
	//@}


	/**@name Optional but implemented*/
	//@{
	UBOOL Exec(const TCHAR* Cmd, FOutputDevice& Ar);
	void SetSceneNode( FSceneNode* Frame );
	void PrecacheTexture( FTextureInfo& Info, DWORD PolyFlags );
	void EndFlash();
	static void StaticConstructor(UClass* Class);
	//@}
};