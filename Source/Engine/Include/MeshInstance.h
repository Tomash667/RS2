#pragma once

#include "Mesh.h"

//-----------------------------------------------------------------------------
enum PLAY_FLAGS
{
	PLAY_ONCE = 0x01, // play animation once then call Decativate
	PLAY_BACK = 0x02, // play animation backward
	PLAY_NO_BLEND = 0x04, // no blending for this animation
	PLAY_IGNORE = 0x08, // ignore Play if this animation is already set
	PLAY_STOP_AT_END = 0x10, // stop animation at end
	PLAY_PRIO0 = 0, // animation priorities
	PLAY_PRIO1 = 0x20,
	PLAY_PRIO2 = 0x40,
	PLAY_PRIO3 = 0x60,
	PLAY_BLEND_WAIT = 0x100, // wait with animation until blending ends
	PLAY_RESTORE = 0x200, // reset speed and blend_max when changing animation
	PLAY_CLEAR_FRAME_END_INFO = 0x400,
	PLAY_MANUAL = 0x800
};

//-----------------------------------------------------------------------------
struct MeshInstance
{
private:
	enum FLAGS
	{
		FLAG_PLAYING = 1 << 0,
		FLAG_ONCE = 1 << 1,
		FLAG_BACK = 1 << 2,
		FLAG_GROUP_ACTIVE = 1 << 3,
		FLAG_BLENDING = 1 << 4,
		FLAG_STOP_AT_END = 1 << 5,
		FLAG_BLEND_WAIT = 1 << 6,
		FLAG_RESTORE = 1 << 7,
		FLAG_UPDATED = 1 << 8
	};

	typedef vector<byte>::const_iterator BoneIter;

public:
	struct Group
	{
		friend struct MeshInstance;

		Group() : anim(nullptr), state(0), speed(1.f), prio(0), blend_max(0.33f), frame_end_info(false)
		{
		}

		void Reset();
		void Play() { SET_BIT(state, FLAG_PLAYING); }
		void Stop() { CLEAR_BIT(state, FLAG_PLAYING); }

		bool IsActive() const { return IS_SET(state, FLAG_GROUP_ACTIVE); }
		bool IsBlending() const { return IS_SET(state, FLAG_BLENDING); }
		bool IsPlaying() const { return IS_SET(state, FLAG_PLAYING); }

		int GetFrameIndex(bool& hit) const { return anim->GetFrameIndex(time, hit); }
		float GetBlendT() const;
		float GetProgress() const { return time / anim->length; }
		Mesh::Animation* GetAnimation() const { return anim; }

	private:
		Mesh::Animation* anim;
		float time, speed, blend_time, blend_max;
		int state, prio, used_group;
		bool frame_end_info;
	};

	explicit MeshInstance(Mesh* mesh);
	void Update(float dt);
	void Play(uint group = 0) { GetGroup(group).Play(); }
	void Play(Mesh::Animation* anim, int flags, uint group);
	void Play(cstring name, int flags, uint group)
	{
		Play(mesh->GetAnimation(name), flags, group);
	}
	void Stop(uint group = 0) { GetGroup(group).Stop(); }
	void Deactivate(uint group = 0, bool in_update = false);
	void SetupBones();
	void SetupBlending(uint group, bool first = true, bool in_update = false);
	void ClearBones();
	void SetToEnd(cstring anim) { SetToEnd(mesh->GetAnimation(anim)); }
	void SetToEnd(Mesh::Animation* anim);
	void SetToEnd();
	void Reset();
	void ResetAnimation();
	void SetProgress(uint group, float progress);
	void Save(FileWriter& f);
	void Load(FileReader& f);

	bool IsBlending() const;

	int GetHighestPriority(uint& group);
	int GetUsableGroup(uint group);
	Group& GetGroup(uint index)
	{
		assert(index < groups.size());
		return groups[index];
	}
	const Group& GetGroup(uint index) const
	{
		assert(index < groups.size());
		return groups[index];
	}
	bool GetEndResult(uint group) const
	{
		return GetGroup(group).frame_end_info;
	}
	bool GetEndResultClear(uint group)
	{
		auto& g = GetGroup(group);
		bool r = g.frame_end_info;
		g.frame_end_info = false;
		return r;
	}
	float GetProgress(uint group = 0) const
	{
		return GetGroup(group).GetProgress();
	}
	Mesh* GetMesh() const { return mesh; }
	const vector<Matrix>& GetMatrixBones() const { return mat_bones; }

private:
	Mesh* mesh;
	vector<Matrix> mat_bones;
	vector<Mesh::KeyframeBone> blendb;
	vector<Group> groups;
	bool need_update;
};
