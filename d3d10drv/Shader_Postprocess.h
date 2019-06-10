#pragma once

#include "Shader.h"
#include "geometrybuffer.h"
#include "vertexformats.h"
#include <math.h>

class Shader_Postprocess : public Shader
{
private:
	static GeometryBuffer *quadGeometryBuffer;
	static ID3D10InputLayout *quadInputLayout;
	static ID3D10EffectPool *pool;

	struct _variables
	{		
		ID3D10EffectShaderResourceVariable* inputTexture;		
		ID3D10EffectVectorVariable* viewPort;
		ID3D10EffectVectorVariable* inputTextureOffset;
		ID3D10EffectScalarVariable* elapsedTime;
	};
	
protected:
	void setViewPort(int x, int y) const;

	
public:
	static _variables variables;

	Shader_Postprocess();
	~Shader_Postprocess();

	//From Shader
	bool compile(const D3D10_SHADER_MACRO *macros, DWORD shaderFlags) override;

	bool compilePostProcessingShader(const TCHAR* filename, const D3D10_SHADER_MACRO *macros, DWORD shaderFlags);
	virtual void setInputTexture(ID3D10ShaderResourceView *texture);	
	void setElapsedTime(float t);
	void setInputTextureOffset(int left, int top) const;
};