#pragma once

struct QuadTree
{
	enum Index
	{
		LEFT_TOP,
		RIGHT_TOP,
		LEFT_BOTTOM,
		RIGHT_BOTTOM
	};

	struct Part
	{
		virtual ~Part();

		Box2d box;
		uint size;
		Part* childs[4];
	};
	
	QuadTree();
	~QuadTree();
	void Init(float size, uint splits, delegate<Part*()> factory = nullptr);
	Int2 PosToIndex(const Vec2& pos);
	void ListVisibleParts(FrustumPlanes& frustum, vector<Part*>& parts);

	Part* GetPart(const Int2& pt);
	Part* GetPart(const Vec2& pos);

private:
	delegate<Part*()> factory;
	Part* root;
	vector<Part*> c;
	float leaf_size;
};
