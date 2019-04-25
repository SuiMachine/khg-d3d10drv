#include "dynamicgeometrybuffer.h"
#include "d3d10drv.h"

DynamicGeometryBuffer::DynamicGeometryBuffer(ID3D10Device* device) : 
GeometryBuffer(device),
mappedIBuffer(nullptr),
mappedVBuffer(nullptr)
{
	
}

bool DynamicGeometryBuffer::create(size_t size, size_t vertexSize)
{
	this->size = size;

	D3D10_BUFFER_DESC vertexBufferDesc;
    vertexBufferDesc.Usage            = D3D10_USAGE_DYNAMIC;
    vertexBufferDesc.ByteWidth        = vertexSize*size;
    vertexBufferDesc.BindFlags        = D3D10_BIND_VERTEX_BUFFER;
    vertexBufferDesc.CPUAccessFlags   = D3D10_CPU_ACCESS_WRITE;
    vertexBufferDesc.MiscFlags        = 0;

	D3D10_BUFFER_DESC indexBufferDesc;
    indexBufferDesc.Usage            = D3D10_USAGE_DYNAMIC;
    indexBufferDesc.ByteWidth        = sizeof(int)*size;
    indexBufferDesc.BindFlags        = D3D10_BIND_INDEX_BUFFER;
    indexBufferDesc.CPUAccessFlags   = D3D10_CPU_ACCESS_WRITE;
    indexBufferDesc.MiscFlags        = 0;

	return GeometryBuffer::create(0,vertexSize,&vertexBufferDesc,&indexBufferDesc,nullptr,nullptr);
}

void DynamicGeometryBuffer::map()
{
	HRESULT hr, hr2;
	if(mappedIBuffer!=nullptr||mappedVBuffer!=nullptr)
	{
		//UD3D10RenderDevice::debugs("map() without unmap");
		return;
	}

	D3D10_MAP m;
	if(clear)
	{
		numVerts=0;
		numIndices=0;
		numUndrawnIndices=0;
		m = D3D10_MAP_WRITE_DISCARD;
		clear=false;

	}
	else
	{
		m = D3D10_MAP_WRITE_NO_OVERWRITE;
	}
	

	hr = vertexBuffer->Map(m,0,(void**)&mappedVBuffer);
	hr2 = indexBuffer->Map(m,0,(void**)&mappedIBuffer);
	if(FAILED(hr) || FAILED(hr2))
	{
		UD3D10RenderDevice::debugs("Failed to map index and/or vertex buffer.");
	}
}

bool DynamicGeometryBuffer::unmap()
{
	if(mappedVBuffer==nullptr || mappedIBuffer == nullptr) //No buffer mapped, do nothing
	{
		return 0;
	}
	vertexBuffer->Unmap();
	mappedVBuffer=nullptr;
	indexBuffer->Unmap();
	mappedIBuffer=nullptr;

	return 1;
}

void DynamicGeometryBuffer::indexTriangleFan(int num)
{		
	//Make sure there's index and vertex buffer room for a triangle fan; if not, the current buffer content is drawn and discarded
	//Index buffer is checked only, as there's equal or more indices than vertices. There.s 3*(n-1) indices for n vertices.
	int newIndices = (num-2)*3;
	
	if(numIndices+newIndices>size)
	{
		D3D::render();
		clear=true;
		map();	
	}
	if(mappedIBuffer==nullptr)
		map();

	//Generate fan indices	
	for(int i=1;i<num-1;i++)
	{
		mappedIBuffer[numIndices++] = numVerts; //Center point
		mappedIBuffer[numIndices++] = numVerts+i;
		mappedIBuffer[numIndices++] = numVerts+i+1;		
	}

	numUndrawnIndices += newIndices;
}

void DynamicGeometryBuffer::indexSingleVertex()
{
	int newIndices = 1;
	
	if(numIndices+newIndices>size)
	{
		draw();
		clear=true;
		map();	
	}
	if(mappedIBuffer==nullptr)
		map();

	mappedIBuffer[numIndices++] = numVerts;

	numUndrawnIndices += newIndices;
}

void *DynamicGeometryBuffer::getVertex()
{	
	return (void*) ((char*) (mappedVBuffer)+stride*numVerts++);
}

void DynamicGeometryBuffer::draw()
{	
	if(mappedVBuffer==nullptr || mappedIBuffer == nullptr || numUndrawnIndices==0)
		return;
	unmap();
	device->DrawIndexed(numUndrawnIndices,numIndices-numUndrawnIndices,0);
	numUndrawnIndices=0;
}

void DynamicGeometryBuffer::newFrame()
{
	clear=true;
}

