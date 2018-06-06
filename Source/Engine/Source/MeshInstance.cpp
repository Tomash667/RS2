#include "EngineCore.h"
#include "MeshInstance.h"


const int BLEND_TO_BIND_POSE = -1;


//=================================================================================================
void MeshInstance::Group::Reset()
{
	anim = nullptr;
	state = 0;
	speed = 1.f;
	prio = 0;
	blend_max = 0.33f;
	frame_end_info = false;
}

//=================================================================================================
float MeshInstance::Group::GetBlendT() const
{
	if(IsBlending())
		return blend_time / blend_max;
	else
		return 1.f;
}


//=================================================================================================
MeshInstance::MeshInstance(Mesh* mesh) : mesh(mesh), need_update(true)
{
	mat_bones.resize(mesh->head.n_bones);
	blendb.resize(mesh->head.n_bones);
	groups.resize(mesh->head.n_groups);
}

//=================================================================================================
void MeshInstance::Play(Mesh::Animation* anim, int flags, uint group)
{
	assert(anim);

	Group& gr = GetGroup(group);

	// ignore if this is current animation
	if(IS_SET(flags, PLAY_IGNORE) && gr.anim == anim)
		return;

	// reset speed & blend time
	if(IS_SET(gr.state, PLAY_RESTORE))
	{
		gr.speed = 1.f;
		gr.blend_max = 0.33f;
	}

	int new_state = FLAG_GROUP_ACTIVE;
	if(!IS_SET(flags, PLAY_MANUAL))
		new_state |= FLAG_PLAYING;
	if(IS_SET(flags, PLAY_ONCE))
		SET_BIT(new_state, FLAG_ONCE);
	if(IS_SET(flags, PLAY_STOP_AT_END))
		SET_BIT(new_state, FLAG_STOP_AT_END);
	if(IS_SET(flags, PLAY_RESTORE))
		SET_BIT(new_state, FLAG_RESTORE);

	// blending
	if(!IS_SET(flags, PLAY_NO_BLEND))
	{
		SetupBlending(group);
		SET_BIT(new_state, FLAG_BLENDING);
		if(IS_SET(flags, PLAY_BLEND_WAIT))
			SET_BIT(new_state, FLAG_BLEND_WAIT);
		gr.blend_time = 0.f;
	}

	// set animation
	if(IS_SET(flags, PLAY_CLEAR_FRAME_END_INFO))
		gr.frame_end_info = false;
	gr.anim = anim;
	gr.prio = ((flags & 0x60) >> 5);
	gr.state = new_state;
	if(IS_SET(flags, PLAY_BACK))
	{
		SET_BIT(gr.state, FLAG_BACK);
		gr.time = anim->length;
	}
	else
		gr.time = 0.f;

	// cancel blending on other bone groups
	if(IS_SET(flags, PLAY_NO_BLEND))
	{
		for(int g = 0; g < mesh->head.n_groups; ++g)
		{
			if(g != group && (!groups[g].IsActive() || groups[g].prio < gr.prio))
				CLEAR_BIT(groups[g].state, FLAG_BLENDING);
		}
	}
}

//=================================================================================================
void MeshInstance::Deactivate(uint group, bool in_update)
{
	Group& gr = GetGroup(group);

	if(IS_SET(gr.state, FLAG_GROUP_ACTIVE))
	{
		SetupBlending(group, true, in_update);

		if(IS_SET(gr.state, FLAG_RESTORE))
		{
			gr.speed = 1.f;
			gr.blend_max = 0.33f;
		}

		gr.state = FLAG_BLENDING;
		gr.blend_time = 0.f;
	}
}

//=================================================================================================
void MeshInstance::Update(float dt)
{
	for(word i = 0; i < mesh->head.n_groups; ++i)
	{
		Group& gr = groups[i];

		if(IS_SET(gr.state, FLAG_UPDATED))
		{
			CLEAR_BIT(gr.state, FLAG_UPDATED);
			continue;
		}

		// blending
		if(IS_SET(gr.state, FLAG_BLENDING))
		{
			need_update = true;
			gr.blend_time += dt;
			if(gr.blend_time >= gr.blend_max)
				CLEAR_BIT(gr.state, FLAG_BLENDING);
		}

		// update animation
		if(IS_SET(gr.state, FLAG_PLAYING))
		{
			need_update = true;

			if(IS_SET(gr.state, FLAG_BLEND_WAIT))
			{
				if(IS_SET(gr.state, FLAG_BLENDING))
					continue;
			}

			if(IS_SET(gr.state, FLAG_BACK))
			{
				// update backward
				gr.time -= dt * gr.speed;
				if(gr.time < 0) // animation ended
				{
					gr.frame_end_info = true;
					if(IS_SET(gr.state, FLAG_ONCE))
					{
						gr.time = 0;
						if(IS_SET(gr.state, FLAG_STOP_AT_END))
							Stop(i);
						else
							Deactivate(i, true);
					}
					else
					{
						gr.time = fmod(gr.time, gr.anim->length) + gr.anim->length;
						if(gr.anim->n_frames == 1)
						{
							gr.time = 0;
							Stop(i);
						}
					}
				}
			}
			else
			{
				// update forward
				gr.time += dt * gr.speed;
				if(gr.time >= gr.anim->length) // animation ended
				{
					gr.frame_end_info = true;
					if(IS_SET(gr.state, FLAG_ONCE))
					{
						gr.time = gr.anim->length;
						if(IS_SET(gr.state, FLAG_STOP_AT_END))
							Stop(i);
						else
							Deactivate(i, true);
					}
					else
					{
						gr.time = fmod(gr.time, gr.anim->length);
						if(gr.anim->n_frames == 1)
						{
							gr.time = 0;
							Stop(i);
						}
					}
				}
			}
		}
	}
}

//====================================================================================================
void MeshInstance::SetupBones()
{
	if(!need_update)
		return;
	need_update = false;

	Matrix BoneToParentPoseMat[32];
	BoneToParentPoseMat[0] = Matrix::IdentityMatrix;
	Mesh::KeyframeBone tmp_keyf;

	// calculate transformations for each group
	const word n_groups = mesh->head.n_groups;
	for(word bones_group = 0; bones_group < n_groups; ++bones_group)
	{
		const Group& gr_bones = groups[bones_group];
		const vector<byte>& bones = mesh->groups[bones_group].bones;
		int anim_group;

		// decide which group to use for blending
		anim_group = GetUsableGroup(bones_group);

		if(anim_group == BLEND_TO_BIND_POSE)
		{
			// there is no animation
			if(gr_bones.IsBlending())
			{
				// blend from current blending to bind pose
				float bt = gr_bones.blend_time / gr_bones.blend_max;

				for(BoneIter it = bones.begin(), end = bones.end(); it != end; ++it)
				{
					const word b = *it;
					Mesh::KeyframeBone::Interpolate(tmp_keyf, blendb[b], Mesh::KeyframeBone::Zero, bt);
					tmp_keyf.Mix(BoneToParentPoseMat[b], mesh->bones[b].mat);
				}
			}
			else
			{
				// no animation and no blending, zero everything
				for(BoneIter it = bones.begin(), end = bones.end(); it != end; ++it)
				{
					const word b = *it;
					BoneToParentPoseMat[b] = mesh->bones[b].mat;
				}
			}
		}
		else
		{
			const Group& gr_anim = groups[anim_group];
			bool hit;
			const int index = gr_anim.GetFrameIndex(hit);
			const vector<Mesh::Keyframe>& frames = gr_anim.anim->frames;

			if(gr_anim.IsBlending() || gr_bones.IsBlending())
			{
				// there is blending
				const float bt = (gr_bones.IsBlending() ? (gr_bones.blend_time / gr_bones.blend_max) :
					(gr_anim.blend_time / gr_anim.blend_max));

				if(hit)
				{
					// exact hit in frame
					const vector<Mesh::KeyframeBone>& keyf = frames[index].bones;
					for(BoneIter it = bones.begin(), end = bones.end(); it != end; ++it)
					{
						const word b = *it;
						Mesh::KeyframeBone::Interpolate(tmp_keyf, blendb[b], keyf[b - 1], bt);
						tmp_keyf.Mix(BoneToParentPoseMat[b], mesh->bones[b].mat);
					}
				}
				else
				{
					// interpolate between two frames
					const float t = (gr_anim.time - frames[index].time) / (frames[index + 1].time - frames[index].time);
					const vector<Mesh::KeyframeBone>& keyf = frames[index].bones;
					const vector<Mesh::KeyframeBone>& keyf2 = frames[index + 1].bones;

					for(BoneIter it = bones.begin(), end = bones.end(); it != end; ++it)
					{
						const word b = *it;
						Mesh::KeyframeBone::Interpolate(tmp_keyf, keyf[b - 1], keyf2[b - 1], t);
						Mesh::KeyframeBone::Interpolate(tmp_keyf, blendb[b], tmp_keyf, bt);
						tmp_keyf.Mix(BoneToParentPoseMat[b], mesh->bones[b].mat);
					}
				}
			}
			else
			{
				// there is no blending
				if(hit)
				{
					// exact hit in frame
					const vector<Mesh::KeyframeBone>& keyf = frames[index].bones;
					for(BoneIter it = bones.begin(), end = bones.end(); it != end; ++it)
					{
						const word b = *it;
						keyf[b - 1].Mix(BoneToParentPoseMat[b], mesh->bones[b].mat);
					}
				}
				else
				{
					// interpolate between two frames
					const float t = (gr_anim.time - frames[index].time) / (frames[index + 1].time - frames[index].time);
					const vector<Mesh::KeyframeBone>& keyf = frames[index].bones;
					const vector<Mesh::KeyframeBone>& keyf2 = frames[index + 1].bones;

					for(BoneIter it = bones.begin(), end = bones.end(); it != end; ++it)
					{
						const word b = *it;
						Mesh::KeyframeBone::Interpolate(tmp_keyf, keyf[b - 1], keyf2[b - 1], t);
						tmp_keyf.Mix(BoneToParentPoseMat[b], mesh->bones[b].mat);
					}
				}
			}
		}
	}

	// calculate matrices transforming from bone to model
	Matrix BoneToModelPoseMat[32];
	BoneToModelPoseMat[0] = Matrix::IdentityMatrix;
	for(word i = 1; i < mesh->head.n_bones; ++i)
	{
		const Mesh::Bone& bone = mesh->bones[i];
		if(bone.parent == 0)
			BoneToModelPoseMat[i] = BoneToParentPoseMat[i];
		else
			BoneToModelPoseMat[i] = BoneToParentPoseMat[i] * BoneToModelPoseMat[bone.parent];
	}

	// calculate final matrices
	mat_bones[0] = Matrix::IdentityMatrix;
	for(word i = 1; i < mesh->head.n_bones; ++i)
		mat_bones[i] = mesh->model_to_bone[i] * BoneToModelPoseMat[i];
}

//=================================================================================================
void MeshInstance::SetupBlending(uint bones_group, bool first, bool in_update)
{
	int anim_group;
	const auto& gr_bones = GetGroup(bones_group);
	const vector<byte>& bones = mesh->groups[bones_group].bones;

	// decide which group to use for blending
	anim_group = GetUsableGroup(bones_group);

	if(anim_group == BLEND_TO_BIND_POSE)
	{
		// there is no animation
		if(gr_bones.IsBlending())
		{
			// blend from current blending to bind pose
			const float bt = gr_bones.blend_time / gr_bones.blend_max;

			for(BoneIter it = bones.begin(), end = bones.end(); it != end; ++it)
			{
				const word b = *it;
				Mesh::KeyframeBone::Interpolate(blendb[b], blendb[b], Mesh::KeyframeBone::Zero, bt);
			}
		}
		else
		{
			// no animation and no blending, zero everything
			for(BoneIter it = bones.begin(), end = bones.end(); it != end; ++it)
				memcpy(&blendb[*it], &Mesh::KeyframeBone::Zero, sizeof(Mesh::KeyframeBone::Zero));
		}
	}
	else
	{
		// there is animation
		const Group& gr_anim = groups[anim_group];
		bool hit;
		const int index = gr_anim.GetFrameIndex(hit);
		const vector<Mesh::Keyframe>& frames = gr_anim.anim->frames;

		if(gr_anim.IsBlending() || gr_bones.IsBlending())
		{
			// there is blending
			const float bt = (gr_bones.IsBlending() ? (gr_bones.blend_time / gr_bones.blend_max) :
				(gr_anim.blend_time / gr_anim.blend_max));

			if(hit)
			{
				// exact hit in frame
				const vector<Mesh::KeyframeBone>& keyf = frames[index].bones;
				for(BoneIter it = bones.begin(), end = bones.end(); it != end; ++it)
				{
					const word b = *it;
					Mesh::KeyframeBone::Interpolate(blendb[b], blendb[b], keyf[b - 1], bt);
				}
			}
			else
			{
				// interpolate between two frames and blend frame
				const float t = (gr_anim.time - frames[index].time) / (frames[index + 1].time - frames[index].time);
				const vector<Mesh::KeyframeBone>& keyf = frames[index].bones;
				const vector<Mesh::KeyframeBone>& keyf2 = frames[index + 1].bones;
				Mesh::KeyframeBone tmp_keyf;

				for(BoneIter it = bones.begin(), end = bones.end(); it != end; ++it)
				{
					const word b = *it;
					Mesh::KeyframeBone::Interpolate(tmp_keyf, keyf[b - 1], keyf2[b - 1], t);
					Mesh::KeyframeBone::Interpolate(blendb[b], blendb[b], tmp_keyf, bt);
				}
			}
		}
		else
		{
			// there is no blending
			if(hit)
			{
				// exact hit in frame
				const vector<Mesh::KeyframeBone>& keyf = frames[index].bones;
				for(BoneIter it = bones.begin(), end = bones.end(); it != end; ++it)
				{
					const word b = *it;
					blendb[b] = keyf[b - 1];
				}
			}
			else
			{
				// interpolate between two frames
				const float t = (gr_anim.time - frames[index].time) / (frames[index + 1].time - frames[index].time);
				const vector<Mesh::KeyframeBone>& keyf = frames[index].bones;
				const vector<Mesh::KeyframeBone>& keyf2 = frames[index + 1].bones;

				for(BoneIter it = bones.begin(), end = bones.end(); it != end; ++it)
				{
					const word b = *it;
					Mesh::KeyframeBone::Interpolate(blendb[b], keyf[b - 1], keyf2[b - 1], t);
				}
			}
		}
	}

	// find child groups that are inactive and setup blending for them
	if(first)
	{
		for(uint group = 0; group < mesh->head.n_groups; ++group)
		{
			auto& gr = groups[group];
			if(group != bones_group && (!gr.IsActive() || gr.prio < gr_bones.prio))
			{
				SetupBlending(group, false);
				SET_BIT(gr.state, FLAG_BLENDING);
				if(in_update && group > bones_group)
					SET_BIT(gr.state, FLAG_UPDATED);
				gr.blend_time = 0;
			}
		}
	}
}

//=================================================================================================
bool MeshInstance::IsBlending() const
{
	for(int i = 0; i < mesh->head.n_groups; ++i)
	{
		if(IS_SET(groups[i].state, FLAG_BLENDING))
			return true;
	}
	return false;
}

//=================================================================================================
int MeshInstance::GetHighestPriority(uint& group)
{
	int best = -1;

	for(uint i = 0; i < uint(mesh->head.n_groups); ++i)
	{
		if(groups[i].IsActive() && groups[i].prio > best)
		{
			best = groups[i].prio;
			group = i;
		}
	}

	return best;
}

//=================================================================================================
int MeshInstance::GetUsableGroup(uint group)
{
	uint top_group;
	int highest_prio = GetHighestPriority(top_group);
	if(highest_prio == -1)
		return BLEND_TO_BIND_POSE;
	else if(groups[group].IsActive() && groups[group].prio == highest_prio)
		return group;
	else
		return top_group;
}

//=================================================================================================
// Clear matrices to allow render base pose without using any animation
void MeshInstance::ClearBones()
{
	for(int i = 0; i < mesh->head.n_bones; ++i)
		mat_bones[i] = Matrix::IdentityMatrix;
	need_update = false;
}

//=================================================================================================
void MeshInstance::SetToEnd(Mesh::Animation* anim)
{
	assert(anim);

	groups[0].anim = anim;
	groups[0].blend_time = 0.f;
	groups[0].state = FLAG_GROUP_ACTIVE;
	groups[0].time = anim->length;
	groups[0].used_group = 0;
	groups[0].prio = 0;

	if(mesh->head.n_groups > 1)
	{
		for(int i = 1; i < mesh->head.n_groups; ++i)
		{
			groups[i].anim = nullptr;
			groups[i].state = 0;
			groups[i].used_group = 0;
			groups[i].time = groups[0].time;
			groups[i].blend_time = groups[0].blend_time;
		}
	}

	need_update = true;
	SetupBones();
}

//=================================================================================================
void MeshInstance::SetToEnd()
{
	groups[0].blend_time = 0.f;
	groups[0].state = FLAG_GROUP_ACTIVE;
	groups[0].time = groups[0].anim->length;
	groups[0].used_group = 0;
	groups[0].prio = 0;

	for(uint i = 1; i < groups.size(); ++i)
	{
		groups[i].state = 0;
		groups[i].used_group = 0;
		groups[i].blend_time = 0;
	}

	need_update = true;
	SetupBones();
}

//=================================================================================================
void MeshInstance::Reset()
{
	for(Group& group : groups)
		group.Reset();
	need_update = true;
}

//=================================================================================================
void MeshInstance::ResetAnimation()
{
	SetupBlending(0);

	groups[0].time = 0.f;
	groups[0].blend_time = 0.f;
	SET_BIT(groups[0].state, FLAG_BLENDING | FLAG_PLAYING);
}

//=================================================================================================
void MeshInstance::SetProgress(uint group_index, float progress)
{
	assert(group_index < groups.size());
	Group& group = groups[group_index];
	assert(group.anim);
	float new_time = group.anim->length * progress;
	if(new_time != group.time)
	{
		group.time = new_time;
		need_update = true;
	}
}

//=================================================================================================
void MeshInstance::Save(FileWriter& f)
{
	assert(mesh);

	for(Group& group : groups)
	{
		f << group.time;
		f << group.speed;
		f << (group.state & ~FLAG_BLENDING);
		f << group.prio;
		f << group.used_group;
		f << (group.anim ? group.anim->name : "");
		f << group.frame_end_info;
	}
}

//=================================================================================================
void MeshInstance::Load(FileReader& f)
{
	assert(mesh);

	for(Group& group : groups)
	{
		f >> group.time;
		f >> group.speed;
		f >> group.state;
		f >> group.prio;
		f >> group.used_group;
		const string& anim_name = f.ReadString1();
		group.anim = (anim_name.empty() ? nullptr : mesh->GetAnimation(anim_name.c_str()));
		f >> group.frame_end_info;
	}

	need_update = true;
}
