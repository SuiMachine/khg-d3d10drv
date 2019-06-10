/**
Shaders that implement the basic Unreal geometry pipeline inherit from this. They share a few shader variables and use the same (dynamic) geometry buffer.
THIS MEANS THAT THEY MUST USE EQUAL SIZE VERTICES
*/

#include <new>
#include "Shader_Unreal.h"
#include "DynamicGeometryBuffer.h" 
#include <xnamath.h>

DynamicGeometryBuffer *Shader_Unreal::dynamicGeometryBuffer;
ID3D10EffectPool *Shader_Unreal::pool;
Shader_Unreal::_variables Shader_Unreal::variables;
ID3D10RenderTargetView *Shader_Unreal::unrealRTV;
ID3D10DepthStencilView *Shader_Unreal::unrealDSV;
ID3D10ShaderResourceView *Shader_Unreal::unrealSRV;
ID3D10DepthStencilView *Shader_Unreal::noMSAADSV;

static const int BUFFER_SIZE = 20000; //Size of buffer for geometry sent by the engine

Shader_Unreal::Shader_Unreal(): Shader()
{

}

Shader_Unreal::~Shader_Unreal()
{	
	SAFE_RELEASE(pool);

	//Make sure the static members only get deleted once
	if(dynamicGeometryBuffer)
	{
		delete dynamicGeometryBuffer;
		dynamicGeometryBuffer = nullptr;		
	}
	geometryBuffer = nullptr;
}

bool Shader_Unreal::compile(const D3D10_SHADER_MACRO *macros, DWORD shaderFlags)
{
	
	if(dynamicGeometryBuffer==nullptr)
	{

		dynamicGeometryBuffer = new (std::nothrow) DynamicGeometryBuffer(device);		
		if(!dynamicGeometryBuffer || !dynamicGeometryBuffer->create(BUFFER_SIZE,sizeof(Vertex_ComplexSurface)))
		{
			UD3D10RenderDevice::debugs("Failed to create dynamic geometry buffer.");
			return false;
		}
		
	}
	if(pool==nullptr)
	{
		hr = D3DX10CreateEffectPoolFromFile("d3d10drv\\unrealpool.fxh",macros,nullptr,"fx_4_0",shaderFlags,0,device,nullptr,&pool,&blob,&hr);
		if(!checkCompileResult(hr))
			return false;
	
		variables.projection = pool->AsEffect()->GetVariableByName("projection")->AsMatrix();
		variables.viewportHeight = pool->AsEffect()->GetVariableByName("viewportHeight")->AsScalar();
		variables.viewportWidth = pool->AsEffect()->GetVariableByName("viewportWidth")->AsScalar();
		variables.diffuseTexture = pool->AsEffect()->GetVariableByName("texDiffuse")->AsShaderResource();
	}
	this->geometryBuffer = dynamicGeometryBuffer; //All 'Unreal' shaders share the same dynamic geometry buffer
	return true;
}

bool Shader_Unreal::compileUnrealShader(const TCHAR* filename,const D3D10_SHADER_MACRO *macros, DWORD shaderFlags,const D3D10_INPUT_ELEMENT_DESC *elementDesc, int numElements)
{
	hr = D3DX10CreateEffectFromFile(filename,macros,nullptr,"fx_4_0",shaderFlags, D3D10_EFFECT_COMPILE_CHILD_EFFECT,device,pool,nullptr,&effect,&blob,nullptr);
	if(!checkCompileResult(hr))
		return 0;	

    //Create the input layout.
	D3D10_PASS_DESC passDesc;
	ID3D10EffectTechnique* t = effect->GetTechniqueByIndex(0);
	if(!t->IsValid())
	{
		UD3D10RenderDevice::debugs("Failed to find technique 0.");
		return 0;
	}
	ID3D10EffectPass* p = t->GetPassByIndex(0);
	if(!p->IsValid())
	{
		UD3D10RenderDevice::debugs("Failed to find pass 0.");
		return 0;
	}
	p->GetDesc(&passDesc);
	hr = device->CreateInputLayout(elementDesc, numElements, passDesc.pIAInputSignature, passDesc.IAInputSignatureSize, &vertexLayout);
	if(FAILED(hr))
	{
		UD3D10RenderDevice::debugs("Error creating input layout.");
		return 0;
	}

	return 1;
}

bool Shader_Unreal::createRenderTargetViews(ID3D10RenderTargetView *backbuffer, const DXGI_SWAP_CHAIN_DESC &swapChainDesc, int multiSampleCount)
{
	
	//Set shared views for all Unreal shaders
	if(unrealRTV==nullptr)
	{		
		if(!Shader::createRenderTargetViews(D3D::getDrawPassFormat(),DXGI_FORMAT_D32_FLOAT,1,1,multiSampleCount,swapChainDesc))
			return false;
		unrealRTV = renderTargetView;
		unrealDSV = depthStencilView ;
		unrealSRV = shaderResourceView;

		if(multiSampleCount>1)
		{
			if(!Shader::createRenderTargetViews(DXGI_FORMAT_UNKNOWN,DXGI_FORMAT_D32_FLOAT,1,1,1,swapChainDesc))
				return false;
			noMSAADSV = depthStencilView;
		}
		
	}

	renderTargetView = unrealRTV;
	depthStencilView = unrealDSV;
	shaderResourceView = unrealSRV;

	return 1;
}

void Shader_Unreal::releaseRenderTargetViews()
{
	SAFE_RELEASE(unrealRTV);
	renderTargetView = NULL;

	SAFE_RELEASE(unrealDSV);
	depthStencilView = NULL;

	SAFE_RELEASE(unrealSRV);
	shaderResourceView = NULL;

	SAFE_RELEASE(noMSAADSV);
	noMSAADSV = NULL;

	Shader::releaseRenderTargetViews();
}


/**
Set projection matrix parameters.
\param aspect The viewport aspect ratio.
\param XoverZ Ratio between frustum X and Z. Projection parameters are for z=1, so x over z gives x coordinate; and x/z*aspect=y/z=y.
*/
void Shader_Unreal::setProjection(float aspect, float XoverZ, float zNear, float zFar) const
{		
	XMMATRIX m;
	static float oldAspect, oldXoverZ, oldzNear;
	if(aspect!=oldAspect || oldXoverZ != XoverZ || zNear != oldzNear)
	{
		D3D::render();
		float xzProper = XoverZ*zFar; //Scale so view isn't zoomed in.
		m = XMMatrixPerspectiveOffCenterLH(-xzProper,xzProper,-aspect*xzProper,aspect*xzProper,zFar, zNear); //Similar to glFrustum
		variables.projection->SetMatrix(&m.m[0][0]);
		oldAspect = aspect;
		oldXoverZ = XoverZ;
		oldzNear = zNear;
	}

}

void Shader_Unreal::setViewportSize(float x, float y) const
{
	variables.viewportWidth->SetFloat(x);
	variables.viewportHeight->SetFloat(y);
}

void Shader_Unreal::setTexture(int pass,ID3D10ShaderResourceView *texture) const
{
	if(pass==0)
		variables.diffuseTexture->SetResourceArray(&texture,pass,1);	
}

/**
Clear backbuffer(s)
\param clearColor The color with which the screen is cleared.
*/
void Shader_Unreal::clear(Vec4& clearColor) const
{
	if(renderTargetView)
		device->ClearRenderTargetView(renderTargetView,(float*) &clearColor);
}

/**
Clear depth
*/
void Shader_Unreal::clearDepth() const
{
	if(depthStencilView)
	{
		device->ClearDepthStencilView(depthStencilView, D3D10_CLEAR_DEPTH, 0.0, 0);
	}
}

/**
Switch to reuse of current buffer to be able to draw things after postprocessing (i.e. they are drawn to the rendertarget of the last post processing step, which should still be bound)
*/
void Shader_Unreal::switchBuffers(enum BUFFERS buffer)
{
	if(buffer==Shader_Unreal::BUFFER_MULTIPASS)
	{
		renderTargetView = unrealRTV ;
		depthStencilView = unrealDSV;
	}
	else
	{
		renderTargetView = 0;
		if(noMSAADSV) //Switch to no-MSAA dsv if we required and made one
			depthStencilView = noMSAADSV;
		else
			depthStencilView = unrealDSV;
	}

}