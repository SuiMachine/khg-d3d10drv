/**
Rune distance fog
*/

#include "shader_fogsurface.h"

Shader_FogSurface::Shader_FogSurface(): Shader_Unreal()
{

}

bool Shader_FogSurface::compile(const D3D10_SHADER_MACRO *macros, DWORD shaderFlags)
{
	Shader_Unreal::compile(macros,shaderFlags);
	D3D10_INPUT_ELEMENT_DESC layoutDesc[] =
    {
		{ "POSITION",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D10_APPEND_ALIGNED_ELEMENT,   D3D10_INPUT_PER_VERTEX_DATA, 0 },
		{ "COLOR",   0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D10_APPEND_ALIGNED_ELEMENT,   D3D10_INPUT_PER_VERTEX_DATA, 0 },				
		{ "BLENDINDICES", 0, DXGI_FORMAT_R32_UINT, 0, D3D10_APPEND_ALIGNED_ELEMENT,  D3D10_INPUT_PER_VERTEX_DATA, 0 },
    };

	if(!Shader_Unreal::compileUnrealShader("d3d10drv\\fogsurface.fx",macros,shaderFlags,layoutDesc,sizeof(layoutDesc)/sizeof(layoutDesc[0])))
		return false;
	
	return true;
}