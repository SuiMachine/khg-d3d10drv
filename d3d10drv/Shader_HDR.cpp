#include "shader_HDR.h"

Shader_HDR::Shader_HDR(): Shader_Postprocess()
{
	for(int i=0;i<NUM_TONEMAP_TEXTURES;i++)
	{
		toneMapRTV[i] = nullptr;
		toneMapSRV[i] = nullptr;
	}
	for(int i=0;i<NUM_BLOOM_TEXTURES;i++)
	{
		bloomRTV[i] = nullptr;
		bloomSRV[i] = nullptr;
	}
	for(int i=0;i<NUM_ADAPTIVE_LUM_TEXTURES;i++)
	{
		adaptiveLumRTV[i] = nullptr;
		adaptiveLumSRV[i] = nullptr;
	}
	brightSRV = nullptr;
	brightRTV = nullptr;
}



bool Shader_HDR::compile(const D3D10_SHADER_MACRO *macros, DWORD shaderFlags)
{
	Shader_Postprocess::compile(macros,shaderFlags);
	if(!Shader_Postprocess::compilePostProcessingShader("d3d10drv\\hdr.fx",macros,shaderFlags))
		return false;
	
	sourceToLumTechnique = effect->GetTechniqueByName("sourceToLum");
	downscaleLumTechnique = effect->GetTechniqueByName("downscaleLum");
	brightPassTechnique = effect->GetTechniqueByName("brightPass");
	blurTechnique = effect->GetTechniqueByName("blur");
	finalTechnique = effect->GetTechniqueByName("finalPass");
	adaptiveLumTechnique = effect->GetTechniqueByName("adaptiveLum");

	variables.luminanceTexture=effect->GetVariableByName("luminanceTex")->AsShaderResource();
	variables.bloomTexture=effect->GetVariableByName("bloomTex")->AsShaderResource();

	return true;
}


bool Shader_HDR::createRenderTargetViews(ID3D10RenderTargetView *backbuffer, const DXGI_SWAP_CHAIN_DESC &swapChainDesc, int multiSampleCount)
{
	
	renderTargetSize.x = swapChainDesc.BufferDesc.Width;
	renderTargetSize.y = swapChainDesc.BufferDesc.Height;

	D3D10_TEXTURE2D_DESC desc;
	desc.CPUAccessFlags = 0;
	desc.MiscFlags = 0;
	desc.ArraySize = 1;
	desc.BindFlags = D3D10_BIND_RENDER_TARGET | D3D10_BIND_SHADER_RESOURCE;
	desc.Usage = D3D10_USAGE_DEFAULT;	
	desc.MipLevels = 1;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;


	ID3D10Texture2D *texture;

	//Tone mapping textures
	int nSampleLen = pow((float)TONEMAP_RESIZE,NUM_TONEMAP_TEXTURES-1);
	for(int i=0;i<Shader_HDR::NUM_TONEMAP_TEXTURES;i++)
	{
		
		desc.Format = DXGI_FORMAT_R32_FLOAT;
		desc.Width = nSampleLen;
		desc.Height = nSampleLen;

		if(FAILED(device->CreateTexture2D(&desc, nullptr, &texture)))
			return false;

        //Create the render target view
		if(FAILED(device->CreateRenderTargetView(texture, nullptr, &toneMapRTV[i])))
			return false;

        //Create the shader resource view
		if(FAILED(device->CreateShaderResourceView(texture, nullptr, &toneMapSRV[i])))
			return false;

		SAFE_RELEASE(texture);

        nSampleLen /= TONEMAP_RESIZE;
	}

	//Adaptive luminance textures
	for(int i=0;i<Shader_HDR::NUM_ADAPTIVE_LUM_TEXTURES;i++)
	{
		
		desc.Format = DXGI_FORMAT_R32_FLOAT;
		desc.Width = 1;
		desc.Height = 1;

		if(FAILED(device->CreateTexture2D(&desc, nullptr, &texture)))
			return false;

        //Create the render target view
		if(FAILED(device->CreateRenderTargetView(texture, nullptr, &adaptiveLumRTV[i])))
			return false;

        //Create the shader resource view
		if(FAILED(device->CreateShaderResourceView(texture, nullptr, &adaptiveLumSRV[i])))
			return false;

		SAFE_RELEASE(texture);
	}

	//Bright pass texture
	desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	desc.Width = renderTargetSize.x /BLOOM_SCALE;
	desc.Height = renderTargetSize.y /BLOOM_SCALE;
	if(FAILED(device->CreateTexture2D(&desc, nullptr, &texture)))
			return false;

    //Create the render target view
	if(FAILED(device->CreateRenderTargetView(texture, nullptr, &brightRTV)))
		return false;

    //Create the shader resource view
	if(FAILED(device->CreateShaderResourceView(texture, nullptr, &brightSRV)))
		return false;

	SAFE_RELEASE(texture);

	//Bloom textures
	for(int i=0;i<Shader_HDR::NUM_BLOOM_TEXTURES;i++)
	{
		desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		desc.Width = swapChainDesc.BufferDesc.Width/BLOOM_SCALE;
		desc.Height = swapChainDesc.BufferDesc.Height/BLOOM_SCALE;

		if(FAILED(device->CreateTexture2D(&desc, nullptr, &texture)))
			return false;

        //Create the render target view
		if(FAILED(device->CreateRenderTargetView(texture, nullptr, &bloomRTV[i])))
			return false;

        //Create the shader resource view
		if(FAILED(device->CreateShaderResourceView(texture, nullptr, &bloomSRV[i])))
			return false;

		SAFE_RELEASE(texture);

	}
	

	//Create output texture
	return Shader::createRenderTargetViews(DXGI_FORMAT_R8G8B8A8_UNORM,DXGI_FORMAT_UNKNOWN,1,1,1,swapChainDesc);
}

void Shader_HDR::releaseRenderTargetViews()
{	
	for(int i=0;i<Shader_HDR::NUM_TONEMAP_TEXTURES;i++)
	{
		SAFE_RELEASE(toneMapRTV[i]);
		SAFE_RELEASE(toneMapSRV[i]);
	}

	SAFE_RELEASE(brightRTV);
	SAFE_RELEASE(brightSRV);

	for(int i=0;i<Shader_HDR::NUM_BLOOM_TEXTURES;i++)
	{
		SAFE_RELEASE(bloomRTV[i]);
		SAFE_RELEASE(bloomSRV[i]);
	}

	for(int i=0;i<Shader_HDR::NUM_ADAPTIVE_LUM_TEXTURES;i++)
	{
		SAFE_RELEASE(adaptiveLumRTV[i]);
		SAFE_RELEASE(adaptiveLumSRV[i]);
	}
	Shader_Postprocess::releaseRenderTargetViews();
}

void Shader_HDR::apply()
{

	int targetSize = pow((float)TONEMAP_RESIZE,NUM_TONEMAP_TEXTURES-1);

	
	//Source-to-sampled-luminance
	device->OMSetRenderTargets(1,&toneMapRTV[0],nullptr);
	setViewPort(targetSize,targetSize);
	sourceToLumTechnique->GetPassByIndex(0)->Apply(0);
	geometryBuffer->draw();

	//Downscale luminance
	for(int i=1;i<NUM_TONEMAP_TEXTURES;i++)
	{
		targetSize/=TONEMAP_RESIZE;
		device->OMSetRenderTargets(1,&toneMapRTV[i],nullptr);
		Shader_Postprocess::setInputTexture(toneMapSRV[i-1]);
		setViewPort(targetSize,targetSize);
		downscaleLumTechnique->GetPassByIndex(0)->Apply(0);
		geometryBuffer->draw();
	}

	//Adaptive luminance
	static int adaptiveLumTexture=0;
	Shader_Postprocess::setInputTexture(adaptiveLumSRV[adaptiveLumTexture]); //Current adapted luminance
	device->OMSetRenderTargets(1,&adaptiveLumRTV[(adaptiveLumTexture+1)%NUM_ADAPTIVE_LUM_TEXTURES],nullptr);	
	variables.luminanceTexture->SetResource(toneMapSRV[NUM_TONEMAP_TEXTURES-1]); //Current scene luminance
	adaptiveLumTechnique->GetPassByIndex(0)->Apply(0);
	geometryBuffer->draw();
	adaptiveLumTexture = (adaptiveLumTexture+1)%NUM_ADAPTIVE_LUM_TEXTURES;
	
	//Bright pass
	setViewPort(renderTargetSize.x/BLOOM_SCALE,renderTargetSize.y/BLOOM_SCALE);
	Shader_Postprocess::setInputTexture(sceneTexture);
	variables.luminanceTexture->SetResource(adaptiveLumSRV[adaptiveLumTexture]);
	device->OMSetRenderTargets(1,&brightRTV,nullptr);
	brightPassTechnique->GetPassByIndex(0)->Apply(0);
	geometryBuffer->draw();
	
	//Bloom pass
	setViewPort(renderTargetSize.x/BLOOM_SCALE,renderTargetSize.y/BLOOM_SCALE);
	Shader_Postprocess::setInputTexture(brightSRV);
	device->OMSetRenderTargets(1,&bloomRTV[0],nullptr);
	blurTechnique->GetPassByIndex(0)->Apply(0);
	geometryBuffer->draw();
	Shader_Postprocess::setInputTexture(bloomSRV[0]);
	device->OMSetRenderTargets(1,&bloomRTV[1],nullptr);
	blurTechnique->GetPassByIndex(1)->Apply(0);
	geometryBuffer->draw();
	
	//Final pass
	Shader_Postprocess::setViewPort(renderTargetSize.x,renderTargetSize.y);
	device->OMSetRenderTargets(1,&renderTargetView,nullptr);
	Shader_Postprocess::setInputTexture(sceneTexture);
	variables.bloomTexture->SetResource(bloomSRV[1]);	
	variables.luminanceTexture->SetResource(adaptiveLumSRV[adaptiveLumTexture]);
	finalTechnique->GetPassByIndex(0)->Apply(0);
	geometryBuffer->draw();

	

}

void Shader_HDR::setInputTexture(ID3D10ShaderResourceView *texture)
{
	sceneTexture = texture;
	Shader_Postprocess::setInputTexture(texture);
}