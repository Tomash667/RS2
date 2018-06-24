#include "EngineCore.h"
#include "QmshLoader.h"
#include "Mesh.h"
#include "MeshBuilder.h"
#include "Vertex.h"
#include "ResourceManager.h"
#include <d3d11_1.h>

QmshLoader::QmshLoader(ResourceManager* res_mgr, ID3D11Device* device, ID3D11DeviceContext* device_context)
	: res_mgr(res_mgr), device(device), device_context(device_context)
{
}

Mesh* QmshLoader::Load(cstring name, cstring path, bool raw)
{
	Mesh* mesh = new Mesh(name);

	cstring dir_part = strrchr(name, '/');
	if(!dir_part)
		dir.clear();
	else
		dir = string(name, dir_part - name + 1);

	try
	{
		FileReader f(path);
		LoadInternal(*mesh, f, raw);
	}
	catch(cstring err)
	{
		delete mesh;
		throw Format("Failed to load mesh '%s': %s", path, err);
	}

	return mesh;
}

void QmshLoader::LoadInternal(Mesh& mesh, FileReader& f, bool raw)
{
	if(!f)
		throw "Failed to open file.";

	// load header
	f.Read(mesh.head);
	if(!f)
		throw "Failed to read mesh header.";
	if(memcmp(mesh.head.format, "QMSH", 4) != 0)
		throw Format("Invalid file signature '%.4s'.", mesh.head.format);
	if(mesh.head.version < 12 || mesh.head.version > 20)
		throw Format("Invalid mesh version '%u'.", mesh.head.version);
	if(mesh.head.version < 20)
		throw Format("Unsupported mesh version '%u'.", mesh.head.version);
	if(mesh.head.n_bones >= 32)
		throw Format("Too many bones (%u).", mesh.head.n_bones);
	if(mesh.head.n_subs == 0)
		throw "Missing model mesh.";
	if(IS_SET(mesh.head.flags, Mesh::F_ANIMATED) && !IS_SET(mesh.head.flags, Mesh::F_USE_PARENT_BONES))
	{
		if(mesh.head.n_bones == 0)
			throw "No bones.";
		if(mesh.head.n_groups == 0)
			throw "No bone groups.";
	}
	if(IS_SET(mesh.head.flags, Mesh::F_TANGENTS | Mesh::F_SPLIT))
		throw Format("Not implemented mesh flags used (%u).", mesh.head.flags);

	// set vertex size, layout
	uint vertex_size;
	if(IS_SET(mesh.head.flags, Mesh::F_PHYSICS))
	{
		vertex_size = sizeof(Vec3);
		mesh.layout = Mesh::VERTEX_POS;
	}
	else if(IS_SET(mesh.head.flags, Mesh::F_ANIMATED))
	{
		vertex_size = sizeof(AniVertex);
		mesh.layout = Mesh::VERTEX_ANIMATED;
	}
	else
	{
		vertex_size = sizeof(Vertex);
		mesh.layout = Mesh::VERTEX_NORMAL;
	}

	// vertex buffer
	uint size = vertex_size * mesh.head.n_verts;
	if(!f.Ensure(size))
		throw "Failed to read vertex data.";

	if(raw)
	{
		mesh.vertex_data.resize(size);
		f.Read(mesh.vertex_data.data(), size);
	}
	else
	{
		buf.resize(size);
		f.Read(buf.data(), size);

		D3D11_BUFFER_DESC v_desc;
		v_desc.Usage = D3D11_USAGE_DEFAULT;
		v_desc.ByteWidth = size;
		v_desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		v_desc.CPUAccessFlags = 0;
		v_desc.MiscFlags = 0;
		v_desc.StructureByteStride = 0;

		D3D11_SUBRESOURCE_DATA v_data;
		v_data.pSysMem = buf.data();

		HRESULT result = device->CreateBuffer(&v_desc, &v_data, &mesh.vb);
		if(FAILED(result))
			throw Format("Failed to create vertex buffer (%u).", result);
	}

	// index buffer
	size = sizeof(word) * mesh.head.n_tris * 3;
	if(!f.Ensure(size))
		throw "Failed to read index data.";

	if(raw)
	{
		mesh.index_data.resize(size / 2);
		f.Read(mesh.index_data.data(), size);
	}
	else
	{
		buf.resize(size);
		f.Read(buf.data(), size);

		D3D11_BUFFER_DESC v_desc;
		v_desc.Usage = D3D11_USAGE_DEFAULT;
		v_desc.ByteWidth = size;
		v_desc.BindFlags = D3D11_BIND_INDEX_BUFFER;
		v_desc.CPUAccessFlags = 0;
		v_desc.MiscFlags = 0;
		v_desc.StructureByteStride = 0;

		D3D11_SUBRESOURCE_DATA v_data;
		v_data.pSysMem = buf.data();

		HRESULT result = device->CreateBuffer(&v_desc, &v_data, &mesh.ib);
		if(FAILED(result))
			throw Format("Failed to create index buffer (%u).", result);
	}

	// submeshes
	string filename;
	if(!f.Ensure(mesh.head.n_subs, Mesh::Submesh::MIN_SIZE))
		throw "Failed to read submesh data.";
	mesh.subs.resize(mesh.head.n_subs);
	uint index = 0;
	for(auto& sub : mesh.subs)
	{
		f >> sub.first;
		f >> sub.tris;
		f >> sub.min_ind;
		f.Skip<word>();
		f >> sub.name;
		f >> filename;

		if(!filename.empty())
			sub.tex = res_mgr->GetTexture(Format("%s%s", dir.c_str(), filename.c_str()));
		else
			sub.tex = nullptr;

		// specular value
		f >> sub.specular_color;
		f >> sub.specular_intensity;
		f >> sub.specular_hardness;

		// normalmap
		if(IS_SET(mesh.head.flags, Mesh::F_TANGENTS))
		{
			f >> filename;
			if(!filename.empty())
			{
				sub.tex_normal = res_mgr->GetTexture(Format("%s%s", dir.c_str(), filename.c_str()));
				f >> sub.normal_factor;
			}
			else
				sub.tex_normal = nullptr;
		}
		else
			sub.tex_normal = nullptr;

		// specular map
		f >> filename;
		if(!filename.empty())
		{
			sub.tex_specular = res_mgr->GetTexture(Format("%s%s", dir.c_str(), filename.c_str()));
			f >> sub.specular_factor;
			f >> sub.specular_color_factor;
		}
		else
			sub.tex_specular = nullptr;

		if(!f)
			throw Format("Failed to read submesh %u.", index);

		++index;
	}

	// animation data
	if(IS_SET(mesh.head.flags, Mesh::F_ANIMATED) && !IS_SET(mesh.head.flags, Mesh::F_USE_PARENT_BONES))
	{
		// bones
		size = Mesh::Bone::MIN_SIZE * mesh.head.n_bones;
		if(!f.Ensure(size))
			throw "Failed to read bones.";
		mesh.bones.resize(mesh.head.n_bones + 1);

		// zero bone
		Mesh::Bone& zero_bone = mesh.bones[0];
		zero_bone.parent = 0;
		zero_bone.name = "zero";
		zero_bone.id = 0;
		zero_bone.mat = Matrix::IdentityMatrix;

		for(byte i = 1; i <= mesh.head.n_bones; ++i)
		{
			Mesh::Bone& bone = mesh.bones[i];

			bone.id = i;
			f.Read(bone.parent);

			f.Read(bone.mat._11);
			f.Read(bone.mat._12);
			f.Read(bone.mat._13);
			bone.mat._14 = 0;
			f.Read(bone.mat._21);
			f.Read(bone.mat._22);
			f.Read(bone.mat._23);
			bone.mat._24 = 0;
			f.Read(bone.mat._31);
			f.Read(bone.mat._32);
			f.Read(bone.mat._33);
			bone.mat._34 = 0;
			f.Read(bone.mat._41);
			f.Read(bone.mat._42);
			f.Read(bone.mat._43);
			bone.mat._44 = 1;

			f.Read(bone.name);

			mesh.bones[bone.parent].childs.push_back(i);
		}

		// add zero bone to counter
		uint real_bones = mesh.head.n_bones;
		mesh.head.n_bones++;

		if(!f)
			throw "Failed to read bones data.";

		// animations
		size = Mesh::Animation::MIN_SIZE * mesh.head.n_anims;
		if(!f.Ensure(size))
			throw "Failed to read animations.";
		mesh.anims.resize(mesh.head.n_anims);

		for(byte i = 0; i < mesh.head.n_anims; ++i)
		{
			Mesh::Animation& anim = mesh.anims[i];

			f.Read(anim.name);
			f.Read(anim.length);
			f.Read(anim.n_frames);

			size = anim.n_frames * (4 + sizeof(Mesh::KeyframeBone) * real_bones);
			if(!f.Ensure(size))
				throw Format("Failed to read animation %u data.", i);

			anim.frames.resize(anim.n_frames);

			for(word j = 0; j < anim.n_frames; ++j)
			{
				f.Read(anim.frames[j].time);
				anim.frames[j].bones.resize(real_bones);
				f.Read(anim.frames[j].bones.data(), sizeof(Mesh::KeyframeBone) * real_bones);
			}
		}
	}

	// load points
	size = Mesh::Point::MIN_SIZE * mesh.head.n_points;
	if(!f.Ensure(size))
		throw "Failed to read points.";
	mesh.attach_points.clear();
	mesh.attach_points.resize(mesh.head.n_points);
	for(word i = 0; i < mesh.head.n_points; ++i)
	{
		Mesh::Point& p = mesh.attach_points[i];

		f.Read(p.name);
		f.Read(p.mat);
		f.Read(p.bone);
		f.Read(p.type);
		f.Read(p.size);
		f.Read(p.rot);
	}

	// bone groups
	if(IS_SET(mesh.head.flags, Mesh::F_ANIMATED) && !IS_SET(mesh.head.flags, Mesh::F_USE_PARENT_BONES))
	{
		if(!f.Ensure(Mesh::BoneGroup::MIN_SIZE * mesh.head.n_groups))
			throw "Failed to read bone groups.";
		mesh.groups.resize(mesh.head.n_groups);
		for(word i = 0; i < mesh.head.n_groups; ++i)
		{
			Mesh::BoneGroup& gr = mesh.groups[i];

			f.Read(gr.name);

			// parent group
			f.Read(gr.parent);
			assert(gr.parent < mesh.head.n_groups);
			assert(gr.parent != i || i == 0);

			// bone indexes
			byte count;
			f.Read(count);
			gr.bones.resize(count);
			f.Read(gr.bones.data(), (uint)gr.bones.size());
		}

		if(!f)
			throw "Failed to read bone groups data.";

		mesh.SetupBoneMatrices();
	}
}

Mesh* QmshLoader::Create(MeshBuilder* mesh_builder)
{
	assert(mesh_builder);
	Mesh* mesh = new Mesh("");
	try
	{
		CreateInternal(*mesh, *mesh_builder);
		return mesh;
	}
	catch(cstring err)
	{
		delete mesh;
		throw Format("Failed to create mesh: %s", err);
	}
}

void QmshLoader::CreateInternal(Mesh& mesh, MeshBuilder& builder)
{
	mesh.head.flags = 0;

	// vb
	uint vertex_size = sizeof(Vertex);
	uint size = vertex_size * builder.vertices.size();

	D3D11_BUFFER_DESC v_desc;
	v_desc.Usage = D3D11_USAGE_DEFAULT;
	v_desc.ByteWidth = size;
	v_desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	v_desc.CPUAccessFlags = 0;
	v_desc.MiscFlags = 0;
	v_desc.StructureByteStride = 0;

	D3D11_SUBRESOURCE_DATA v_data;
	v_data.pSysMem = builder.vertices.data();

	HRESULT result = device->CreateBuffer(&v_desc, &v_data, &mesh.vb);
	if(FAILED(result))
		throw Format("Failed to create vertex buffer (%u).", result);

	// ib
	size = sizeof(word) * builder.indices.size();

	v_desc.ByteWidth = size;
	v_desc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	v_data.pSysMem = builder.indices.data();

	result = device->CreateBuffer(&v_desc, &v_data, &mesh.ib);
	if(FAILED(result))
		throw Format("Failed to create index buffer (%u).", result);

	// subs
	mesh.subs.resize(builder.subs.size());
	for(uint i = 0, count = builder.subs.size(); i < count; ++i)
	{
		Mesh::Submesh& sub = mesh.subs[i];
		MeshBuilder::Submesh& sub_info = builder.subs[i];
		sub.first = sub_info.first;
		sub.tris = sub_info.tris;
		sub.min_ind = 0;
		sub.tex = sub_info.tex;
		sub.tex_normal = nullptr;
		sub.tex_specular = nullptr;
		// default values from blender TODO
		/*f >> sub.specular_color;
		f >> sub.specular_intensity;
		f >> sub.specular_hardness;*/
	}
}
