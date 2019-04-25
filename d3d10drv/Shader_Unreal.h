#pragma once

#include "Shader.h"
#include "dynamicgeometrybuffer.h"

class Shader_Unreal : public Shader
{
private:
	static DynamicGeometryBuffer *dynamicGeometryBuffer;
	static ID3D10EffectPool *pool;
	static ID3D10RenderTargetView *unrealRTV;
	static ID3D10DepthStencilView *unrealDSV;
	static ID3D10ShaderResourceView* unrealSRV;
	static ID3D10DepthStencilView *noMSAADSV; /**< Depth stencil view for things drawn after post processing*/

	struct _variables
	{
		ID3D10EffectMatrixVariable* projection; /**< projection matrix */		
		ID3D10EffectShaderResourceVariable* diffuseTexture;
		ID3D10EffectScalarVariable* viewportHeight; /**< Viewport height in pixels */
		ID3D10EffectScalarVariable* viewportWidth; /**< Viewport width in pixels */
	};
	
	
public:
	enum BUFFERS{BUFFER_MULTIPASS,BUFFER_HUD};

	static _variables variables;

	Shader_Unreal();
	virtual ~Shader_Unreal();

	//From Shader
	bool compile(const D3D10_SHADER_MACRO *macros, DWORD shaderFlags) override;

	bool compileUnrealShader(const char* filename,const D3D10_SHADER_MACRO *macros, DWORD shaderFlags,const D3D10_INPUT_ELEMENT_DESC *elementDesc, int numElements);
	bool createRenderTargetViews(ID3D10RenderTargetView *backbuffer, const DXGI_SWAP_CHAIN_DESC &swapChainDesc,int multiSampleCount);
	void releaseRenderTargetViews();
	void Shader_Unreal::setProjection(float aspect, float XoverZ, float zNear, float zFar) const;
	void Shader_Unreal::setViewportSize(float x, float y) const;
	virtual void Shader_Unreal::setTexture(int  pass,ID3D10ShaderResourceView *texture) const;
	void Shader_Unreal::clear(Vec4& clearColor) const;
	void Shader_Unreal::clearDepth() const;
	void switchBuffers(enum BUFFERS buffer);
};