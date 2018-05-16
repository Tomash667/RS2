#pragma once

//-----------------------------------------------------------------------------
struct Vertex
{
	Vec3 pos;
	Vec3 normal;
	Vec2 tex;
};

//-----------------------------------------------------------------------------
struct AniVertex
{
	Vec3 pos;
	float weights;
	uint indices;
	Vec3 normal;
	Vec2 tex;
};

//-----------------------------------------------------------------------------
struct GuiVertex
{
	Vec2 pos;
	Vec2 tex;
	Vec4 color;
};

//-----------------------------------------------------------------------------
struct ParticleVertex
{
	Vec3 pos;
	Vec2 tex;
	Vec4 color;
};
