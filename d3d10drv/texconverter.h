/**
\file texconversion.h
*/

#pragma once
#include "texturecache.h"
#include "d3d10drv.h"

class TexConverter
{
private:
	TextureCache *textureCache;

	/**
	Format for a texture, tells the conversion functions if data should be allocated, block sizes taken into account, etc
	*/
	struct TextureFormat
	{
		bool supported; /**< Is format supported by us */
		char blocksize; /**< Block size (in one dimension) in bytes for compressed textures */
		char pixelsPerBlock; /**< Pixels each block of a compressed texture encodes */
		bool directAssign; /**< No conversion and temporary storage needed */
		DXGI_FORMAT d3dFormat; /**< D3D format to use when creating texture */
		void (*conversionFunc)(const FTextureInfo&, DWORD, void *, int);	/**< Conversion function to use if no direct assignment possible */
	};
	static TexConverter::TextureFormat formats[];

	/**@name Format conversion functions */
	//@{
	static void fromPaletted(const FTextureInfo& Info,DWORD PolyFlags,void *target, int mipLevel);
	static void fromBGRA7(const FTextureInfo& Info,DWORD PolyFlags,void *target,int mipLevel);
	//@}

	static void convertMip(const FTextureInfo& Info,const TextureFormat &format, DWORD PolyFlags,int mipLevel, D3D10_SUBRESOURCE_DATA &data);
	static TextureCache::TextureMetaData buildMetaData(const FTextureInfo& Info, DWORD PolyFlags,DWORD customPolyFlags=0);
	
public:
	TexConverter(TextureCache *textureCache);
	void convertAndCache(FTextureInfo& Info, DWORD PolyFlags) const;
	void update(FTextureInfo& Info,DWORD PolyFlags) const;

};