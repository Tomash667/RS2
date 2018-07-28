#pragma once

#include "UnitStats.h"

enum Animation
{
	ANI_STAND,
	ANI_ROTATE_LEFT,
	ANI_ROTATE_RIGHT,
	ANI_WALK,
	ANI_WALK_BACK,
	ANI_RUN,
	ANI_ACTION,
	ANI_DIE,
	ANI_IDLE
};

enum UnitType
{
	UNIT_PLAYER,
	UNIT_ZOMBIE,
	UNIT_NPC
};

struct Unit
{
	explicit Unit(UnitType type) : hp(100), maxhp(100), animation(ANI_STAND), type(type), last_damage(0), dying(false) {}
	virtual ~Unit() {}
	void Update(Animation new_animation);

	virtual void Save(FileWriter& f);
	virtual void Load(FileReader& f);

	bool IsAlive() const { return hp > 0; }

	float GetHpp() const { return float(hp) / maxhp; }
	Box GetBox() const;
	Vec3 GetSoundPos() const;
	float GetAngleDiff(const Vec3& target) const;
	const UnitStats& GetStats() const { return UnitStats::stats[type == UNIT_ZOMBIE ? 1 : 0]; }

	template<typename T>
	T& To()
	{
		static_assert(false, "Not specialized type!");
	}
	template<>
	Npc& To()
	{
		assert(type == UNIT_NPC);
		return (Npc&)*this;
	}

	SceneNode* node;
	UnitType type;
	int hp, maxhp, death_timer;
	float last_damage;
	Animation animation;
	bool dying;

	static const float radius;
	static const float height;
};
