#pragma once

#include "shader_postprocess.h"
#include "texturecache.h"

class Shader_FirstPass : public Shader_Postprocess
{
private:
	struct 
	{
		ID3D10EffectVectorVariable* flash;
		ID3D10EffectShaderResourceVariable* texInputMSAA;
	} variables;

	
public:	
	bool compile(const D3D10_SHADER_MACRO *macros, DWORD shaderFlags) override;
	Shader_FirstPass();
	bool createRenderTargetViews(ID3D10RenderTargetView *backbuffer, const DXGI_SWAP_CHAIN_DESC &swapChainDesc, int multiSampleCount) override;
	void Shader_FirstPass::setInputTexture(ID3D10ShaderResourceView *texture) override;
	void flash(Vec4 &color) const;
};