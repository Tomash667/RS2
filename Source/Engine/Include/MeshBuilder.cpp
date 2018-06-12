#include "EngineCore.h"
#include "MeshBuilder.h"
#include "Mesh.h"

void MeshBuilder::Append(Mesh* mesh, const Matrix& matrix)
{
	assert(mesh && !mesh->vertex_data.empty());
	assert(mesh->layout == Mesh::VERTEX_NORMAL); // YAGNI

	const uint vertex_size = sizeof(Vertex);

	// copy vertex data
	Vertex* v = (Vertex*)mesh->vertex_data.data();
	uint vertex_offset = vertices.size();
	vertices.resize(vertex_offset + mesh->head.n_verts);
	for(uint i = 0; i < mesh->head.n_verts; ++i)
	{
		Vertex& nv = vertices[i + vertex_offset];
		Vertex& ov = v[i];
		nv.pos = Vec3::Transform(ov.pos, matrix);
		nv.normal = Vec3::TransformNormal(ov.normal, matrix);
		nv.tex = ov.tex;
	}

	for(Mesh::Submesh& sub : mesh->subs)
	{
		// get existing or add submesh
		uint sub_index = 0;
		for(Submesh& sub_info : subs)
		{
			if(sub_info.tex == sub.tex)
				break;
			++sub_index;
		}
		if(sub_index == subs.size())
		{
			subs.resize(subs.size() + 1);
			Submesh& sub_info = subs.back();
			sub_info.tex = sub.tex;
		}

		// copy indices
		Submesh& sub_info = subs[sub_index];
		sub_info.indices.reserve(index_offset + sub.tris * 3);
		for(uint i = 0; i < sub.tris * 3u; ++i)
			sub_info.indices.push_back(mesh->index_data[i + sub.first * 3] + index_offset);
	}
	index_offset += mesh->head.n_verts;
}

void MeshBuilder::Clear()
{
	vertices.clear();
	indices.clear();
	subs.clear();
	index_offset = 0;
}

void MeshBuilder::GenerateIndices()
{
	assert(indices.empty());
	uint verts = vertices.size();
	assert(verts % 4 == 0);
	uint faces = verts / 4;
	uint index = 0;
	for(uint i = 0; i < faces; ++i)
	{
		indices.push_back(index);
		indices.push_back(index + 1);
		indices.push_back(index + 2);
		indices.push_back(index + 2);
		indices.push_back(index + 1);
		indices.push_back(index + 3);
		index += 4;
	}
}

void MeshBuilder::JoinIndices()
{
	assert(indices.empty());
	uint total = 0, offset = 0;
	for(Submesh& sub : subs)
	{
		sub.first = offset;
		sub.tris = sub.indices.size() / 3;
		total += sub.indices.size();
		offset += sub.indices.size() / 3;
	}

	indices.resize(total);
	for(Submesh& sub : subs)
		memcpy(indices.data() + sub.first * 3, sub.indices.data(), sub.indices.size() * sizeof(word));
}
