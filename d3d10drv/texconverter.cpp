/**
\class TexConverter
Functions to convert from Unreal format textures to standard R8G8B8A8 Direct3D 10 format inital data.
Uses both Unreal and Direct3D datatypes, but no access to D3D objects like the device etc.

The game has two types of textures: static ones and dynamic. Dynamic texures are parametric ones such as water, etc.
After trying multiple methods, it was determined best to create static textures as USAGE_IMMUTABLE, and dynamic ones as USAGE_DEFAULT (i.e. not USAGE_DYNAMIC).
USAGE_DEFAULT textures cannot be mapped, but they are updated using a copy operation. A nice thing about this is that it allows the texture handling to be streamlined;
it used to have seperate paths for map()-able and immutable textures.
However copy-able and immutable textures can be handled about the same way as they both are created from initial data, instead of being filled after their creation.

Some texture types can be used by D3D without conversion; depending on the type (see formats array below) direct assignments can take place.

New textures are created by having the D3D class create a texture out of their converted/assigned mips stored as D3D_SUBRESOURCE_DATA;
the texture conversion function sets the TEXTURE_2D_DESC parameters for this depending on the texture size, if it is dynamic, etc.
Existing textures are updated by passing a new mip to the D3D class; only the 0th mip is updated, which should be fine (afaik there's no dynamic textures with >1 mips).

Texture conversion functions write to a void pointer so they can work unmodified regardless of the underlying memory.

Additional notes:
- Textures can be updated while a frame is being drawn (i.e. between lock() and unlock()). This means that a texture can even need to be be updated between two successive drawXXXX() calls.
- Textures haven't always the correct 'masked' flag upon initial caching. as such, they must sometimes be replaced if the game later tries to load it with the flag.
	As this cannot be detected in advance, they're created as immutable. Updating is done by deleting and recreating.
- For example dynamic lights have neither bParametric nor bRealtime set. Fortunately, these seem to have bRealtimechanged set initially.
- BRGA7 textures have garbage data outside their UClamp and reading outside the VClamp can lead to access violations. To be able to still direct assign them,
all textures are made only as large as the UClamp*VClamp and the texture coordinates are scaled to reflect this. Furthermore, the D3D_SUBRESOURCE_DATA's stride
parameter is set so the data outside the UClamp is skipped.

Override/extra textures:
Textures can be overridden by external .dds files. Additionally, extra layers (bump, detail) can be provided even if the texture didn't come with these. 
These are stored in the texture's extraExternalTextures array. When a diffuse texture is applied, the extra textures set here will be used instead of the bump/detail layers
provided by the game (if any). Implementation wise this means that override textures replace the originals in the texture cache. However, extra textures are linked to their parent,
so the originals, if there are any, are still present in the cache (as these can be used by other textures).

Only real textures, not lightmaps, fogmaps etc. can be overridden. Every overrideable texture type can have extra textures assigned. However, the renderer chooses when to 
apply these, which only makes sense for diffuse textures.

Finally, each override texture can have custom polyflags in a file, these are loaded and ORed by the renderer with the flags provided to draw calls. Especially useful to force alpha blending instead of masking.
*/
#include <stdio.h>
#include <new>
#include <D3dx10.h>
#include "TexConverter.h"
#include "polyflags.h"
#include <fstream>

/**
Mappings from Unreal to our texture info
*/
TexConverter::TextureFormat TexConverter::formats[] = 
{
	{true,0,0,false,DXGI_FORMAT_R8G8B8A8_UNORM,&TexConverter::fromPaletted},		/**< TEXF_P8 = 0x00 */
	{true,0,0,true,DXGI_FORMAT_R8G8B8A8_UNORM,nullptr},								/**< TEXF_RGBA7	= 0x01 */
	{false,0,0,true,DXGI_FORMAT_R8G8B8A8_UNORM,nullptr},								/**< TEXF_RGB16	= 0x02 */
	{true,4,8,true,DXGI_FORMAT_BC1_UNORM,nullptr},									/**< TEXF_DXT1 = 0x03 */
	{false,0,0,true,DXGI_FORMAT_UNKNOWN,nullptr},									/**< TEXF_RGB8 = 0x04 */
	{true,0,0,true,DXGI_FORMAT_R8G8B8A8_UNORM,nullptr},								/**< TEXF_RGBA8	= 0x05 */
};

/**
Build metadata from Unreal info
*/
TextureCache::TextureMetaData TexConverter::buildMetaData(const FTextureInfo &Info, DWORD PolyFlags,DWORD customPolyFlags)
{
	TextureCache::TextureMetaData metadata;
	metadata.multU = 1.0 / (Info.UScale * Info.UClamp);
	metadata.multV = 1.0 / (Info.VScale * Info.VClamp);
	metadata.masked = (PolyFlags & PF_Masked)!=0;
	metadata.customPolyFlags = customPolyFlags;
	for(int i=0;i<TextureCache::DUMMY_NUM_EXTERNAL_TEXTURES;i++)
	{
		metadata.externalTextures[i]=nullptr;
	}
	return metadata;
}

TexConverter::TexConverter(TextureCache *textureCache)
{
	this->textureCache = textureCache;
}

/**
Fill texture info structure and execute proper conversion of pixel data.

\param Info Unreal texture information, includes cache id, size information, texture data.
\param PolyFlags Polyflags, see polyflags.h.
*/
void TexConverter::convertAndCache(FTextureInfo& Info,DWORD PolyFlags) const
{

	
	if(Info.Format > TEXF_RGBA8)
	{
		UD3D10RenderDevice::debugs("Unknown texture type.");
		return;
	}
	
	TextureFormat &format=formats[Info.Format];
	if(format.supported == false)
	{
		UD3D10RenderDevice::debugs("Unsupported texture type.");
		return;
	}

	//Unreal 1 S3TC texture fix: if texture info size doesn't match mip size (happens for some textures for some reason), scale up clamp (which is what we use for the size)
	if(Info.USize != Info.Mips[0]->USize)
	{
		float scale = (float)Info.Mips[0]->USize/Info.USize;
		Info.USize = Info.Mips[0]->USize; //dont use this but just to be sure
		Info.UClamp *= scale;
		Info.UScale /= scale;
	}
	if(Info.VSize != Info.Mips[0]->VSize)
	{
		float scale = (float)Info.Mips[0]->VSize/Info.VSize;
		Info.VSize = Info.Mips[0]->VSize;
		Info.VClamp *= scale;
		Info.VScale /= scale;
	}

	//Set texture info. These parameters are the same for each usage of the texture.
	TextureCache::TextureMetaData metadata= buildMetaData(Info,PolyFlags);	
	//Mult is a multiplier (so division is only done once here instead of when texture is applied) to normalize texture coordinates.
	//metadata.width = Info.USize;
	//metadata.height = Info.VSize;	
	//metadata.multU = 1.0 / (Info.UScale * Info.USize);
	//metadata.multV = 1.0 / (Info.VScale * Info.VSize);
	

	CLAMP(Info.NumMips,0,MAX_MIPS); //Some third party s3tc textures report more mips than the info structure fits	

	//Convert each mip level
	D3D10_SUBRESOURCE_DATA* data = new (std::nothrow) D3D10_SUBRESOURCE_DATA[Info.NumMips];
	if(data == nullptr)
	{
		return;
	}
	for(int i=0;i<Info.NumMips;i++)
	{
		convertMip(Info,format,PolyFlags,i,data[i]);
	}

	//Create a texture from the converted data
	bool dynamic = ((Info.TextureFlags & TF_RealtimeChanged || Info.TextureFlags & TF_Realtime || Info.TextureFlags & TF_Parametric) != 0);

	D3D10_TEXTURE2D_DESC desc;
	desc.BindFlags = D3D10_BIND_SHADER_RESOURCE;
	desc.ArraySize = 1;
	desc.Height = Info.VClamp;
	desc.Width = Info.UClamp;
	desc.MipLevels = Info.NumMips;
	desc.MiscFlags = 0;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;	
	desc.Format = format.d3dFormat;
	
	if(dynamic)
	{
		desc.Usage = D3D10_USAGE_DYNAMIC;
		desc.CPUAccessFlags = D3D10_CPU_ACCESS_WRITE;
	}
	else
	{
		desc.Usage = D3D10_USAGE_IMMUTABLE;
		desc.CPUAccessFlags = 0;
	}
	if(format.blocksize>0) //Compressed textures should be a whole amount of blocks
	{
		desc.Width += Info.USize%format.blocksize;
		desc.Height += Info.VSize%format.blocksize;
	}

	ID3D10Texture2D* texture = textureCache->createTexture(desc,*data);
	if(texture==nullptr)
	{
		//_putws(Info.Texture->GetFullName());
		return;
	}

	textureCache->cacheTexture(Info.CacheID,metadata,texture);

	//Delete temporary data
	if(!format.directAssign)
	{
		for(int i=0;i<Info.NumMips;i++)
		{
			delete [] data[i].pSysMem;
		}		
	}
	delete [] data;
	SAFE_RELEASE(texture);
}

/**
Update a dynamic texture by converting its 0th mip and letting D3D update it.
*/
void TexConverter::update(FTextureInfo& Info,DWORD PolyFlags) const
{	
	D3D10_SUBRESOURCE_DATA data;
	//Info.bRealtimeChanged=0; //Clear this flag (from other renderes)
	TextureFormat format = formats[Info.Format];
	convertMip(Info,format,PolyFlags,0,data);
	textureCache->updateMip(Info,0,data);
	if(!format.directAssign)
		delete [] data.pSysMem;
}

/**
Fills a D3D10_SUBRESOURCE_DATA structure with converted texture data for a mipmap; if possible, assigns instead of converts.
\param Info Unreal texture info.
\param format Conversion parameters for the texture.
\param PolyFlags Polyflags. See polyflags.h.
\param mipLevel Which mip to convert.
\param data Direct3D 10 structure which will be filled.

\note Caller must free data.pSysMem for non-directAssign textures.
*/
void TexConverter::convertMip(const FTextureInfo& Info,const TextureFormat &format,DWORD PolyFlags,int mipLevel, D3D10_SUBRESOURCE_DATA &data)
{	
	//Set stride
	if(format.blocksize>0)
	{	
		data.SysMemPitch=max(Info.Mips[mipLevel]->USize,format.blocksize)*format.pixelsPerBlock/format.blocksize; //Max() as each mip is at least block sized in each direction		
	}
	else
	{
		data.SysMemPitch=Info.Mips[mipLevel]->USize*sizeof(DWORD); //Pitch is set so garbage data outside of UClamp is skipped
	}

	//Assign or convert
	if(format.directAssign) //Direct assignment from Unreal to our texture is possible
	{
		data.pSysMem = Info.Mips[mipLevel]->DataPtr;		
	}
	else //Texture needs to be converted via temporary data; allocate it
	{
		data.pSysMem = new (std::nothrow) DWORD[Info.Mips[mipLevel]->USize*max((Info.VClamp>>mipLevel),1)]; //max(...) as otherwise USize*0 can occur
		if(data.pSysMem==nullptr)
		{
			UD3D10RenderDevice::debugs("Convert: Error allocating texture initial data memory.");
			return;
		}				
		//Convert
		format.conversionFunc(Info,PolyFlags,(void*)data.pSysMem,mipLevel);
	}
}

/**
Convert from palleted 8bpp to r8g8b8a8.
*/
// Globals.
void TexConverter::fromPaletted(const FTextureInfo& Info,DWORD PolyFlags, void *target,int mipLevel)
{
	//If texture is masked with palette index 0 = transparent; make that index black w. alpha 0 (black looks best for the border that gets left after masking)
	if(PolyFlags & PF_Masked )
	{
		*(DWORD*)(&(Info.Palette->R)) = (DWORD) 0;
	}

	DWORD *dest = (DWORD*) target;
	BYTE *source = (BYTE*) Info.Mips[mipLevel]->DataPtr;
	BYTE *sourceEnd = source + Info.Mips[mipLevel]->USize*Info.Mips[mipLevel]->VSize;

	while(source<sourceEnd)
	{
		auto palletedPixel = Info.Palette[*source];
		auto maxColor = Info.MaxColor[*source];
		palletedPixel.A = palletedPixel.A != 0 || palletedPixel.R > 0 || palletedPixel.G > 0 || palletedPixel.B > 0 || (maxColor.R == 0 && maxColor.G == 0 && maxColor.B == 0 && maxColor.A == 0) ? 128 : 0;

		//palletedPixel.A = 255;
		*dest=*(DWORD*)&(palletedPixel);
		//auto cast = reinterpret_cast<byte*>(dest);
		//cast[3] = 20;

		source++;
		dest++;
	}
}

/**
BGRA7 to RGBA8. Used for lightmaps and fog. Straightforward, just multiply by 2.
\note IMPORTANT these textures do not have valid data outside of their U/VClamp; there's garbage outside UClamp and reading it outside VClamp sometimes results in access violations.
Unfortunately this means a direct assignment is not possible as we need to manually repeat the rows/columns outside of the clamping range.
\note This format is only used for fog and lightmap; it is also the only format used for those. As such, we can at least do the swizzling and scaling in-shader and use memcpy() here.
\deprecated Direct assignment instead, see text at top of file.
*/
void TexConverter::fromBGRA7(const FTextureInfo& Info,DWORD PolyFlags,void *target,int mipLevel)
{/*
	unsigned int VClamp = Info.VClamp>>mipLevel;
	unsigned int UClamp = Info.UClamp>>mipLevel;
	unsigned int USize = Info.Mips[mipLevel]->USize;
	unsigned int VSize = Info.Mips[mipLevel]->VSize;
	DWORD* src =(DWORD*) Info.Mips[mipLevel]->DataPtr;
	DWORD* dst = (DWORD*) target;

	unsigned int row;
	//Copy rows up to VClamp
	for(row=0;row<VClamp;row++)
	{
		memcpy(dst,src,UClamp*sizeof(DWORD)); //Copy valid part
		for(unsigned int col=UClamp;col<USize;col++) //Repeat last pixel as padding
		{
			dst[col]=dst[UClamp-1];
		}
		//Go to next row
		src+=USize;
		dst+=USize;
	}
	
	//Outside row clamp, create copy of last valid row
	for(DWORD* dst2=dst;row<VSize;row++)
	{	
		memcpy(dst2,dst-USize,USize*sizeof(DWORD));
		dst2+=USize;
	}*/
}
