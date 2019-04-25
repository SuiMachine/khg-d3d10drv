/**
Cache for game textures; also handles the external extra textures.
*/

#include "texturecache.h"
#include "d3d10drv.h"


//Definitions of extra external textures
const TextureCache::ExternalTexture TextureCache::externalTextures[TextureCache::DUMMY_NUM_EXTERNAL_TEXTURES] = {{".detail",1},{".bump",0},{".height",0}};


TextureCache::TextureCache(ID3D10Device *device)
{
	this->device = device;
}

/**
Create a texture from a descriptor and data to fill it with.
\param desc Direct3D texture description.
\param data Data to fill the texture with.
*/
ID3D10Texture2D *TextureCache::createTexture(const D3D10_TEXTURE2D_DESC &desc,const D3D10_SUBRESOURCE_DATA &data) const
{
	//Creates a texture, setting the TextureInfo's data member.
	HRESULT hr;	

	ID3D10Texture2D *texture;
	hr=device->CreateTexture2D(&desc,&data, &texture);
	if(FAILED(hr))
	{
		UD3D10RenderDevice::debugs("Error creating texture resource.");
		return nullptr;
	}
	return texture;
}

/**
Update a single texture mip using a copy operation.
\param id CacheID to insert texture with.
\param mipNum Mip level to update.
\param data Data to write to the mip.
*/
void TextureCache::updateMip(const FTextureInfo& Info,int mipNum,const D3D10_SUBRESOURCE_DATA &data) const
{
	//If texture is currently bound, draw buffers before updating
	for(int i=0;i<TextureCache::DUMMY_NUM_TEXTURE_PASSES;i++)
	{
		if(texturePasses.boundTextureID[i]==Info.CacheID)
		{
			D3D::render();
			break;
		}
	}

	//Update
	const auto& entry = textureCache.find(Info.CacheID)->second;
	//device->UpdateSubresource(entry.texture,mipNum,nullptr,(void*) data.pSysMem,data.SysMemPitch,data.SysMemSlicePitch);

	//UpdateSubResource leads to flickering on nvidia
	D3D10_MAPPED_TEXTURE2D Mapping;
	entry.texture->Map(mipNum,D3D10_MAP_WRITE_DISCARD,0,&Mapping);

	unsigned char* pDst = static_cast<unsigned char*>(Mapping.pData);
	const unsigned char* pSrc = static_cast<const unsigned char*>(data.pSysMem);
	for (int y = 0; y < Info.VClamp; y++)
	{
		memcpy(pDst, pSrc, Info.UClamp*sizeof(DWORD)>>mipNum);
		pSrc += data.SysMemPitch;
		pDst += Mapping.RowPitch;
	}

	entry.texture->Unmap(mipNum);
}

/**
Load a texture from a .dds file

\return true when succesful
*/
bool TextureCache::loadFileTexture(char* fileName, ID3D10Texture2D **tex, D3DX10_IMAGE_LOAD_INFO *loadInfo) const
{
	HRESULT hr; 

	hr = D3DX10CreateTextureFromFile(device, fileName, loadInfo, nullptr, (ID3D10Resource** )tex, nullptr);
	if(FAILED(hr))
		return false;

	return true;
}

/**
Create a resource view (texture usable by shader) from a filled-in texture and cache it. Caller can then release the texture.
\param id CacheID to insert texture with.
\param metadata Texture metadata.
\param tex A filled Direct3D texture.
\param extraIndex Index of the extra external texture slot to use (optional)
*/
void TextureCache::cacheTexture(unsigned __int64 id,const TextureMetaData &metadata, ID3D10Texture2D *tex, int extraIndex)
{
	HRESULT hr;

	D3D10_TEXTURE2D_DESC desc;
	tex->GetDesc(&desc);
	//Create resource view
	ID3D10ShaderResourceView* r;
	D3D10_SHADER_RESOURCE_VIEW_DESC srDesc;
	srDesc.Format = desc.Format;
	srDesc.ViewDimension = D3D10_SRV_DIMENSION_TEXTURE2D;
	srDesc.Texture2D.MostDetailedMip = 0;
	srDesc.Texture2D.MipLevels = desc.MipLevels;

	hr = device->CreateShaderResourceView(tex,&srDesc,&r);
	if(FAILED(hr))
	{
		UD3D10RenderDevice::debugs("Error creating texture shader resource view.");
		return;
	}

	//Cache texture
	if(extraIndex==-1)
	{
		CachedTexture c;
		c.metadata = metadata;
		tex->AddRef();
		c.texture = tex;
		c.resourceView = r;
		for(int i=0;i<DUMMY_NUM_EXTERNAL_TEXTURES;i++)
		{
			c.externalTextures[i]=nullptr;			
		}
		textureCache[id]=c;	
	}
	else //add extra texture
	{
		CachedTexture *c = &textureCache[id];
		c->externalTextures[extraIndex] = r;
		c->metadata.externalTextures[extraIndex]=true;
	}

}

/**
Returns true if texture is in cache.
\param id CacheID for texture.
*/
bool TextureCache::textureIsCached(DWORD64 id) const
{	
	return textureCache.find(id) != textureCache.end();
}

/**
Returns texture metadata.
\param id CacheID for texture.
*/
const TextureCache::TextureMetaData &TextureCache::getTextureMetaData(DWORD64 id) const
{
	return textureCache.find(id)->second.metadata;
}


/**
Set the texture for a texture pass (diffuse, lightmap, etc).
Texture is only set if it's not already the current one for that pass.
Cached polygons (using the previous set of textures) are drawn before the switch is made.
\param id CacheID for texture. NULL sets no texture for the pass (by disabling it using a shader constant).
\param extraIndex Index of the extra external texture slot to use (optional), -1 for none.
\return texture metadata so renderer can use parameters such as scale/pan; NULL is texture not found
*/
const TextureCache::TextureMetaData *TextureCache::setTexture(const Shader_Unreal* shader,TexturePass pass,DWORD64 id, int extraIndex)
{	
	static TextureMetaData *metadata[DUMMY_NUM_TEXTURE_PASSES]; //Cache this so it can even be returned when no texture was actually set (because same id as last time)	

	if(id!=texturePasses.boundTextureID[pass]) //If different texture than previous one, draw geometry in buffer and switch to new texture
	{			
		texturePasses.boundTextureID[pass]=id;
		
		D3D::render();

		//Turn on and switch to new texture			
		CachedTexture *tex;
		if(!textureIsCached(id)) //Texture not in cache, conversion probably went wrong.
			return nullptr;
		tex = &textureCache[id];
		if(extraIndex==-1)
			shader->setTexture(pass,tex->resourceView);
		else
			shader->setTexture(pass,tex->externalTextures[extraIndex]);
			
		metadata[pass] = &tex->metadata;
		
	}

	return metadata[pass];
}

/**
Delete a texture (so it can be overwritten with an updated one).
*/
void TextureCache::deleteTexture(DWORD64 id)
{
	std::unordered_map<DWORD64,CachedTexture>::iterator i = textureCache.find(id);
	if(i==textureCache.end())
		return;
	SAFE_RELEASE(i->second.texture);
	SAFE_RELEASE(i->second.resourceView);
	
	for(int j=0;j<DUMMY_NUM_EXTERNAL_TEXTURES;j++)
	{
		SAFE_RELEASE(i->second.externalTextures[j]);
	}

	textureCache.erase(i);
}

/**
Clear texture cache.
*/
void TextureCache::flush()
{
	//Clear bindings so textures will be rebound after flush
	for(int i=0;i<DUMMY_NUM_TEXTURE_PASSES;i++)
	{
		texturePasses.boundTextureID[i]=0;
	}

	//Delete textures
	for(std::unordered_map<DWORD64,CachedTexture>::iterator i=textureCache.begin();i!=textureCache.end();i++)
	{	
		while(i->second.resourceView)
		{
			SAFE_RELEASE(i->second.resourceView);
		}

		while(i->second.texture)
		{
			SAFE_RELEASE(i->second.texture);
		}
		
		for(int j=0;j<DUMMY_NUM_EXTERNAL_TEXTURES;j++)
		{
			SAFE_RELEASE(i->second.externalTextures[j]);
		}
	}
	textureCache.clear();
}