#include "shader_tile.h"

Shader_Tile::Shader_Tile(): Shader_Unreal()
{
	this->topology = D3D10_PRIMITIVE_TOPOLOGY_POINTLIST;	
}

bool Shader_Tile::compile(const D3D10_SHADER_MACRO *macros, DWORD shaderFlags)
{
	Shader_Unreal::compile(macros,shaderFlags);
	D3D10_INPUT_ELEMENT_DESC layoutDesc[] =
    {
		{ "POSITION",   0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D10_APPEND_ALIGNED_ELEMENT,   D3D10_INPUT_PER_VERTEX_DATA, 0 },
		{ "POSITION",   1, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D10_APPEND_ALIGNED_ELEMENT,   D3D10_INPUT_PER_VERTEX_DATA, 0 },
		{ "COLOR",   0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D10_APPEND_ALIGNED_ELEMENT,   D3D10_INPUT_PER_VERTEX_DATA, 0 },
		{ "PSIZE",     0, DXGI_FORMAT_R32_FLOAT, 0, D3D10_APPEND_ALIGNED_ELEMENT,  D3D10_INPUT_PER_VERTEX_DATA, 0 },
		{ "BLENDINDICES", 0, DXGI_FORMAT_R32_UINT, 0, D3D10_APPEND_ALIGNED_ELEMENT,  D3D10_INPUT_PER_VERTEX_DATA, 0 },
    };

	if(!Shader_Unreal::compileUnrealShader("d3d10drv\\tile.fx",macros,shaderFlags,layoutDesc,sizeof(layoutDesc)/sizeof(layoutDesc[0])))
		return false;
	
	return true;
}



