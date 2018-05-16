#include "EngineCore.h"
#include "ParticleEmitter.h"


float DropRange(float v, float t)
{
	if(v > 0)
	{
		float t_up = v / G;
		if(t_up >= t)
			return (v*v) / (2 * G);
		else
			return v * t - (G*(t*t)) / 2;
	}
	else
		return v * t - G * (t*t) / 2;
}


void ParticleEmitter::Init()
{
	particles.resize(max_particles);
	time = 0.f;
	alive = 0;
	destroy = false;
	for(int i = 0; i < max_particles; ++i)
		particles[i].exists = false;

	// calculate radius
	float t;
	if(life > 0)
		t = min(particle_life, life);
	else
		t = particle_life;
	float r = 0.f;
	// left
	float r2 = abs(pos_min.x + speed_min.x * t);
	if(r2 > r)
		r = r2;
	// right
	r2 = abs(pos_max.x + speed_max.x * t);
	if(r2 > r)
		r = r2;
	// back
	r2 = abs(pos_min.z + speed_min.z * t);
	if(r2 > r)
		r = r2;
	// forward
	r2 = abs(pos_max.z + speed_max.z * t);
	if(r2 > r)
		r = r2;
	// up
	r2 = abs(pos_max.y + DropRange(speed_max.y, t));
	if(r2 > r)
		r = r2;
	// down
	r2 = abs(pos_min.y + DropRange(speed_min.y, t));
	if(r2 > r)
		r = r2;

	radius = sqrt(2 * r*r);
}

bool ParticleEmitter::Update(float dt)
{
	if(emisions == 0 || (life > 0 && (life -= dt) <= 0.f))
		destroy = true;

	if(destroy && alive == 0)
		return false;

	// update particles
	for(Particle& p : particles)
	{
		if(!p.exists)
			continue;

		if((p.life -= dt) <= 0.f)
		{
			p.exists = false;
			--alive;
		}
		else
		{
			p.pos += p.speed * dt;
			p.speed.y -= p.gravity * dt;
		}
	}

	// emit new ones
	if(!destroy && (emisions == -1 || emisions > 0) && ((time += dt) >= emision_interval))
	{
		if(emisions > 0)
			--emisions;
		time -= emision_interval;

		int count = min(Random(spawn_min, spawn_max), max_particles - alive);
		vector<Particle>::iterator it = particles.begin();

		for(int i = 0; i < count; ++i)
		{
			while(it->exists)
				++it;

			Particle& p = *it;
			p.exists = true;
			p.gravity = G;
			p.life = particle_life;
			p.pos = pos + Vec3::Random(pos_min, pos_max);
			p.speed = Vec3::Random(speed_min, speed_max);
		}

		alive += count;
	}

	return true;
}
