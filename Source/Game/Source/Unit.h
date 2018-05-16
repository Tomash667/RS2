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
	explicit Unit(bool is_zombie) : hp(100), animation(ANI_STAND), is_zombie(is_zombie) {}
	void Update(Animation new_animation);
	float GetHpp() const { return float(hp) / 100; }
	Box GetBox() const;

	SceneNode* node;
	int hp;
	Animation animation;
	bool is_zombie;

	static const float radius;
	static const float height;
};