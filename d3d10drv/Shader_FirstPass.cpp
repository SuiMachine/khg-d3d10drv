/**
First shader pass, done after geomatry is drawn. Resolves MSAA.
*/

#include "shader_firstpass.h"

Shader_FirstPass::Shader_FirstPass(): Shader_Postprocess()
{

}

bool Shader_FirstPass::compile(const D3D10_SHADER_MACRO *macros, DWORD shaderFlags)
{
	Shader_Postprocess::compile(macros,shaderFlags);
	if(!Shader_Postprocess::compilePostProcessingShader("d3d10drv\\firstpass.fx",macros,shaderFlags))
		return false;
	
	variables.flash = effect->GetVariableByName("flash")->AsVector();
	variables.texInputMSAA = effect->GetVariableByName("inputTextureMSAA")->AsShaderResource();

	return true;
}


bool Shader_FirstPass::createRenderTargetViews(ID3D10RenderTargetView *backbuffer, const DXGI_SWAP_CHAIN_DESC &swapChainDesc, int multiSampleCount)
{
	return Shader::createRenderTargetViews(D3D::getDrawPassFormat(),DXGI_FORMAT_UNKNOWN,1,1,1,swapChainDesc);
}

void Shader_FirstPass::setInputTexture(ID3D10ShaderResourceView *texture)
{
	variables.texInputMSAA->SetResource(texture);
}

void Shader_FirstPass::flash(Vec4 &color) const
{
	variables.flash->SetFloatVector((float*)&color);
}
