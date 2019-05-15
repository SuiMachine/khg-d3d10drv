/**
\class UD3D10RenderDevice

This is the interface between the game and the graphics API.
For the D3D10 renderer, an effort was made to have it not work directly with D3D types and objects; it is purely concerned with answering the game and putting
data in correct structures for further processing. This leaves this class relatively clean and easy to understand, and should make it a good basis for further work.
It contains only the bare essential functions to implement the renderer interface.
There are two exceptions: UD3D10RenderDevice::debugs() and UD3D10RenderDevice::getOption are helpers not required by the game.

Called UD3D10RenderDevice as Unreal leaves out first letter when accessing the class; now it can be accessed as D3D10RenderDevice.
*/

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include <io.h>
#include <FCNTL.H>

#include <new>

#include "resource.h"
#include "d3d10drv.h"
#include "texconverter.h"
#include "customflags.h"
#include "misc.h"
#include "vertexformats.h"
#include "shader_gouraudpolygon.h"
#include "shader_tile.h"
#include "shader_complexsurface.h"
#include "shader_fogsurface.h"
#include <iostream>

//UObject glue
IMPLEMENT_PACKAGE(D3D10Drv);

IMPLEMENT_CLASS(UD3D10RenderDevice);


static bool drawingWeapon; /** Whether the depth buffer was cleared and projection parameters set to draw the weapon model */
static bool drawingHUD;
static int customFOV; /**Field of view calculated from aspect ratio */
static bool firstFrameOfLevel;
/** See SetSceneNode() */
static float zNear = 0.5f; //Default for the games is 1, but results in cut-off UT weapons with widescreen FOVs
static float zFar;
static LARGE_INTEGER perfCounterFreq;
static TextureCache *textureCache;
static TexConverter *texConverter;
static Shader_GouraudPolygon *shader_GouraudPolygon;
static Shader_Tile *shader_Tile;
static Shader_ComplexSurface *shader_ComplexSurface;
static Shader_FogSurface *shader_FogSurface;

/**
Prints text to the game's log and the standard output if in debug mode.
\param s A the message to print.
\note Does not take a wide character string because not everything we want to print might be available as such (i.e. ID3D10Blobs).
*/
void UD3D10RenderDevice::debugs(char* s)
{ 
	char buf[255];
	size_t n;
	//mbstowcs_s(&n,buf,255,s,254);	
	//GLog->Log(buf);
	#ifdef _DEBUG //In debug mode, print output to console
	puts(s);
	#endif
}

int UD3D10RenderDevice::getOption(UClass* Class, TCHAR* name, int defaultVal, bool isBool)
{
	return 0;
}

/**
Attempts to read a property from the game's config file; on failure, a default is written (so it can be changed by the user) and returned.
\param name A string identifying the config file options.
\param defaultVal The default value to write and return if the option is not found.
\param isBool Whether the parameter's a boolean or integer
\return The value for the property.
\note The default value is written so it can be user modified (either from the config or preferences window) from then on.
*/
int UD3D10RenderDevice::getOption(char* name,int defaultVal, bool isBool)
{
	char* Section = "D3D10Drv.D3D10RenderDevice";
	int out;
	if(isBool)
	{
		if(!GetConfigBool(Section, name, (INT&)out))
		{
			SetConfigBool(Section, name, defaultVal);
			out = defaultVal;
		}
	}
	else
	{
		if(!GetConfigInt(Section, name, (INT&)out))
		{
			SetConfigInt(Section, name, defaultVal);
			out = defaultVal;
		}
	}
	return out;
}

/**
Constructor called by the game when the renderer is first created.
\note Required to compile for Unreal Tournament. 
\note Binding settings to the preferences window needs to done here instead of in init() or the game crashes when starting a map if the renderer's been restarted at least once.
\KHG: Static Constructor is different in KHG.
*/
void UD3D10RenderDevice::StaticConstructor(UClass* Class)
{
	//Make the property appear in the preferences window; this will automatically pick up the current value and write back changes.	
	new(Class, "Precache", RF_Public) UBoolProperty(CPP_PROPERTY(options.precache), TEXT("Options"), CPF_Config);
	new(Class, "Antialiasing", RF_Public) UIntProperty(CPP_PROPERTY(D3DOptions.samples), TEXT("Options"), CPF_Config);
	new(Class, "Anisotropy", RF_Public) UIntProperty(CPP_PROPERTY(D3DOptions.aniso), TEXT("Options"), CPF_Config);
	new(Class, "VSync", RF_Public) UBoolProperty(CPP_PROPERTY(D3DOptions.VSync), TEXT("Options"), CPF_Config);
	new(Class, "ParallaxOcclusionMapping", RF_Public) UBoolProperty(CPP_PROPERTY(D3DOptions.POM), TEXT("Options"), CPF_Config);
	new(Class, "LODBias", RF_Public) UIntProperty(CPP_PROPERTY(D3DOptions.LODBias), TEXT("Options"), CPF_Config);
	new(Class, "AlphaToCoverage", RF_Public) UBoolProperty(CPP_PROPERTY(D3DOptions.alphaToCoverage), TEXT("Options"), CPF_Config);
	new(Class, "BumpMapping", RF_Public) UBoolProperty(CPP_PROPERTY(D3DOptions.bumpMapping), TEXT("Options"), CPF_Config);
	new(Class, "ClassicLighting", RF_Public) UBoolProperty(CPP_PROPERTY(D3DOptions.classicLighting), TEXT("Options"), CPF_Config);
	new(Class, "AutoFOV", RF_Public) UBoolProperty(CPP_PROPERTY(options.autoFOV), TEXT("Options"), CPF_Config);
	new(Class, "FPSLimit", RF_Public) UIntProperty(CPP_PROPERTY(options.FPSLimit), TEXT("Options"), CPF_Config);
	new(Class, "SimulateMultiPassTexturing", RF_Public) UBoolProperty(CPP_PROPERTY(D3DOptions.simulateMultipassTexturing), TEXT("Options"), CPF_Config);
	new(Class, "UnlimitedViewDistance", RF_Public) UBoolProperty(CPP_PROPERTY(options.unlimitedViewDistance), TEXT("Options"), CPF_Config);

	//Turn on parent class options by default. If done here (instead of in Init()), the ingame preferences still work
	getOption("Coronas", 1, true);
	getOption("HighDetailActors", 1, true);
	getOption("VolumetricLighting", 1, true);
	getOption("ShinySurfaces", 1, true);
	getOption("DetailTextures", 1, true);

	//Create a console to print debug stuff to.
#ifdef _KHGDEBUG
	AllocConsole();

	// Get STDOUT handle
	HANDLE ConsoleOutput = GetStdHandle(STD_OUTPUT_HANDLE);
	int SystemOutput = _open_osfhandle(intptr_t(ConsoleOutput), _O_TEXT);
	FILE* COutputHandle = _fdopen(SystemOutput, "w");

	// Get STDERR handle
	HANDLE ConsoleError = GetStdHandle(STD_ERROR_HANDLE);
	int SystemError = _open_osfhandle(intptr_t(ConsoleError), _O_TEXT);
	FILE* CErrorHandle = _fdopen(SystemError, "w");

	// Get STDIN handle
	HANDLE ConsoleInput = GetStdHandle(STD_INPUT_HANDLE);
	int SystemInput = _open_osfhandle(intptr_t(ConsoleInput), _O_TEXT);
	FILE* CInputHandle = _fdopen(SystemInput, "r");

	// Redirect the CRT standard input, output, and error handles to the console
	freopen_s(&CInputHandle, "CONIN$", "r", stdin);
	freopen_s(&COutputHandle, "CONOUT$", "w", stdout);
	freopen_s(&CErrorHandle, "CONOUT$", "w", stderr);

	std::wcout.clear();
	std::cout.clear();
	std::wcerr.clear();
	std::cerr.clear();
	std::wcin.clear();
	std::cin.clear();
#endif
}



/**
Initialization of renderer.
- Set parent class options. Some of these are settings for the renderer to heed, others control what the game does.
	- URenderDevice::SpanBased; Probably for software renderers.
	- URenderDevice::Fullscreen; Only for Voodoo cards.
	- URenderDevice::SupportsTC; Game sends compressed textures if present.
	- URenderDevice::SupportsDistanceFog; Distance fog. Don't know how this is supposed to be implemented.
	- URenderDevice::SupportsLazyTextures; Renderer loads and unloads texture info when needed (???).
	- URenderDevice::PrefersDeferredLoad; Renderer prefers not to cache textures in advance (???).
	- URenderDevice::ShinySurfaces; Renderer supports detail textures. The game sends them always, so it's meant as a detail setting for the renderer.
	- URenderDevice::Coronas; If enabled, the game draws light coronas.
	- URenderDevice::HighDetailActors; If enabled, game sends more detailed models (???).
	- URenderDevice::VolumetricLighting; If enabled, the game sets fog textures for surfaces if needed.
	- URenderDevice::PrecacheOnFlip; The game will call the PrecacheTexture() function to load textures in advance. Also see Flush().
	- URenderDevice::Viewport; Always set to InViewport.
- Initialize graphics api.
- Resize buffers (convenient to use SetRes() for this).

\param InViewport viewport parameters, can get the window handle.
\param NewX Viewport width.
\param NewY Viewport height.
\param NewColorBytes Color depth.
\param Fullscreen Whether fullscreen mode should be used.
\return 1 if init succesful. On 0, game errors out.

\note D3D10 renderer ignores color depth.
*/
UBOOL UD3D10RenderDevice::Init(UViewport *InViewport,INT NewX, INT NewY, INT NewColorBytes, UBOOL Fullscreen)
{
	UD3D10RenderDevice::debugs("Initializing Direct3D 10 renderer.");
	
	//Set parent class params
	URenderDevice::SpanBased = 0;
	URenderDevice::FullscreenOnly = 0;
	URenderDevice::SupportsFogMaps = 0;
	URenderDevice::HighDetailActors = 1;
	URenderDevice::ShinySurfaces = 1;
	URenderDevice::VolumetricLighting = 1;
	//URenderDevice::SupportsTC = 1;
	URenderDevice::SupportsDistanceFog = 0;
	//URenderDevice::SupportsLazyTextures = 0;


	

	//Get/set config options.
	options.precache = getOption("Precache",0,true);
	D3DOptions.samples = getOption("Antialiasing",4,false);
	D3DOptions.aniso = getOption("Anisotropy",8,false);
	D3DOptions.VSync = getOption("VSync",1,true);	
	D3DOptions.POM = getOption("ParallaxOcclusionMapping",0,true);	
	D3DOptions.LODBias = getOption("LODBias",0,false);
	D3DOptions.bumpMapping = getOption("BumpMapping",0,true);	
	D3DOptions.classicLighting = getOption("ClassicLighting",1,true);	
	D3DOptions.alphaToCoverage = getOption("AlphaToCoverage",0,true);
	options.autoFOV = getOption("AutoFOV",1,true);
	options.FPSLimit = getOption("FPSLimit",100,false);
	D3DOptions.simulateMultipassTexturing = getOption("simulateMultipassTexturing",1,true);
	options.unlimitedViewDistance = getOption("unlimitedViewDistance",0,true);
	if(options.unlimitedViewDistance)
		zFar = 65536.0f;
	else
		zFar = 32760.0f;
	 
	//Set parent options
	URenderDevice::Viewport = InViewport;

	//Do some nice compatibility fixing: set processor affinity to single-cpu
	SetProcessAffinityMask(GetCurrentProcess(),0x1);

	//Initialize Direct3D
	
	if(!D3D::init((HWND) InViewport->GetWindow(),D3DOptions))
	{
		GError.Log("Init: Initializing Direct3D failed.");
		return 0;
	}
	
	if(!UD3D10RenderDevice::SetRes(NewX,NewY,NewColorBytes,Fullscreen))
	{
		GError.Log("Init: SetRes failed.");
		return 0;
	}

	textureCache= new (std::nothrow) TextureCache(D3D::getDevice());
	if(!textureCache)
	{
		GError.Log("Error allocating texture cache.");
		return 0;
	}

	texConverter = new (std::nothrow) TexConverter(textureCache);
	if(!texConverter)
	{
		GError.Log("Error allocating texture converter.");
		return 0;
	}

	shader_GouraudPolygon = static_cast<Shader_GouraudPolygon*>(D3D::getShader(D3D::SHADER_GOURAUDPOLYGON));
	shader_Tile = static_cast<Shader_Tile*>(D3D::getShader(D3D::SHADER_TILE));
	shader_ComplexSurface = static_cast<Shader_ComplexSurface*>(D3D::getShader(D3D::SHADER_COMPLEXSURFACE));
	shader_FogSurface = static_cast<Shader_FogSurface*>(D3D::getShader(D3D::SHADER_FOGSURFACE));

	//Brightness
	float brightness;
	GetConfigFloat("WinDrv.WindowsClient", "Brightness", brightness);
	D3D::setBrightness(brightness);

	//URenderDevice::PrecacheOnFlip = 1; //Turned on to immediately recache on init (prevents lack of textures after fullscreen switch)

	QueryPerformanceFrequency(&perfCounterFreq); //Init performance counter frequency.
	
	return 1;
}

/**
Resize buffers and viewport.
\return 1 if resize succesful. On 0, game errors out.

\note Switching to fullscreen exits and reinitializes the renderer.
\note Fullscreen can have values other than 0 and 1 for some reason.
\note This function MUST call URenderDevice::Viewport->ResizeViewport() or the game will stall.
*/
UBOOL UD3D10RenderDevice::SetRes(INT NewX, INT NewY, INT NewColorBytes, UBOOL Fullscreen)
{
	//Without BLIT_Direct3D major flickering occurs when switching from fullscreen to windowed.
	UBOOL Result = URenderDevice::Viewport->ResizeViewport(BLIT_HardwarePaint|BLIT_Direct3D, NewX, NewY, NewColorBytes);
	auto device = D3D::getDevice();
	


	if (!Result) 
	{
		GError.Log("SetRes: Error resizing viewport.");
		return 0;
	}	
	if(!D3D::resize(NewX,NewY,(Fullscreen!=0)))
	{
		GError.Log("SetRes: D3D::Resize failed.");
		return 0;
	}
	
	//Calculate new FOV. Is set, if needed, at frame start as game resets FOV on level load.
	int defaultFOV = 90;

	customFOV = Misc::getFov(defaultFOV,Viewport->SizeX,Viewport->SizeY);

	return 1;
}

/**
Cleanup.
*/
void UD3D10RenderDevice::Exit()
{
	UD3D10RenderDevice::debugs("Direct3D 10 renderer exiting.");
	textureCache->flush();
	delete textureCache;
	delete texConverter;
	D3D::uninit();
	FreeConsole();
}

/**
Empty texture cache.
\param AllowPrecache Enabled if the game allows us to precache; respond by setting URenderDevice::PrecacheOnFlip = 1 if wanted. This does make load times longer.

\note Brightness is applied here; flush is called on each brightness change.
*/

void UD3D10RenderDevice::Flush()
{
	Flush(0);
}

void UD3D10RenderDevice::Flush(UBOOL AllowPrecache)
{
	textureCache->flush();
	D3D::setBrightness(Viewport->GetOuterUClient()->Brightness);
	//If caching is allowed, tell the game to make caching calls (PrecacheTexture() function)

	if (AllowPrecache && options.precache)
		PrecacheOnFlip = 1;
}

/**
Clear screen and depth buffer, prepare buffers to receive data.
\param FlashScale To do with flash effects, see notes.
\param FlashFog To do with flash effects, see notes.
\param ScreenClear The color with which to clear the screen. Used for Rune fog.
\param RenderLockFlags Signify whether the screen should be cleared. Depth buffer should always be cleared.
\param InHitData Something to do with clipping planes; safe to ignore.
\param InHitSize Something to do with clipping planes; safe to ignore.

\note 'Flash' effects are fullscreen colorization, for example when the player is underwater (blue) or being hit (red).
Depending on the values of the related parameters (see source code) this should be drawn; the games don't always send a blank flash when none should be drawn.
EndFlash() ends this, but other renderers actually save the parameters and start drawing it there so it gets blended with the final scene.
\note RenderLockFlags aren't always properly set, this results in for example glitching in the Unreal castle flyover, in the wall of the tower with the Nali on it.
*/
void UD3D10RenderDevice::Lock(FPlane FlashScale, FPlane FlashFog, FPlane ScreenClear, DWORD RenderLockFlags, BYTE* InHitData, INT* InHitSize )
{
	float deltaTime;
	static LARGE_INTEGER oldTime;
	LARGE_INTEGER time;
	if(oldTime.QuadPart==0)
		QueryPerformanceCounter(&oldTime); //Initial time

	
	QueryPerformanceCounter(&time);
	deltaTime  =  (time.QuadPart-oldTime.QuadPart) / (float)perfCounterFreq.QuadPart;
	
	//FPS limiter
	if(options.FPSLimit > 0)
	{		
		while(deltaTime<(float)1/options.FPSLimit) //Busy wait for max accuracy
		{
			QueryPerformanceCounter(&time);
			deltaTime  =  (time.QuadPart-oldTime.QuadPart) / (float)perfCounterFreq.QuadPart;
		}		
	}
	oldTime.QuadPart = time.QuadPart;	
	

	//If needed, set new field of view; the game resets this on level switches etc. Can't be done in config as Unreal doesn't support this.
	if(options.autoFOV && Viewport->Actor->DesiredFOV!=customFOV)
	{		
		TCHAR buf[8]="fov ";
		_itoa_s(customFOV,&buf[4],4,10);
		Viewport->Actor->DesiredFOV =customFOV; //Do this so the value is set even if FOV settings don't take effect (multiplayer mode) 
		//URenderDevice::Viewport->Exec(buf,*GLog); //And this so the FOV change actually happens				
		URenderDevice::Viewport->Exec(buf);
	}

	D3D::newFrame(deltaTime);

	//Set up flash if needed
	Vec4 flashFog = Vec4(FlashFog.X,FlashFog.Y,FlashFog.Z,0.0f);
	D3D::flash(flashFog);		

	shader_Tile->switchBuffers(Shader_Unreal::BUFFER_MULTIPASS);
	shader_GouraudPolygon->switchBuffers(Shader_Unreal::BUFFER_MULTIPASS);
	shader_ComplexSurface->switchBuffers(Shader_Unreal::BUFFER_MULTIPASS);

	//Clear (can use any shader for this)
	shader_GouraudPolygon->clearDepth();	// Depth needs to be always cleared 
	shader_GouraudPolygon->clear(*((Vec4*)&ScreenClear.X));

	drawingWeapon=false;
	drawingHUD=false;
}

/**
Finish rendering.
/param Blit Whether the front and back buffers should be swapped.
*/
void UD3D10RenderDevice::Unlock(UBOOL Blit)
{
	if(Blit)
	{
		D3D::present();
	}
}

/**
Complex surfaces are used for map geometry. They consists of facets which in turn consist of polys (triangle fans).
\param Frame The scene. See SetSceneNode().
\param Surface Holds information on the various texture passes and the surface's PolyFlags.
	- PolyFlags contains the correct flags for this surface. See polyflags.h
	- Texture is the diffuse texture.
	- DetailTexture is the nice close-up detail that's modulated with the diffuse texture for walls. It's up to the renderer to only draw these on near surfaces.
	- LightMap is the precalculated map lighting. Should be drawn with a -.5 pan offset.
	- FogMap is precalculated fog. Should be drawn with a -.5 pan offset. Should be added, not modulated. Flags determine if it should be applied, see polyflags.h.
	- MacroTexture is similar to a detail texture but for far away surfaces. Rarely used.
\param Facet Contains coordinates and polygons.
	- MapCoords are used to calculate texture coordinates. Involved. See code.
	- Polys is a linked list of triangle fan arrays; each element is similar to the models used in DrawGouraudPolygon().
	
\note DetailTexture and FogMap are mutually exclusive; D3D10 renderer just uses seperate binds for them anyway.
\note D3D10 renderer handles DetailTexture range in shader.
\note Check if submitted polygons are valid (3 or more points).
*/
void UD3D10RenderDevice::DrawComplexSurface(FSceneNode* Frame, FSurfaceInfo& Surface, FSurfaceFacet& Facet )
{

	DWORD flags;
	D3D::switchToShader(D3D::SHADER_COMPLEXSURFACE);

	//Cache and set textures
	const TextureCache::TextureMetaData *diffuse=nullptr, *lightMap=nullptr, *detail=nullptr, *fogMap=nullptr, *macro=nullptr;

	PrecacheTexture(*Surface.Texture,Surface.PolyFlags);	

	if(!(diffuse = textureCache->setTexture(shader_ComplexSurface,TextureCache::PASS_DIFFUSE,Surface.Texture->CacheID)))
		return;

	flags = Surface.PolyFlags;
	flags |= diffuse->customPolyFlags;
	
	shader_ComplexSurface->setFlags(flags);
	
	if(Surface.LightMap)
	{
		PrecacheTexture(*Surface.LightMap,0);
		if(!(lightMap = textureCache->setTexture(shader_ComplexSurface,TextureCache::PASS_LIGHT,Surface.LightMap->CacheID)))
			return;
		shader_ComplexSurface->switchPass(TextureCache::PASS_LIGHT,1);
	}
	else
	{
		shader_ComplexSurface->switchPass(TextureCache::PASS_LIGHT,0);
	}

	if(diffuse && diffuse->externalTextures[TextureCache::EXTRA_TEX_DETAIL])
	{
		if(!(detail = textureCache->setTexture(shader_ComplexSurface,TextureCache::PASS_DETAIL,Surface.Texture->CacheID,TextureCache::EXTRA_TEX_DETAIL)))
			return;
		shader_ComplexSurface->switchPass(TextureCache::PASS_DETAIL,1);
	}
	else if(Surface.DetailTexture)
	{
		PrecacheTexture(*Surface.DetailTexture,0);
		if(!(detail = textureCache->setTexture(shader_ComplexSurface,TextureCache::PASS_DETAIL,Surface.DetailTexture->CacheID)))
			return;
		shader_ComplexSurface->switchPass(TextureCache::PASS_DETAIL,1);
	}
	else
	{
		shader_ComplexSurface->switchPass(TextureCache::PASS_DETAIL,0);
	}

	if(Surface.FogMap)
	{
		PrecacheTexture(*Surface.FogMap,0);
		if(!(fogMap = textureCache->setTexture(shader_ComplexSurface,TextureCache::PASS_FOG,Surface.FogMap->CacheID)))
			return;
		shader_ComplexSurface->switchPass(TextureCache::PASS_FOG,1);
	}
	else
	{
		shader_ComplexSurface->switchPass(TextureCache::PASS_FOG,0);
	}
	if(Surface.MacroTexture)
	{
		PrecacheTexture(*Surface.MacroTexture,0);
		if(!(macro = textureCache->setTexture(shader_ComplexSurface,TextureCache::PASS_MACRO,Surface.MacroTexture->CacheID)))
			return;
		shader_ComplexSurface->switchPass(TextureCache::PASS_MACRO,1);
	}
	else
	{
		shader_ComplexSurface->switchPass(TextureCache::PASS_MACRO,0);
	}

	if(diffuse && diffuse->externalTextures[TextureCache::EXTRA_TEX_BUMP])
	{
		if(!textureCache->setTexture(shader_ComplexSurface,TextureCache::PASS_BUMP,Surface.Texture->CacheID,TextureCache::EXTRA_TEX_BUMP))
			return;
		shader_ComplexSurface->switchPass(TextureCache::PASS_BUMP,1);
	}/*
	else if(Surface.Texture->Texture->BumpMap)
	{	
		//Little more work as this is not exposed normally.
		FTextureInfo texInfo;
		#if UNREALTOURNAMENT
			Surface.Texture->Texture->BumpMap->Lock(texInfo,FTime(),0,this);	
		#else
			Surface.Texture->Texture->BumpMap->Lock(texInfo,0,0,this);	
		#endif
		PrecacheTexture(texInfo,Surface.PolyFlags);	
		if(!textureCache->setTexture(shader_ComplexSurface,TextureCache::PASS_BUMP,texInfo.CacheID))
			return;
		Surface.Texture->Texture->BumpMap->Unlock(texInfo);
		shader_ComplexSurface->switchPass(TextureCache::PASS_BUMP,1);
	}*/

	else
	{
		shader_ComplexSurface->switchPass(TextureCache::PASS_BUMP,0);
	}

	if(diffuse && diffuse->externalTextures[TextureCache::EXTRA_TEX_HEIGHT])
	{
		if(!textureCache->setTexture(shader_ComplexSurface,TextureCache::PASS_HEIGHT,Surface.Texture->CacheID,TextureCache::EXTRA_TEX_HEIGHT))
			return;
		shader_ComplexSurface->switchPass(TextureCache::PASS_HEIGHT,1);
	}
	else
	{
		shader_ComplexSurface->switchPass(TextureCache::PASS_HEIGHT,0);
	}

	//Code from OpenGL renderer to calculate texture coordinates
	FLOAT UDot = Facet.MapCoords.XAxis | Facet.MapCoords.Origin;
	FLOAT VDot = Facet.MapCoords.YAxis | Facet.MapCoords.Origin;
	
	//Draw each polygon
	for(FSavedPoly* Poly=Facet.Polys; Poly; Poly=Poly->Next )
	{
		if(Poly->NumPts < 3) //Skip invalid polygons
			continue;

		DynamicGeometryBuffer *buf = static_cast<DynamicGeometryBuffer*>(shader_ComplexSurface->getGeometryBuffer());
		buf->indexTriangleFan(Poly->NumPts); //Reserve space and generate indices for fan		
		for( INT i=0; i<Poly->NumPts; i++ )
		{
			Vertex_ComplexSurface *v = (Vertex_ComplexSurface*) buf->getVertex();
			
			//Code from OpenGL renderer to calculate texture coordinates
			FLOAT U = Facet.MapCoords.XAxis | Poly->Pts[i]->Point;
			FLOAT V = Facet.MapCoords.YAxis | Poly->Pts[i]->Point;
			FLOAT UCoord = U-UDot;
			FLOAT VCoord = V-VDot;

			//Diffuse texture coordinates
			v->TexCoord[0].x = (UCoord-Surface.Texture->Pan.X)*diffuse->multU;
			v->TexCoord[0].y = (VCoord-Surface.Texture->Pan.Y)*diffuse->multV;

			if(Surface.LightMap)
			{
				//Lightmaps require pan correction of -.5
				v->TexCoord[1].x = (UCoord-(Surface.LightMap->Pan.X-0.5f*Surface.LightMap->UScale) )*lightMap->multU; 
				v->TexCoord[1].y = (VCoord-(Surface.LightMap->Pan.Y-0.5f*Surface.LightMap->VScale) )*lightMap->multV;
			}
			if(Surface.DetailTexture)
			{
				v->TexCoord[2].x = (UCoord-Surface.DetailTexture->Pan.X)*detail->multU; 
				v->TexCoord[2].y = (VCoord-Surface.DetailTexture->Pan.Y)*detail->multV;
			}
			if(Surface.FogMap)
			{
				//Fogmaps require pan correction of -.5
				v->TexCoord[3].x = (UCoord-(Surface.FogMap->Pan.X-0.5f*Surface.FogMap->UScale) )*fogMap->multU; 
				v->TexCoord[3].y = (VCoord-(Surface.FogMap->Pan.Y-0.5f*Surface.FogMap->VScale) )*fogMap->multV;			
			}
			if(Surface.MacroTexture)
			{
				v->TexCoord[4].x = (UCoord-Surface.MacroTexture->Pan.X)*macro->multU; 
				v->TexCoord[4].y = (VCoord-Surface.MacroTexture->Pan.Y)*macro->multV;			
			}
			
			v->flags = flags;
			v->Pos = *(Vec3*)&Poly->Pts[i]->Point.X; //Position

		}

	}
}

/**
Gouraud shaded polygons are used for 3D models and surprisingly decals and shadows. 
They are sent with a call of this function per triangle fan, worldview transformed and lit. They do have normals and texture coordinates (no panning).
\param Frame The scene. See SetSceneNode().
\param Info The texture for the model. Models only come with diffuse textures.
\param Pts A triangle fan stored as an array. Each element has a normal, light (i.e. color) and fog (color due to being in fog).
\param NumPts Number of verts in fan.
\param PolyFlags Contains the correct flags for this model. See polyflags.h
\param Span Probably for software renderers.

\note Modulated models (i.e. shadows) shouldn't have a color, and fog should only be applied to models with the correct flags for that. The D3D10 renderer handles this in the shader.
\note Check if submitted polygons are valid (3 or more points).
*/
void UD3D10RenderDevice::DrawGouraudPolygon( FSceneNode* Frame, FTextureInfo& Info, FTransTexture** Pts, int NumPts, DWORD PolyFlags, FSpanBuffer* Span )
{

	if(NumPts<3) //Invalid triangle
		return;
	
	D3D::switchToShader(D3D::SHADER_GOURAUDPOLYGON);	
	
	//Set texture
	PrecacheTexture(Info,PolyFlags);
	const TextureCache::TextureMetaData *diffuse = nullptr;
	if(!(diffuse=textureCache->setTexture(shader_GouraudPolygon,TextureCache::PASS_DIFFUSE,Info.CacheID)))
		return;
	
	DWORD flags = PolyFlags | diffuse->customPolyFlags;
	shader_GouraudPolygon->setFlags(flags);

	//Buffer triangle fans
	DynamicGeometryBuffer *buf = static_cast<DynamicGeometryBuffer*>(shader_GouraudPolygon->getGeometryBuffer());
	buf->indexTriangleFan(NumPts); //Reserve space and generate indices for fan
	for(INT i=0; i<NumPts; i++) //Set fan verts
	{
		Vertex_GouraudPolygon *v = (Vertex_GouraudPolygon*) buf->getVertex();				
		v->Pos = *(Vec3*)&Pts[i]->Point.X;
		v->TexCoord.x = (Pts[i]->U)*diffuse->multU;
		v->TexCoord.y = (Pts[i]->V)*diffuse->multV;
		v->Color = *(Vec4*)&Pts[i]->Light.X;
		v->Fog = *(Vec4*)&Pts[i]->Fog.X;
		v->flags = flags;
		v->Color.w = 1.0f;

	}

}

/**
Used for 2D UI elements, smoke. 
\param Frame The scene. See SetSceneNode().
\param Info The texture for the quad.
\param X X coord in screen space.
\param Y Y coord in screen space.
\param XL Width in pixels
\param YL Height in pixels
\param U Texure U coordinate for left.
\param V Texture V coordinate for top.
\param UL U+UL is coordinate for right.
\param VL V+VL is coordinate for bottom.
\param Span Probably for software renderers.
\param Z coordinate (similar to that of other primitives).
\param Color color
\param Fog fog (unused)
\param PolyFlags Contains the correct flags for this tile. See polyflags.h

\note Need to set scene node here otherwise Deus Ex dialogue letterboxes will look wrong; they aren't properly sent to SetSceneNode() it seems.
\note Drawn by expanding a single vertex to a quad in the geometry shader.
*/
void UD3D10RenderDevice::DrawTile( FSceneNode* Frame, FTextureInfo& Info, FLOAT X, FLOAT Y, FLOAT XL, FLOAT YL, FLOAT U, FLOAT V, FLOAT UL, FLOAT VL, class FSpanBuffer* Span, FLOAT Z, FPlane Color, FPlane Fog, DWORD PolyFlags )
{
	D3D::switchToShader(D3D::SHADER_TILE);
	SetSceneNode(Frame); //Set scene node fix.

	PrecacheTexture(Info,PolyFlags);
	const TextureCache::TextureMetaData *diffuse = nullptr;
	if(!(diffuse=textureCache->setTexture(shader_Tile,TextureCache::PASS_DIFFUSE,Info.CacheID)))
		return;
	
	DWORD flags = PolyFlags | diffuse->customPolyFlags;
	shader_Tile->setFlags(flags);
	DynamicGeometryBuffer *buf = static_cast<DynamicGeometryBuffer*>(shader_Tile->getGeometryBuffer());
	buf->indexSingleVertex(); //Reserve space and generate indices for fan
	Vertex_Tile* v = (Vertex_Tile*) buf->getVertex();
	
	v->XYWH.x = X;
	v->XYWH.y = Y;
	v->XYWH.z = XL;
	v->XYWH.w = YL;
	v->UVWH.x = U * diffuse->multU;
	v->UVWH.y = V * diffuse->multV;
	v->UVWH.z = UL * diffuse->multU;
	v->UVWH.w = VL * diffuse->multV;
	v->z = Z;
	v->Color = *((Vec4*)&Color.X);
	
	v->Color.w = 1.0f;

	v->flags = flags;
}

/**
For UnrealED.
*/
void UD3D10RenderDevice::Draw2DLine( FSceneNode* Frame, FPlane Color, DWORD LineFlags, FVector P1, FVector P2 )
{
}

/**
For UnrealED.
*/
void UD3D10RenderDevice::Draw2DPoint( FSceneNode* Frame, FPlane Color, DWORD LineFlags, FLOAT X1, FLOAT Y1, FLOAT X2, FLOAT Y2, FLOAT Z )
{
}

/**
Clear the depth buffer. Used to draw the skybox behind the rest of the geometry, and weapon in front.
\note It is important that any vertex buffer contents be commited before actually clearing the depth!
*/
void UD3D10RenderDevice::ClearZ( FSceneNode* Frame )
{
	D3D::render();
	shader_GouraudPolygon->clearDepth(); //can be any shader
}

/**
Something to do with clipping planes, not needed. 
*/
void UD3D10RenderDevice::PushHit( const BYTE* Data, INT Count )
{
}

/**
Something to do with clipping planes, not needed. 
*/
void UD3D10RenderDevice::PopHit( INT Count, UBOOL bForce )
{
}

/**
Something to do with FPS counters etc, not needed. 
*/
void UD3D10RenderDevice::GetStats( TCHAR* Result )
{

}

/**
Used for screenshots and savegame previews.
\param Pixels An array of 32 bit pixels in which to dump the back buffer.
*/
void UD3D10RenderDevice::ReadPixels( FColor* Pixels )
{
	UD3D10RenderDevice::debugs("Dumping screenshot...");
	D3D::getScreenshot((Vec4_byte*)Pixels);
	UD3D10RenderDevice::debugs("Done");
}

/**
Various command from the game. Can be used to intercept input. First let the parent class handle the command.

\param Cmd The command
	- GetRes Should return a list of resolutions in string form "HxW HxW" etc.
\param Ar A class to which to log responses using Ar.Log().

\note Deus Ex ignores resolutions it does not like.
*/
UBOOL UD3D10RenderDevice::Exec(const TCHAR* Cmd, FOutputDevice& Ar)
{
	//First try parent
	char* ptr;
	if(ParseCommand(&Cmd,"GetRes"))
	{
		UD3D10RenderDevice::debugs("Getting modelist...");
		TCHAR * resolutions=D3D::getModes();
		Ar.Log(resolutions);
		delete [] resolutions;
		UD3D10RenderDevice::debugs("Done.");
		return 1;
	}	
	else if((ptr=(char*)strstr(Cmd,"Brightness"))) //Brightness is sent as "brightness [val]".
	{
		UD3D10RenderDevice::debugs("Setting brightness.");
		if((ptr=strchr(ptr,' ')))//Search for space after 'brightness'
		{
			float b;
			b=atof(ptr); //Get brightness value;
			D3D::setBrightness(b);
		}
	}
	return 0;
}



/**
This optional function can be used to set the frustum and viewport parameters per scene change instead of per drawXXXX() call.
\param Frame Contains various information with which to build frustum and viewport.
\note Standard Z parameters: near 1, far 32760.
*/
void UD3D10RenderDevice::SetSceneNode(FSceneNode* Frame )
{
	//Calculate projection parameters
	float aspect = Frame->FY/Frame->FX;
	float RProjZ = appTan(Viewport->Actor->FovAngle * PI/360.0 );

 	D3D::setViewPort(Frame->X,Frame->Y,Frame->XB,Frame->YB); //Viewport is set here as it changes during gameplay. For example in DX conversations
	shader_GouraudPolygon->setViewportSize(Frame->X,Frame->Y);	//Shared by all Unreal shaders
	shader_GouraudPolygon->setProjection(aspect,RProjZ,zNear,zFar);	//Shared by all Unreal shaders
}

/**
Store a texture in the renderer-kept texture cache. Only called by the game if URenderDevice::PrecacheOnFlip is 1.
\param Info Texture (meta)data. Includes a CacheID with which to index.
\param PolyFlags Contains the correct flags for this texture. See polyflags.h

\note Already cached textures are skipped, unless it's a dynamic texture, in which case it is updated.
\note Extra care is taken to recache textures that aren't saved as masked, but now have flags indicating they should be (masking is not always properly set).
	as this couldn't be anticipated in advance, the texture needs to be deleted and recreated.
*/
void UD3D10RenderDevice::PrecacheTexture( FTextureInfo& Info, DWORD PolyFlags )
{
	if(textureCache->textureIsCached(Info.CacheID))
	{
		if((Info.TextureFlags & TF_RealtimeChanged ) == TF_RealtimeChanged) //Update already cached realtime textures
		{
			texConverter->update(Info,PolyFlags);
			return;
		}
		else if((PolyFlags & PF_Masked)&&!textureCache->getTextureMetaData(Info.CacheID).masked) //Mask bit changed. Static texture, so must be deleted and recreated.
		{			
			textureCache->deleteTexture(Info.CacheID);	
		}
		else //Texture is already cached and doesn't need to be modified
		{
			return;
		}		
	}

	//Cache texture
	texConverter->convertAndCache(Info, PolyFlags); //Fills TextureInfo with metadata and a D3D format texture		

}

/**
Other renderers handle flashes here by saving the related structures; this one does it in Lock().

Seems this is called before HUD elements are drawn so the flash doesn't overlay them.
*/
void  UD3D10RenderDevice::EndFlash()
{
	/** Postprocess scene and then draw HUD to buffer used in last pass so it doesn't get postprocessed */
	if(!drawingHUD)
	{
		D3D::postprocess();
		shader_Tile->switchBuffers(Shader_Unreal::BUFFER_HUD);
		shader_GouraudPolygon->switchBuffers(Shader_Unreal::BUFFER_HUD); //Rune draws the 'RUNE' main menu logo after EndFlash()
		shader_ComplexSurface->switchBuffers(Shader_Unreal::BUFFER_HUD); //For Deus Ex security cams
		shader_ComplexSurface->clearDepth();
		drawingHUD=true;
	}
}
