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

	struct Factory;

	struct Part
	{
		void Remove(Factory* factory);
		bool IsLeaf() const { return size == 0; }

		Box2d box;
		uint size;
		Part* childs[4];
	};

	struct Factory
	{
		virtual Part* Create() { return new Part; }
		virtual void Remove(Part* part) { delete part; }
	};

	QuadTree();
	~QuadTree();
	void Init(float size, uint splits, Factory* factory = nullptr);
	Int2 PosToIndex(const Vec2& pos);
	void ListVisibleParts(FrustumPlanes& frustum, vector<Part*>& parts);
	void ForEach(delegate<void(Part*)> callback, bool leaf_only = true);

	Part* GetPart(const Int2& pt);
	Part* GetPart(const Vec2& pos);
	Part* GetRoot() { return root; }

private:
	Factory* factory;
	Part* root;
	vector<Part*> c;
	float leaf_size;
};
