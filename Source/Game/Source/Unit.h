#pragma once

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
	float GetHpp() const { return float(hp) / maxhp; }
	Box GetBox() const;
	Vec3 GetSoundPos() const;
	float GetAngleDiff(const Vec3& target) const;
	bool IsAlive() const { return hp > 0; }
	virtual void Save(FileWriter& f);
	virtual void Load(FileReader& f);

	SceneNode* node;
	UnitType type;
	int hp, maxhp, death_timer; // FIXME - moved death_timer
	float last_damage;
	Animation animation;
	bool dying;

	static const float radius;
	static const float height;
};
