#pragma once

class Shader;

#include <d3d10.h>
#include <d3dx10.h>
#include "d3d10drv.h"
#include "geometrybuffer.h"

class Shader
{

protected:
	static struct StateStruct
	{
		ID3D10DepthStencilState* dstate_Enable;
		ID3D10DepthStencilState* dstate_Disable;
		ID3D10BlendState* bstate_Alpha;
		ID3D10BlendState* bstate_Translucent;
		ID3D10BlendState* bstate_Modulate;
		ID3D10BlendState* bstate_NoBlend;
		ID3D10BlendState* bstate_Masked;
		ID3D10BlendState* bstate_Invis;	
	} states;

	GeometryBuffer* geometryBuffer;
	ID3D10RenderTargetView* renderTargetView;
	ID3D10DepthStencilView* depthStencilView;
	ID3D10InputLayout* vertexLayout;
	ID3D10ShaderResourceView* shaderResourceView;
	static ID3D10Blob* blob;
	static ID3D10Device *device;
	ID3D10Effect* effect;
	D3D10_PRIMITIVE_TOPOLOGY topology;
	HRESULT hr;

	static bool checkCompileResult(HRESULT hr);
	bool createRenderTargetViews(DXGI_FORMAT format, DXGI_FORMAT depthFormat,float scaleX, float scaleY, int samples, const DXGI_SWAP_CHAIN_DESC &swapChainDesc);


public:
	Shader();
	static bool initShaderSystem(ID3D10Device *device, const D3D10_SHADER_MACRO *macros, DWORD shaderFlags);
	virtual bool compile(const D3D10_SHADER_MACRO *macros, DWORD shaderFlags)=0;
	virtual bool createRenderTargetViews(ID3D10RenderTargetView *backbuffer, const DXGI_SWAP_CHAIN_DESC &swapChainDesc, int multiSampleCount)=0;
	virtual void releaseRenderTargetViews();
	virtual ~Shader();
	virtual void bind();
	virtual void apply();
	GeometryBuffer *getGeometryBuffer() const;
	ID3D10ShaderResourceView *getResourceView() const;
	virtual void setFlags(int flags);
};