/**
\class D3D

Main Direct3D functionality; self-contained, does not call external functions apart from the debug output one.
Does not use Unreal data apart from a couple of PolyFlags. Does not require the renderer interface to deal with Direct3D structures.
Quite a lot of code, but splitting this up does not really seem worth it.

An effort is made to reduce the amount of needed draw() calls. As such, state is only changed when absolutely necessary.
*/
#ifdef _DEBUG
#define _DEBUGDX //debug device
#endif

//#define _PERFHUD //nv perfhud support

#include <cstdio>
#include <dxgi.h>
#include <d3d10.h>
//#include <d3dx10.h>
#include "d3d10drv.h"
#include "d3d.h"
#include "shader_gouraudpolygon.h"
#include "shader_tile.h"
#include "shader_complexsurface.h"
#include "shader_fogsurface.h"
#include "shader_firstpass.h"
#include "shader_hdr.h"
#include "shader_finalpass.h"
#include "shader_dummy.h"

/**
D3D Objects
*/
static struct
{
	IDXGIFactory *factory;
	IDXGIOutput* output;
	ID3D10Device* device;
	IDXGISwapChain* swapChain;
	ID3D10RenderTargetView* backBufferRTV;
} D3DObjects;

/**
Shaders
*/
static Shader *currentShader=0;
static Shader* shaders[D3D::DUMMY_NUM_SHADERS];

/*
Misc
*/
static const DXGI_FORMAT BACKBUFFER_FORMAT = DXGI_FORMAT_R8G8B8A8_UNORM;
static const DXGI_FORMAT HDR_FORMAT = DXGI_FORMAT_R11G11B10_FLOAT;

static D3D::Options options;
static HWND hWnd;
static int resX, resY;

/**
Create Direct3D device, swapchain, etc. Purely boilerplate stuff.

\param hWnd Window to use as a surface.
\param createOptions the D3D::Options which to use.
*/
int D3D::init(HWND _hWnd,D3D::Options &createOptions)
{
	hWnd = _hWnd;
	HRESULT hr;

	options = createOptions; //Set config options
	CLAMP(options.samples,1,D3D10_MAX_MULTISAMPLE_SAMPLE_COUNT);
	CLAMP(options.aniso,0,16);
	CLAMP(options.VSync,0,1);
	CLAMP(options.LODBias,-10,10);
	UD3D10RenderDevice::debugs("Initializing Direct3D.");
	
	IDXGIAdapter* selectedAdapter = nullptr; 
	D3D10_DRIVER_TYPE driverType = D3D10_DRIVER_TYPE_HARDWARE; 

	UINT flags=0;
	#ifdef _DEBUGDX
		flags = D3D10_CREATE_DEVICE_DEBUG; //debug runtime (prints debug messages)
	#endif

	hr=D3D10CreateDevice(selectedAdapter,driverType,NULL,flags,D3D10_SDK_VERSION, &D3DObjects.device);
	if(FAILED(hr))
	{
		UD3D10RenderDevice::debugs("Error creating device.");
		return 0;
	}

	if(!D3D::findAALevel()) //Clamp MSAA option to max supported level.
		return 0;

	//Create device and swap chain
	DXGI_SWAP_CHAIN_DESC sd;
	ZeroMemory( &sd, sizeof( sd ) );
	sd.BufferCount = 1;
	//sd.BufferDesc.Width = Window::getWidth();
	//sd.BufferDesc.Height = Window::getHeight();
	sd.BufferDesc.Format = BACKBUFFER_FORMAT;
	//sd.BufferDesc.RefreshRate.Numerator = 60;
	//sd.BufferDesc.RefreshRate.Denominator = 1;
	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	sd.OutputWindow = hWnd;
	sd.SampleDesc.Count = 1;
	sd.SampleDesc.Quality = 0;
	sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
	sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
	sd.Windowed = TRUE;


	//Create device we're actually going to use
	hr = D3D10CreateDeviceAndSwapChain(nullptr, D3D10_DRIVER_TYPE_HARDWARE, NULL, flags, D3D10_SDK_VERSION, &sd,&D3DObjects.swapChain, &D3DObjects.device);
//	hr = D3DObjects.factory->CreateSwapChain(D3DObjects.device,&sd,&D3DObjects.swapChain);
	if(FAILED(hr))
	{
		UD3D10RenderDevice::debugs("Error creating swap chain");
		return 0;
	}
	
//	D3DObjects.factory->MakeWindowAssociation(hWnd,DXGI_MWA_NO_WINDOW_CHANGES ); //Stop DXGI from interfering with the game
	D3DObjects.swapChain->GetContainingOutput(&D3DObjects.output);


		
	if(!initShaders())
		return 0;

#ifdef _DEBUGDX
	//Disable certain debug output
	ID3D10InfoQueue * pInfoQueue;
	D3DObjects.device->QueryInterface( __uuidof(ID3D10InfoQueue),  (void **)&pInfoQueue );

	//set up the list of messages to filter
	D3D10_MESSAGE_ID messageIDs [] = { 
		D3D10_MESSAGE_ID_DEVICE_DRAW_SHADERRESOURCEVIEW_NOT_SET,
		D3D10_MESSAGE_ID_PSSETSHADERRESOURCES_UNBINDDELETINGOBJECT,
		D3D10_MESSAGE_ID_OMSETRENDERTARGETS_UNBINDDELETINGOBJECT,
		D3D10_MESSAGE_ID_CHECKFORMATSUPPORT_FORMAT_DEPRECATED};

	//set the DenyList to use the list of messages
	D3D10_INFO_QUEUE_FILTER filter = { 0 };
	filter.DenyList.NumIDs = sizeof(messageIDs);
	filter.DenyList.pIDList = messageIDs;

	//apply the filter to the info queue
	pInfoQueue->AddStorageFilterEntries( &filter );  

#endif


	return 1;
}

bool D3D::initShaders()
{
	DWORD dwShaderFlags = D3D10_SHADER_ENABLE_STRICTNESS;

	//Set shader macro options
	#define OPTION_TO_STRING(x) char buf##x[10];_itoa_s(options.##x,buf##x,10,10);
	#define OPTIONSTRING_TO_SHADERVAR(x,y) {y,buf##x}
	OPTION_TO_STRING(aniso);
	OPTION_TO_STRING(LODBias);
	OPTION_TO_STRING(samples);
	OPTION_TO_STRING(POM);
	OPTION_TO_STRING(bumpMapping);
	OPTION_TO_STRING(alphaToCoverage);
	OPTION_TO_STRING(classicLighting);
	
	D3D10_SHADER_MACRO macros[] = {
	OPTIONSTRING_TO_SHADERVAR(aniso,"NUM_ANISO"),
	OPTIONSTRING_TO_SHADERVAR(LODBias,"LODBIAS"),	
	OPTIONSTRING_TO_SHADERVAR(samples,"SAMPLES"),
	OPTIONSTRING_TO_SHADERVAR(POM,"POM_ENABLED"),
	OPTIONSTRING_TO_SHADERVAR(alphaToCoverage,"ALPHA_TO_COVERAGE_ENABLED"),
	OPTIONSTRING_TO_SHADERVAR(bumpMapping,"BUMPMAPPING_ENABLED"),
	OPTIONSTRING_TO_SHADERVAR(classicLighting,"CLASSIC_LIGHTING"),
	NULL};	

	

	//Compile shaders
	if(!Shader::initShaderSystem(D3DObjects.device,macros,dwShaderFlags))
	{
		UD3D10RenderDevice::debugs("Error compiling effects file. Please make sure states.fxh resides in the \"\\system\\d3d10drv\" directory.");		
		return 0;
	}

	try
	{
		shaders[D3D::SHADER_GOURAUDPOLYGON] = new Shader_GouraudPolygon();
		shaders[D3D::SHADER_TILE] = new Shader_Tile();
		shaders[D3D::SHADER_COMPLEXSURFACE] = new Shader_ComplexSurface(options.simulateMultipassTexturing==1);
		shaders[D3D::SHADER_FOGSURFACE] = new Shader_FogSurface();
		shaders[D3D::SHADER_FIRSTPASS] = new Shader_FirstPass();
		if(!options.classicLighting)
			shaders[D3D::SHADER_HDR] = new Shader_HDR();
		else 
			shaders[D3D::SHADER_HDR] = new Shader_Dummy();		
		shaders[D3D::SHADER_FINALPASS] = new Shader_FinalPass();
	}
	catch(std::bad_alloc&)
	{
		UD3D10RenderDevice::debugs("Error creating shader instances.");		
		return 0;
	}
	for(int i=0;i<D3D::DUMMY_NUM_SHADERS;i++)
	{
		if(!shaders[i]->compile(macros,dwShaderFlags))
			return false;
	}

	return 1;
}

/**
Create a render target view from the backbuffer and depth stencil buffer.
*/
int D3D::createRenderTargetViews()
{
	HRESULT hr;

	//Get backbuffer
	ID3D10Texture2D* pBuffer;
	hr = D3DObjects.swapChain->GetBuffer( 0, __uuidof(ID3D10Texture2D), (LPVOID*)&pBuffer );
	if(FAILED(hr))
	{
		UD3D10RenderDevice::debugs("Error getting swap chain buffer.");
		return 0;
	}

	//Create backbuffer RTV for if shaders want to write to it directly
	hr = D3DObjects.device->CreateRenderTargetView(pBuffer,nullptr,&D3DObjects.backBufferRTV  );
	if(FAILED(hr))
	{
		UD3D10RenderDevice::debugs("Error creating render target view (backbuffer).");
		return 0;
	}

	//Swap chain params so shaders know size, etc
	DXGI_SWAP_CHAIN_DESC scd;
	D3DObjects.swapChain->GetDesc(&scd);

	for(int i=0;i<D3D::DUMMY_NUM_SHADERS;i++)
	{
		if(!shaders[i]->createRenderTargetViews(D3DObjects.backBufferRTV,scd, options.samples))
		{
			UD3D10RenderDevice::debugs("Error creating render target views.");
			return 0;
		}			
	}
	if(options.classicLighting)
		static_cast<Shader_Dummy*>(shaders[D3D::SHADER_HDR])->setShaderResourceView(shaders[D3D::SHADER_HDR-1]->getResourceView());

	SAFE_RELEASE(pBuffer);
	
	return 1;
}


/**
Cleanup
*/
void D3D::uninit()
{
	UD3D10RenderDevice::debugs("Uninit.");
	D3DObjects.swapChain->SetFullscreenState(FALSE,nullptr); //Go windowed so swapchain can be released
	D3DObjects.device->Flush();
	if(D3DObjects.device)
		D3DObjects.device->ClearState();

	for(int i=0;i<D3D::DUMMY_NUM_SHADERS;i++)
	{
		shaders[i]->releaseRenderTargetViews();
		delete shaders[i];
	}

	SAFE_RELEASE(D3DObjects.backBufferRTV);
	
	SAFE_RELEASE(D3DObjects.swapChain);
	SAFE_RELEASE(D3DObjects.device);
	SAFE_RELEASE(D3DObjects.output);
	SAFE_RELEASE(D3DObjects.factory);
	UD3D10RenderDevice::debugs("Bye.");
}

ID3D10Device *D3D::getDevice()
{
	return D3DObjects.device;
}

/**
Set resolution and windowed/fullscreen.

\note DX10 is volatile; the order in which the steps are taken is critical.
*/
int D3D::resize(int X, int Y, bool fullScreen)
{		
	
	#ifdef _DEBUG
	printf("%d %d %d\n",X,Y,fullScreen);
	#endif
	HRESULT hr;
	DXGI_SWAP_CHAIN_DESC sd;

	switchToShader(-1); //Switch to no pass so stuff will be rebound later.

	//Get swap chain description
	hr = D3DObjects.swapChain->GetDesc(&sd);
	if(FAILED(hr))
	{
		UD3D10RenderDevice::debugs("Failed to get swap chain description.");
		return 0;
	}
	sd.BufferDesc.Width = X; //Set these so we can use this for getclosestmatchingmode
	sd.BufferDesc.Height = Y;
	sd.Flags &= ~DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

	//Release render target views
	SAFE_RELEASE(D3DObjects.backBufferRTV);
	for(int i=0;i<D3D::DUMMY_NUM_SHADERS;i++)
		shaders[i]->releaseRenderTargetViews();

	//Set fullscreen resolution
	if(fullScreen)
	{
		if(FAILED(hr))
		{
			UD3D10RenderDevice::debugs("Failed to get output adapter.");
			return 0;
		}
		DXGI_MODE_DESC fullscreenMode = sd.BufferDesc;
		//hr = D3DObjects.output->FindClosestMatchingMode(&sd.BufferDesc,&fullscreenMode,D3DObjects.device);
		if(FAILED(hr))
		{
			UD3D10RenderDevice::debugs("Failed to get matching display mode.");
			return 0;
		}
		hr = D3DObjects.swapChain->ResizeTarget(&fullscreenMode);
		if(FAILED(hr))
		{
			UD3D10RenderDevice::debugs("Failed to set full-screen resolution.");
			return 0;
		}
		hr = D3DObjects.swapChain->SetFullscreenState(TRUE,nullptr);
		if(FAILED(hr))
		{
			UD3D10RenderDevice::debugs("Failed to switch to full-screen.");
			//return 0;
		}
		//MS recommends doing this
		fullscreenMode.RefreshRate.Denominator=0;
		fullscreenMode.RefreshRate.Numerator=0;
		hr = D3DObjects.swapChain->ResizeTarget(&fullscreenMode);
		if(FAILED(hr))
		{
			UD3D10RenderDevice::debugs("Failed to set full-screen resolution.");
			return 0;
		}
		sd.BufferDesc = fullscreenMode;
		sd.Flags |= DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
	}	

	
	//This must be done after fullscreen stuff or blitting will be used instead of flipping
	hr = D3DObjects.swapChain->ResizeBuffers(sd.BufferCount,X,Y,sd.BufferDesc.Format,sd.Flags); //Resize backbuffer
	if(FAILED(hr))
	{
		UD3D10RenderDevice::debugs("Failed to resize back buffer.");
		return 0;
	}
	if(!createRenderTargetViews()) //Recreate render target view
	{
		return 0;
	}
	
	//Reset viewport, it's sometimes lost.
	D3D10_VIEWPORT vp;
	vp.Width = X;
	vp.Height = Y;
	vp.MinDepth = 0.0;
	vp.MaxDepth = 1.0;
	vp.TopLeftX = 0;
	vp.TopLeftY = 0;
	D3DObjects.device->RSSetViewports(1,&vp);

	resX=X;
	resY=Y;
	return 1;
}

/**
Get the format to use to draw geometry, HDR passes
*/
DXGI_FORMAT D3D::getDrawPassFormat()
{
	if(options.classicLighting)
		return BACKBUFFER_FORMAT;
	else
		return HDR_FORMAT;
}

/**
Set up things for rendering a new frame. For example, update shader time.

\param time Time since last frame in seconds
*/
void D3D::newFrame(float time)
{	
	for(int i=0;i<D3D::DUMMY_NUM_SHADERS;i++)
		shaders[i]->getGeometryBuffer()->newFrame();
	
	static_cast<Shader_Postprocess*>(shaders[SHADER_FIRSTPASS])->setElapsedTime(time);
}




/**
Draw current buffer contents.
*/
void D3D::render()
{	
	//Draw main geometry buffer
	if(currentShader && currentShader->getGeometryBuffer()->hasContents())
	{
		currentShader->apply();
	}
}


/**
Set up render targets, textures, etc. for the chosen shader.
\param index The number of the shader. -1 for no shader.

\note Current buffer contents will be drawn unless switch is to -1. Use this to prevent static buffers from being redrawn on switch.
*/
void D3D::switchToShader(int index)
{
	static int currIndex=-1;


	if(index!=currIndex)
	{			
		if(index<0)
			currentShader=0;
		else
		{
			render();
			currentShader=shaders[index];
			currentShader->bind();
		}
		currIndex = index;
	}
}

Shader* D3D::getShader(int index)
{
	return shaders[index];
}

void D3D::postprocess()
{
	ID3D10ShaderResourceView *texture;
	if(currentShader)
		 texture = currentShader->getResourceView(); //Output of the draw passes
	else
		texture = nullptr;
	shaders[SHADER_FIRSTPASS]->setFlags(0); //Turn off blending and depth
	setViewPort(resX,resY,0,0);
	for(int i=D3D::SHADER_FIRSTPASS;i<D3D::SHADER_FINALPASS;i++)
	{
		switchToShader(i);
		Shader_Postprocess *pps = static_cast<Shader_Postprocess*>(currentShader);	
		pps->setInputTexture(texture);
		texture = currentShader->getResourceView(); //This output is input for next pass
		D3D::render();
		switchToShader(-1);
	}

}

/**
Flip
*/
void D3D::present()
{
	HRESULT hr;
	
	//Apply final pass
	shaders[SHADER_FINALPASS]->setFlags(0);
	ID3D10ShaderResourceView *texture = shaders[D3D::SHADER_FINALPASS-1]->getResourceView(); //Output of the draw passes
	switchToShader(D3D::SHADER_FINALPASS);
	Shader_Postprocess *pps = static_cast<Shader_Postprocess*>(currentShader);	
	pps->setInputTexture(texture);		
	D3D::render();
	switchToShader(-1);


	//Flip
	hr = D3DObjects.swapChain->Present((options.VSync!=0),0);
	if(FAILED(hr))
	{
		UD3D10RenderDevice::debugs("Present error.");
		return;
	}
}

/**
Set up the viewport. Also sets height and width in shader.
\note Buffered polys must be committed first, otherwise glitches will occur (for example, Deus Ex security cams).
*/
void D3D::setViewPort(int X, int Y, int left, int top)
{
	static int oldX, oldY, oldLeft, oldTop;
	if(X!=oldX || Y!=oldY || left != oldLeft || top != oldTop)
	{
		
		render();
		D3D10_VIEWPORT vp;
		vp.Width = X;
		vp.Height = Y;
		vp.MinDepth = 0.0;
		vp.MaxDepth = 1.0;
		vp.TopLeftX = left;
		vp.TopLeftY = top;

		D3DObjects.device->RSSetViewports(1,&vp);
		
	}
	oldLeft = left; oldTop = top; oldX = X; oldY = Y;

	static_cast<Shader_Postprocess*>(shaders[SHADER_FIRSTPASS])->setInputTextureOffset(left,top);
}


/**
Create a string of supported display modes.
\return String of modes. Caller must delete[] this.
\note No error checking.
\note Deus Ex and Unreal (non-Gold) only show 16 resolutions, so for it make it the 16 highest ones. Also for Unreal Gold for compatibity with v226.
*/	
TCHAR *D3D::getModes()
{
	TCHAR *out;

	//Get number of modes
	UINT num = 0;	
	D3DObjects.output->GetDisplayModeList(BACKBUFFER_FORMAT, 0, &num, nullptr);
	const int resStringLength = 10;
	out = new TCHAR[num*resStringLength+1];
	out[0]=0;
	
	DXGI_MODE_DESC * descs = new DXGI_MODE_DESC[num];
	D3DObjects.output->GetDisplayModeList(BACKBUFFER_FORMAT, 0, &num, descs);
	
	//Add each mode once (disregard refresh rates etc)
	const int maxItems = 16;
	int h[maxItems];
	int w[maxItems];
	h[0]=0;
	w[0]=0;
	int slot=maxItems-1;
	//Go through the modes backwards and find up to 16 ones
	for(int i=num;i>0&&slot>0;i--)
	{		
		if(slot==maxItems-1 || w[slot+1]!=descs[i-1].Width || h[slot+1]!=descs[i-1].Height)
		{
			w[slot]=descs[i-1].Width;
			h[slot]=descs[i-1].Height;
			printf("%d\n",w[slot]);
			slot--;
		}
	}
	//Build the string by now going through the saved modes forwards.
	for(int i=slot+1;i<maxItems;i++)	
	{
		TCHAR curr[resStringLength+1];
		sprintf_s(curr,resStringLength+1,"%dx%d ",w[i],h[i]);
		strcat_s(out,num*resStringLength+1,curr);	
	}
	
	//Throw away trailing space
	out[strlen(out)-1]=0;
	delete [] descs;
	return out;
}

/**
Return screen data by copying the back buffer to a staging resource and copying this into an array.
\param buf Array in which the data will be written.
\note No error checking.
*/
void D3D::getScreenshot(Vec4_byte* buf)
{
	ID3D10Texture2D* backBuffer;
	D3DObjects.swapChain->GetBuffer( 0, __uuidof(ID3D10Texture2D), (LPVOID*)&backBuffer );

	D3D10_TEXTURE2D_DESC desc;
	backBuffer->GetDesc(&desc);
	desc.BindFlags = 0;
	desc.SampleDesc.Count=1;

	//Copy backbuffer
	ID3D10Texture2D* tstaging;
	desc.CPUAccessFlags = D3D10_CPU_ACCESS_READ;
	desc.Usage = D3D10_USAGE_STAGING;
	D3DObjects.device->CreateTexture2D(&desc,nullptr,&tstaging);	

	D3DObjects.device->CopySubresourceRegion(tstaging,0,0,0,0,backBuffer,0,nullptr);
	

	//Map copy
	D3D10_MAPPED_TEXTURE2D tempMapped;
	tstaging->Map(0,D3D10_MAP_READ ,0,&tempMapped);

	//Convert BGRA to RGBA, minding the source stride.
	Vec4_byte* rowSrc =(Vec4_byte*) tempMapped.pData;
	Vec4_byte* rowDst = buf;
	for(unsigned int row=0;row<desc.Height;row++)
	{
		for(unsigned int col=0;col<desc.Width;col++)
		{
			rowDst[col].x = rowSrc[col].z;
			rowDst[col].y = rowSrc[col].y;
			rowDst[col].z = rowSrc[col].x;
			rowDst[col].w = rowSrc[col].w;	
		}
		//Go to next row
		rowSrc+=(tempMapped.RowPitch/sizeof(Vec4_byte));
		rowDst+=desc.Width;
		
	}

	//Clean up
	tstaging->Unmap(0);
	SAFE_RELEASE(backBuffer);
	SAFE_RELEASE(tstaging);
	UD3D10RenderDevice::debugs("Done.");
}

/**
Find the maximum level of MSAA supported by the device and clamp the options.MSAA setting to this.
\return 1 if succesful.
*/
int D3D::findAALevel()
{
	//Create device to check MSAA support with
	HRESULT hr;

	//Check MSAA support by going down the counts until a suitable one is found	
	UINT levels=0;
	int count;
	for(count = options.samples ;levels==0&&count>0;count--)
	{
		hr = D3DObjects.device->CheckMultisampleQualityLevels(D3D::getDrawPassFormat(),count,&levels);
		if(FAILED(hr))
		{
			UD3D10RenderDevice::debugs("Error getting MSAA support level.");
			return 0;
		}
		if(levels!=0) //sample amount is supported
			break;
	}
	if(count!=options.samples)
	{
		UD3D10RenderDevice::debugs("Anti aliasing setting decreased; requested setting unsupported.");
		options.samples = count;
	}
	return 1;
}

/**
Sets the in-shader brightness.
\param brightness Brightness 0-1.
*/
void D3D::setBrightness(float brightness)
{
	static_cast<Shader_FinalPass*>(shaders[D3D::SHADER_FINALPASS])->setBrightness(brightness);
}


/**
Notify the shader a flash effect should be drawn.
*/
void D3D::flash(Vec4 &color)
{	
	static_cast<Shader_FirstPass*>(shaders[D3D::SHADER_FIRSTPASS])->flash(color);
}
