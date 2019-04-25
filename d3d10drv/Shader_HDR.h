#pragma once

#include "shader_Postprocess.h"

class Shader_HDR : public Shader_Postprocess
{
private:
	ID3D10ShaderResourceView *sceneTexture; //<* The original input texture, i.e. the rendered scene */

	ID3D10EffectTechnique *sourceToLumTechnique;
	ID3D10EffectTechnique *downscaleLumTechnique;
	ID3D10EffectTechnique *brightPassTechnique;
	ID3D10EffectTechnique *blurTechnique;
	ID3D10EffectTechnique *finalTechnique;
	ID3D10EffectTechnique *adaptiveLumTechnique;

	Vec2_int renderTargetSize;


	static const int NUM_TONEMAP_TEXTURES = 5;
	static const int TONEMAP_RESIZE = 3; /**< How much each tonemapping size downsamples */
	static const int NUM_BLOOM_TEXTURES = 2;
	static const int NUM_ADAPTIVE_LUM_TEXTURES = 2;
	static const int BLOOM_SCALE = 4; /**< Bloom texture size = backbuffersize / BLOOM_SCALE */
	ID3D10ShaderResourceView* toneMapSRV[NUM_TONEMAP_TEXTURES];
	ID3D10RenderTargetView* toneMapRTV[NUM_TONEMAP_TEXTURES];
	ID3D10ShaderResourceView* bloomSRV[NUM_BLOOM_TEXTURES];
	ID3D10RenderTargetView* bloomRTV[NUM_BLOOM_TEXTURES];
	ID3D10ShaderResourceView* adaptiveLumSRV[NUM_ADAPTIVE_LUM_TEXTURES];
	ID3D10RenderTargetView* adaptiveLumRTV[NUM_ADAPTIVE_LUM_TEXTURES];
	ID3D10ShaderResourceView* brightSRV;
	ID3D10RenderTargetView* brightRTV;
	
	struct 
	{
		ID3D10EffectShaderResourceVariable *luminanceTexture;
		ID3D10EffectShaderResourceVariable *bloomTexture;
	} variables;
	
public:		
	Shader_HDR();
	
	bool compile(const D3D10_SHADER_MACRO *macros, DWORD shaderFlags) override;
	bool createRenderTargetViews(ID3D10RenderTargetView *backbuffer, const DXGI_SWAP_CHAIN_DESC &swapChainDesc, int multiSampleCount) override;
	void releaseRenderTargetViews() override;
	void apply() override;
	void setInputTexture(ID3D10ShaderResourceView *texture) override;	
};