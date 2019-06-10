/**
Main shader class, offers housekeeping and helper functions.
*/

#include "shader.h"
#include "polyflags.h" //for polyflags

Shader::StateStruct Shader::states;
ID3D10Device *Shader::device;
ID3D10Blob* Shader::blob;

Shader::Shader(): geometryBuffer(nullptr), renderTargetView(nullptr), depthStencilView(nullptr), shaderResourceView(nullptr), effect(nullptr), vertexLayout(nullptr), topology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST)
{

}

Shader::~Shader()
{
	delete geometryBuffer;	
	SAFE_RELEASE(effect);
	SAFE_RELEASE(vertexLayout);
	SAFE_RELEASE(states.dstate_Enable);
	SAFE_RELEASE(states.dstate_Disable);
	SAFE_RELEASE(states.bstate_NoBlend);
	SAFE_RELEASE(states.bstate_Translucent);
	SAFE_RELEASE(states.bstate_Modulate);
	SAFE_RELEASE(states.bstate_Alpha);
}

/**
Global initialization of shader subsystem
*/
bool Shader::initShaderSystem(ID3D10Device *device, const D3D10_SHADER_MACRO *macros, DWORD shaderFlags)
{
	Shader::device = device;

	//Get states
	ID3D10Effect* tempEffect;
	HRESULT hr;
	hr = D3DX10CreateEffectFromFile("d3d10drv\\states.fxh",macros,nullptr,"fx_4_0",shaderFlags,0,device,nullptr,nullptr,&tempEffect,nullptr,nullptr);
	
	if(!checkCompileResult(hr))
	{		
		return false;
	}

	tempEffect->GetVariableByName("dstate_Enable")->AsDepthStencil()->GetDepthStencilState(0,&states.dstate_Enable);
	tempEffect->GetVariableByName("dstate_Disable")->AsDepthStencil()->GetDepthStencilState(0,&states.dstate_Disable);
	tempEffect->GetVariableByName("bstate_Translucent")->AsBlend()->GetBlendState(0,&states.bstate_Translucent);
	tempEffect->GetVariableByName("bstate_Modulate")->AsBlend()->GetBlendState(0,&states.bstate_Modulate);
	tempEffect->GetVariableByName("bstate_NoBlend")->AsBlend()->GetBlendState(0,&states.bstate_NoBlend);
	tempEffect->GetVariableByName("bstate_Masked")->AsBlend()->GetBlendState(0,&states.bstate_Masked);
	tempEffect->GetVariableByName("bstate_Alpha")->AsBlend()->GetBlendState(0,&states.bstate_Alpha);
	tempEffect->GetVariableByName("bstate_Invis")->AsBlend()->GetBlendState(0,&states.bstate_Invis);
	SAFE_RELEASE(tempEffect);

	return true;
}

/**
Check shader compile result and show errors if applicable.
*/
bool Shader::checkCompileResult(HRESULT hr)
{
	if(blob) //Show compile errors if present
			UD3D10RenderDevice::debugs((TCHAR*) blob->GetBufferPointer());
	if(FAILED(hr))
	{
		UD3D10RenderDevice::debugs("Error compiling effects file. Please make sure it resides in the \"\\system\\d3d10drv\" directory.");		
		return false;
	}
	return true;
}

/**
Create render target, depth and resource views.

\param samples Desired multisample amount for these buffers (can be different than what the game is using)

\note If a format is null the buffer/view will be skipped.
*/
bool Shader::createRenderTargetViews(DXGI_FORMAT format, DXGI_FORMAT depthFormat, float scaleX, float scaleY, int samples, const DXGI_SWAP_CHAIN_DESC &swapChainDesc)
{
	
	ID3D10Texture2D *tex=nullptr;
	if(format!=DXGI_FORMAT_UNKNOWN)
	{
		
		//Create texture
		D3D10_TEXTURE2D_DESC texDesc;
		texDesc.ArraySize=1;
		texDesc.BindFlags=D3D10_BIND_RENDER_TARGET | D3D10_BIND_SHADER_RESOURCE;
		texDesc.CPUAccessFlags=0;
		texDesc.Format= format;
		texDesc.Height = swapChainDesc.BufferDesc.Height*scaleY;
		texDesc.Width = swapChainDesc.BufferDesc.Width*scaleX;
		texDesc.MipLevels = 1;
		texDesc.MiscFlags = 0;
		texDesc.SampleDesc.Count=samples;
		texDesc.SampleDesc.Quality = 0;
		texDesc.Usage=D3D10_USAGE_DEFAULT;

		if(FAILED(device->CreateTexture2D(&texDesc,0,&tex)))
		{
			UD3D10RenderDevice::debugs("Error creating buffer texture.");
			return 0;
		}

		//Render target view
		hr = device->CreateRenderTargetView(tex,nullptr,&renderTargetView);	
		if(FAILED(hr))
		{
			UD3D10RenderDevice::debugs("Error creating render target view.");
			return 0;
		}

		//Shader resource view
		if(FAILED(device->CreateShaderResourceView(tex, nullptr,&shaderResourceView)))
		{
			UD3D10RenderDevice::debugs("Error creating shader resource view.");
			return 0;
		}

		SAFE_RELEASE(tex);
		
	}

	if(depthFormat!=DXGI_FORMAT_UNKNOWN)
	{
		//Depth stencil
		D3D10_TEXTURE2D_DESC descDepth;
		
		//Internal texture
		ID3D10Texture2D *depthTexInternal;
		descDepth.Width = swapChainDesc.BufferDesc.Width*scaleX;
		descDepth.Height = swapChainDesc.BufferDesc.Height*scaleY;
		descDepth.MipLevels = 1;
		descDepth.ArraySize = 1;
		descDepth.Format =  depthFormat;
		descDepth.SampleDesc.Count = samples;
		descDepth.SampleDesc.Quality = 0;
		descDepth.Usage = D3D10_USAGE_DEFAULT;
		descDepth.BindFlags = D3D10_BIND_DEPTH_STENCIL;
		descDepth.CPUAccessFlags = 0;
		descDepth.MiscFlags = 0;
		if(FAILED(device->CreateTexture2D( &descDepth, nullptr,&depthTexInternal )))
		{
			UD3D10RenderDevice::debugs("Depth texture creation failed.");
			return 0;
		}

		//Depth Stencil view
		if(FAILED(device->CreateDepthStencilView(depthTexInternal,nullptr,&depthStencilView )))
		{
			UD3D10RenderDevice::debugs("Error creating render target view (depth).");
			return 0;
		}
		SAFE_RELEASE(depthTexInternal);
	}
	return 1;
}

void Shader::releaseRenderTargetViews()
{	
	SAFE_RELEASE(renderTargetView);
	SAFE_RELEASE(depthStencilView);
	SAFE_RELEASE(shaderResourceView);
}



void Shader::bind()
{
	UINT offset=0;	
	geometryBuffer->bind();
	device->IASetPrimitiveTopology(topology);	
	device->IASetInputLayout(vertexLayout);
	Shader::setFlags(0);
	if(renderTargetView==0) //If not set, reuse existing one
	{
		ID3D10RenderTargetView *rv;
		device->OMGetRenderTargets(1,&rv,nullptr);
		device->OMSetRenderTargets(1,&rv,depthStencilView);
	}
	else
		device->OMSetRenderTargets(1,&renderTargetView,depthStencilView);
}

/**
Draw the shader's buffer contents.
*/
void Shader::apply()
{
	effect->GetTechniqueByIndex(0)->GetPassByIndex(0)->Apply(0);
	geometryBuffer->draw();
}

GeometryBuffer *Shader::getGeometryBuffer() const
{
	return geometryBuffer;
}

ID3D10ShaderResourceView *Shader::getResourceView() const
{
	return shaderResourceView;
}

/** Handle flags that change depth or blend state. See polyflags.h.
Only done if flag is different from current.
If there's any buffered geometry, it will drawn before setting the new flags.
\param flags Unreal polyflags.
\param d3dflags Custom flags defined in d3d.h.
\note Bottleneck; make sure buffers are only rendered due to flag changes when absolutely necessary	
**/
void Shader::setFlags(int flags)
{
	const int BLEND_FLAGS = PF_Translucent | PF_Modulated |PF_Invisible |PF_Masked
//	#ifdef RUNE
		| PF_AlphaBlend
//	#endif
		;
	const int RELEVANT_FLAGS = BLEND_FLAGS|PF_Occlude;
	
	static int currFlags=0;

	
	if(!(flags & (PF_Translucent|PF_Modulated))) //If none of these flags, occlude (opengl renderer)
	{
		flags |= PF_Occlude;
	}

	int changedFlags = currFlags ^ flags;
	if (changedFlags&RELEVANT_FLAGS) //only blend flag changes are relevant	
	{
		D3D::render();

		//Set blend state		
		if(changedFlags & BLEND_FLAGS) //Only set blend state if it actually changed
		{
			ID3D10BlendState *blendState;
			if(flags&PF_Invisible)
			{
				blendState = states.bstate_Invis;
			}
			
			else if(flags&PF_Translucent)
			{
				blendState = states.bstate_Translucent;
			}
			else if(flags&PF_Modulated)
			{				
				blendState = states.bstate_Modulate;
			}
						
		//	#ifdef RUNE
			else if (flags&PF_AlphaBlend)
			{
				blendState = states.bstate_Alpha;
			}
		//	#endif
			else if (flags&PF_Masked)
			{
				blendState = states.bstate_Masked;
			}
			else
			{
				blendState = states.bstate_NoBlend;
			}
			device->OMSetBlendState(blendState,nullptr,0xffffffff);
	
		}
		
		//Set depth state
		if(changedFlags & PF_Occlude)
		{
			ID3D10DepthStencilState *depthState;
			if(flags & PF_Occlude)
				depthState = states.dstate_Enable;
			else
				depthState = states.dstate_Disable;
			device->OMSetDepthStencilState(depthState,1);
		}

		currFlags = flags;
	}
}
