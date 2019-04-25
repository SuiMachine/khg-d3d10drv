#pragma once

#include "shader_unreal.h"
#include "texturecache.h"

class Shader_ComplexSurface : public Shader_Unreal
{
private:
	struct 
	{
		ID3D10EffectScalarVariable* useTexturePass; /**< Bool whether to use each texture pass (shader side) */
		ID3D10EffectShaderResourceVariable* textures;		 
	} variables;
	ID3D10BlendState *bstate_Translucent_ComplexSurface; /**< Special blend state to enable the Glide renderer's multi pass rendering, see shader for details */
	static const int numBools = TextureCache::DUMMY_NUM_TEXTURE_PASSES -1; //-1 because diffuse is always enabled
	bool enableChanged;
	BOOL useTexturePass[numBools];
	bool simulateMultipassTexturing;
	
public:	
	Shader_ComplexSurface(bool simulateMultipassTexturing);
	~Shader_ComplexSurface();
	bool compile(const D3D10_SHADER_MACRO *macros, DWORD shaderFlags) override;	
	void switchPass(TextureCache::TexturePass pass,BOOL val);
	void apply() override;	
	void Shader_ComplexSurface::setTexture(int pass,ID3D10ShaderResourceView *texture) const;
	void setFlags(int flags) override;
};