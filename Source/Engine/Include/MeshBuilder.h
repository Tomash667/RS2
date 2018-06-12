#pragma once

#include "Vertex.h"

struct MeshBuilder
{
	struct Submesh
	{
		Texture* tex;
		vector<word> indices;
		uint tris, first;
	};

	void Append(Mesh* mesh, const Matrix& matrix);
	void Clear();
	void GenerateIndices();
	void JoinIndices();

	vector<Vertex> vertices;
	vector<word> indices;
	vector<Submesh> subs;
	uint index_offset;
};
