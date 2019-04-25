#pragma once

class TextureCache;

#include <d3d10.h>
#include <d3dx10.h>
#include <unordered_map>
#include "shader_unreal.h"


class TextureCache
{

public:
	/**
	List of texture passes that can be bound.
	\note DUMMY_NUM_TEXTURE_PASSES is so arrays can be indexed etc. for each pass.
	*/
	enum TexturePass {PASS_DIFFUSE,PASS_LIGHT,PASS_DETAIL,PASS_FOG,PASS_MACRO,PASS_BUMP,PASS_HEIGHT,DUMMY_NUM_TEXTURE_PASSES}; 

	/**
	External textures: textures that don't replace another texture, but are extra bumpmaps, detail textures etc.
	*/
	enum ExternalTextures{EXTRA_TEX_DETAIL,EXTRA_TEX_BUMP,EXTRA_TEX_HEIGHT,DUMMY_NUM_EXTERNAL_TEXTURES};
	
	struct ExternalTexture
	{
		char* suffix; /**< Filename suffix */
		UINT mipLevels; /**< Number of mip levels used */
	};
	static const ExternalTexture externalTextures[DUMMY_NUM_EXTERNAL_TEXTURES];

	/** Texture metadata stored and retrieved with cached textures */
	struct TextureMetaData
	{
		//UINT height;
		//UINT width;

		/** Precalculated parameters with which to normalize texture coordinates */
		FLOAT multU;
		FLOAT multV;
		bool masked; /**< Tracked to fix masking issues, see UD3D10RenderDevice::PrecacheTexture */
		bool externalTextures[DUMMY_NUM_EXTERNAL_TEXTURES]; /**< Which extra texture slots are used */
		DWORD customPolyFlags; /**< To allow override textures to have their own polyflags set in a file */
	};

	/** Cached, API format texture */
	struct CachedTexture
	{
		TextureMetaData metadata;
		ID3D10ShaderResourceView* resourceView;
		ID3D10Texture2D* texture;
		ID3D10ShaderResourceView* externalTextures[DUMMY_NUM_EXTERNAL_TEXTURES]; /**< Extra detail/bump textures which can be used even if the game doesn't offer any, see texconversion.cpp */
	};


private:
	/**
	Texture cache variables
	*/
	struct
	{
		DWORD64 boundTextureID[DUMMY_NUM_TEXTURE_PASSES]; /**< CPU side bound texture IDs for the various passes as defined in the shader */
	} texturePasses;


	std::unordered_map <unsigned __int64, CachedTexture> textureCache; /**< The actual cache */


	ID3D10Device *device;

public:
	/**@name Texture cache */
	//@{

	TextureCache(ID3D10Device *device);
	ID3D10Texture2D *createTexture(const D3D10_TEXTURE2D_DESC &desc, const D3D10_SUBRESOURCE_DATA &data) const;
	void updateMip(const FTextureInfo& Info,int mipNum, const D3D10_SUBRESOURCE_DATA &data) const;
	bool loadFileTexture(char* fileName, ID3D10Texture2D **tex, D3DX10_IMAGE_LOAD_INFO *loadInfo) const;
	void cacheTexture(DWORD64 id,const TextureMetaData &metadata, ID3D10Texture2D *tex,int extraIndex=-1);
	bool textureIsCached(DWORD64 id) const;	
	const TextureMetaData &getTextureMetaData(DWORD64 id) const;
	const TextureMetaData *setTexture(const Shader_Unreal* shader, TexturePass pass,DWORD64 id,int extraIndex=-1);
	void deleteTexture(DWORD64 id);
	void flush();
	//@}
};