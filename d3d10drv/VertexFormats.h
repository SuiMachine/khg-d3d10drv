#pragma once


	/** 2 float vector */
	struct Vec2
	{
		float x,y;
	};

	/** 3 float vector */
	struct Vec3
	{
		float x,y,z;
	};

	/** 4 float vector */
	struct Vec4
	{
		Vec4(float x, float y, float z, float w)
		:x(x),y(y),z(z),w(w)
		{

		}
		float x,y,z,w;
	};

	/** 4 byte vector */
	struct Vec4_byte
	{
		BYTE x,y,z,w;
	};

	struct Vec2_int
	{
		int x,y;
	};
	


struct Vertex_GouraudPolygon
{
	Vec3 Pos;
	Vec4 Color;
	Vec4 Fog;
	Vec2 TexCoord;
	DWORD flags;
};

struct Vertex_ComplexSurface
{
	Vec3 Pos;
	Vec2 TexCoord[5];
	DWORD flags;
};

struct Vertex_Tile
{
	Vec4 XYWH;
	Vec4 UVWH;
	Vec4 Color;
	float z;
	DWORD flags;
};

struct Vertex_FogSurface
{
	Vec3 Pos;
	Vec4 Color;
	DWORD flags;
	char padding[24]; //So it can be used in a buffer together with the others
};

struct Vertex_Simple
{
	Vec3 Pos;
	//Vec2 Tex;
};