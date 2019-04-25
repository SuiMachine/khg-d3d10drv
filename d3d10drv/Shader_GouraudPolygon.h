#pragma once

#include "shader_unreal.h"

class Shader_GouraudPolygon : public Shader_Unreal
{
private:
	
public:
	struct 
	{
		ID3D10EffectScalarVariable* projectionMode; /**< Projection transform mode (near/far) */		
		ID3D10EffectVectorVariable* fogColor; /**< Fog color */
		ID3D10EffectScalarVariable* fogDist; /**< Fog end distance */
	} variables;

	Shader_GouraudPolygon();
	bool compile(const D3D10_SHADER_MACRO *macros, DWORD shaderFlags) override;
	void fog(float dist,Vec4 *color)  const;
};