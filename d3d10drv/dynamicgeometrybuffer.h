#pragma once

#include "geometrybuffer.h"


class DynamicGeometryBuffer: public GeometryBuffer
{
private:
	bool clear;
	unsigned int numVerts; //Number of buffered verts	
	void *mappedVBuffer; //Memmapped version of vertex buffer
	int *mappedIBuffer; //Memmapped version of index buffer
	size_t size; //Maximum size of buffer
	void map();
	bool unmap();	

public:
	DynamicGeometryBuffer(ID3D10Device* device);

	//From GeometryBuffer
	bool create(size_t size, size_t vertexSize);
	void draw() override;
	void newFrame() override;

	void indexTriangleFan(int num);
	void indexSingleVertex();
	void* getVertex();	
};