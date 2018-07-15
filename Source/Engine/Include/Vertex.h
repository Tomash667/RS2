#pragma once

//-----------------------------------------------------------------------------
struct SimpleVertex
{
	Vec3 pos;
	Vec2 tex;
};

//-----------------------------------------------------------------------------
struct ColorVertex
{
	Vec3 pos;
	Vec4 color;
};

//-----------------------------------------------------------------------------
struct Vertex
{
	Vec3 pos;
	Vec3 normal;
	Vec2 tex;

	Vertex() {}
	Vertex(const Vec3& pos, const Vec3& normal, const Vec2& tex) : pos(pos), normal(normal), tex(tex) {}
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
