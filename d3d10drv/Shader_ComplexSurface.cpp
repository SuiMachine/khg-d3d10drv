#include "shader_complexsurface.h"

Shader_ComplexSurface::Shader_ComplexSurface(bool simulateMultipassTexturing): simulateMultipassTexturing(simulateMultipassTexturing), Shader_Unreal()
{

}

Shader_ComplexSurface::~Shader_ComplexSurface()
{
	SAFE_RELEASE(bstate_Translucent_ComplexSurface);
}


bool Shader_ComplexSurface::compile(const D3D10_SHADER_MACRO *macros, DWORD shaderFlags)
{
	Shader_Unreal::compile(macros,shaderFlags);
	D3D10_INPUT_ELEMENT_DESC layoutDesc[] =
    {
		{ "POSITION",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D10_APPEND_ALIGNED_ELEMENT,   D3D10_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD",   0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D10_APPEND_ALIGNED_ELEMENT,   D3D10_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD",   1, DXGI_FORMAT_R32G32_FLOAT, 0, D3D10_APPEND_ALIGNED_ELEMENT,   D3D10_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD",   2, DXGI_FORMAT_R32G32_FLOAT, 0, D3D10_APPEND_ALIGNED_ELEMENT,   D3D10_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD",   3, DXGI_FORMAT_R32G32_FLOAT, 0, D3D10_APPEND_ALIGNED_ELEMENT,   D3D10_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD",   4, DXGI_FORMAT_R32G32_FLOAT, 0, D3D10_APPEND_ALIGNED_ELEMENT,   D3D10_INPUT_PER_VERTEX_DATA, 0 },
		{ "BLENDINDICES", 0, DXGI_FORMAT_R32_UINT, 0, D3D10_APPEND_ALIGNED_ELEMENT,  D3D10_INPUT_PER_VERTEX_DATA, 0 },
    };

	if(!Shader_Unreal::compileUnrealShader("d3d10drv\\complexsurface.fx",macros,shaderFlags,layoutDesc,sizeof(layoutDesc)/sizeof(layoutDesc[0])))
		return false;
	
	variables.useTexturePass = effect->GetVariableByName("useTexturePass")->AsScalar();
	variables.textures = effect->GetVariableByName("textures")->AsShaderResource();
	effect->GetVariableByName("bstate_Translucent_ComplexSurface")->AsBlend()->GetBlendState(0,&bstate_Translucent_ComplexSurface);
	
	return true;
}

void Shader_ComplexSurface::switchPass(TextureCache::TexturePass pass, BOOL val)
{
	if(useTexturePass[pass-1]!=val)	
	{
		D3D::render(); //draw geometry that might have used other textures
		enableChanged = true;
	}
	useTexturePass[pass-1]=val;
}



void Shader_ComplexSurface::apply() 
{
	//apply texture enabled array

	if(enableChanged)
	{
		variables.useTexturePass->SetBoolArray(this->useTexturePass,0,this->numBools);
		enableChanged=false;
	}
	Shader::apply();
}

void Shader_ComplexSurface::setTexture(int pass,ID3D10ShaderResourceView *texture) const
{
	if(pass==0)
		Shader_Unreal::setTexture(pass,texture);
	else
		variables.textures->SetResourceArray(&texture,pass-1,1);	
}

void Shader_ComplexSurface::setFlags(int flags)
{
	//Apply blendmode that in conjunction with the shader emulates the look of multi-pass textured lightmaps.
	
	ID3D10BlendState *b = states.bstate_Translucent;
	if(simulateMultipassTexturing)
	{
		states.bstate_Translucent = bstate_Translucent_ComplexSurface;
	}
	Shader::setFlags(flags);
	states.bstate_Translucent = b;
	
}