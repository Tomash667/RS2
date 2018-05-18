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
	ANI_DIE
};

struct Unit
{
	explicit Unit(bool is_zombie) : hp(100), animation(ANI_STAND), is_zombie(is_zombie), last_damage(0), dying(false) {}
	void Update(Animation new_animation);
	float GetHpp() const { return float(hp) / 100; }
	Box GetBox() const;
	Vec3 GetSoundPos() const;

	SceneNode* node;
	int hp;
	float last_damage;
	Animation animation;
	bool is_zombie, dying;

	static const float radius;
	static const float height;
};
