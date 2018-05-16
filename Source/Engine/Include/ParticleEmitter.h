#pragma once

//-----------------------------------------------------------------------------
enum PARTICLE_OP
{
	POP_CONST,
	POP_LINEAR_SHRINK
};

//-----------------------------------------------------------------------------
struct ParticleEmitter : ObjectPoolProxy<ParticleEmitter>
{
	Texture* tex;
	float emision_interval, life, particle_life, alpha, size;
	int emisions, spawn_min, spawn_max, max_particles;
	Vec3 pos, speed_min, speed_max, pos_min, pos_max;
	PARTICLE_OP op_size, op_alpha;

private:
	friend class Scene;
	friend class ParticleShader;

	struct Particle
	{
		Vec3 pos, speed;
		float life, gravity;
		bool exists;
	};

	void Init();
	bool Update(float dt);
	float GetAlpha(const Particle &p) const
	{
		if(op_alpha == POP_CONST)
			return alpha;
		else
			return Lerp(0.f, alpha, p.life / particle_life);
	}
	float GetScale(const Particle &p) const
	{
		if(op_size == POP_CONST)
			return size;
		else
			return Lerp(0.f, size, p.life / particle_life);
	}

	vector<Particle> particles;
	float time, radius;
	int alive;
	bool destroy;
};
