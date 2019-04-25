/**
Final shader pass, done after HUD elements are drawn. Handles brightness.
*/

#include "shader_finalpass.h"

Shader_FinalPass::Shader_FinalPass(): Shader_Postprocess()
{

}

bool Shader_FinalPass::compile(const D3D10_SHADER_MACRO *macros, DWORD shaderFlags)
{
	Shader_Postprocess::compile(macros,shaderFlags);
	if(!Shader_Postprocess::compilePostProcessingShader("d3d10drv\\finalpass.fx",macros,shaderFlags))
		return false;
	
	variables.brightness = effect->GetVariableByName("brightness")->AsScalar();
	
	return true;
}

bool Shader_FinalPass::createRenderTargetViews(ID3D10RenderTargetView *backbuffer, const DXGI_SWAP_CHAIN_DESC &swapChainDesc, int multiSampleCount)
{
	renderTargetView=backbuffer;	
	return 1;
}

void Shader_FinalPass::releaseRenderTargetViews()
{	
	//Do nothing, D3D class releases backbuffer
}

void Shader_FinalPass::setBrightness(float brightness) const
{
	variables.brightness->SetFloat(brightness);
}

