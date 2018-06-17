#include "EngineCore.h"
#include "Mesh.h"
#include <d3d11_1.h>
#include "InternalHelper.h"


const Mesh::KeyframeBone Mesh::KeyframeBone::Zero = { Vec3::Zero, Quat::Identity, 1.f };


Mesh::Mesh(cstring name) : Resource(name, Resource::Type::Mesh), vb(nullptr), ib(nullptr)
{
}

Mesh::~Mesh()
{
	SafeRelease(vb);
	SafeRelease(ib);
}

void Mesh::SetupBoneMatrices()
{
	model_to_bone.resize(head.n_bones);
	model_to_bone[0] = Matrix::IdentityMatrix;

	for(word i = 1; i < head.n_bones; ++i)
	{
		const Mesh::Bone& bone = bones[i];
		bone.mat.Inverse(model_to_bone[i]);

		if(bone.parent > 0)
			model_to_bone[i] = model_to_bone[bone.parent] * model_to_bone[i];
	}
}

Mesh::Animation* Mesh::GetAnimation(cstring name)
{
	assert(name);

	for(vector<Animation>::iterator it = anims.begin(), end = anims.end(); it != end; ++it)
	{
		if(it->name == name)
			return &*it;
	}

	return nullptr;
}

Mesh::Bone* Mesh::GetBone(cstring name)
{
	assert(name);

	for(vector<Bone>::iterator it = bones.begin(), end = bones.end(); it != end; ++it)
	{
		if(it->name == name)
			return &*it;
	}

	return nullptr;
}

Mesh::Point* Mesh::GetPoint(cstring name)
{
	assert(name);

	for(vector<Point>::iterator it = attach_points.begin(), end = attach_points.end(); it != end; ++it)
	{
		if(it->name == name)
			return &*it;
	}

	return nullptr;
}

Mesh::Point* Mesh::FindPoint(cstring name)
{
	assert(name);

	int len = strlen(name);

	for(vector<Point>::iterator it = attach_points.begin(), end = attach_points.end(); it != end; ++it)
	{
		if(strncmp(name, (*it).name.c_str(), len) == 0)
			return &*it;
	}

	return nullptr;
}

bool Mesh::HavePoint(Point* point)
{
	assert(point);
	for(Point& pt : attach_points)
	{
		if(&pt == point)
			return true;
	}
	return false;
}


int Mesh::Animation::GetFrameIndex(float time, bool& hit)
{
	assert(time >= 0 && time <= length);

	for(word i = 0; i < n_frames; ++i)
	{
		if(Equal(time, frames[i].time))
		{
			// hit frame
			hit = true;
			return i;
		}
		else if(time < frames[i].time)
		{
			// need to interpolate between two frames
			assert(i != 0 && "Time before first frame!");
			hit = false;
			return i - 1;
		}
	}

	// unreachable, should hit assert
	return 0;
}

void Mesh::Animation::GetKeyframeData(uint bone, float time, KeyframeBone& keyframe)
{
	bool hit;
	int index = GetFrameIndex(time, hit);

	if(hit)
	{
		// exact hit in frame
		keyframe = frames[index].bones[bone - 1];
	}
	else
	{
		// interpolate beetween two key frames
		const auto& keyf = frames[index].bones[bone - 1];
		const auto& keyf2 = frames[index + 1].bones[bone - 1];
		const float t = (time - frames[index].time) / (frames[index + 1].time - frames[index].time);

		KeyframeBone::Interpolate(keyframe, keyf, keyf2, t);
	}
}


void Mesh::KeyframeBone::Interpolate(Mesh::KeyframeBone& out, const Mesh::KeyframeBone& k, const Mesh::KeyframeBone& k2, float t)
{
	out.rot = Quat::Slerp(k.rot, k2.rot, t);
	out.pos = Vec3::Lerp(k.pos, k2.pos, t);
	out.scale = Lerp(k.scale, k2.scale, t);
}

void Mesh::KeyframeBone::Mix(Matrix& out, const Matrix& mul) const
{
	out = Matrix::Scale(scale)
		* Matrix::Rotation(rot)
		* Matrix::Translation(pos)
		* mul;
}

