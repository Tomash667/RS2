#include "EngineCore.h"
#include "QmshLoader.h"
#include "Mesh.h"
#include "Vertex.h"
#include "ResourceManager.h"
#include <d3d11_1.h>

QmshLoader::QmshLoader(ResourceManager* res_mgr, ID3D11Device* device, ID3D11DeviceContext* device_context)
	: res_mgr(res_mgr), device(device), device_context(device_context)
{
}

Mesh* QmshLoader::Load(cstring name, cstring path)
{
	Mesh* mesh = new Mesh(name);

	try
	{
		FileReader f(path);
		LoadInternal(*mesh, f);
	}
	catch(cstring err)
	{
		delete mesh;
		throw Format("Failed to load mesh '%s': %s", path, err);
	}

	return mesh;
}

void QmshLoader::LoadInternal(Mesh& mesh, FileReader& f)
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
	if(IS_SET(mesh.head.flags, Mesh::F_PHYSICS | Mesh::F_TANGENTS || Mesh::F_SPLIT))
		throw Format("Not implemented mesh flags used (%u).", mesh.head.flags);

	// set vertex size
	uint vertex_size;
	if(IS_SET(mesh.head.flags, Mesh::F_ANIMATED))
		vertex_size = sizeof(AniVertex);
	else
		vertex_size = sizeof(Vertex);

	// vertex buffer
	uint size = vertex_size * mesh.head.n_verts;
	if(!f.Ensure(size))
		throw "Failed to read vertex data.";

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

	// index buffer
	size = sizeof(word) * mesh.head.n_tris * 3;
	if(!f.Ensure(size))
		throw "Failed to read index data.";

	buf.resize(size);
	f.Read(buf.data(), size);

	v_desc.ByteWidth = size;
	v_desc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	v_data.pSysMem = buf.data();

	result = device->CreateBuffer(&v_desc, &v_data, &mesh.ib);
	if(FAILED(result))
		throw Format("Failed to create index buffer (%u).", result);

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
			sub.tex = res_mgr->GetTexture(filename);
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
				sub.tex_normal = res_mgr->GetTexture(filename);
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
			sub.tex_specular = res_mgr->GetTexture(filename);
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
