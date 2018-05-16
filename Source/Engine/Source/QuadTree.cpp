#include "Core.h"
#include "QuadTree.h"


QuadTree::Part::~Part()
{
	for(int i = 0; i < 4; ++i)
		delete childs[i];
}


QuadTree::QuadTree() : root(nullptr)
{
	factory = [] { return new Part; };
}

QuadTree::~QuadTree()
{
	delete root;
}

void QuadTree::Init(float size, uint splits, delegate<Part*()> factory)
{
	assert(size > 0 && splits > 0);
	if(factory)
		this->factory = factory;

	root = factory();
	root->box = Box2d(0, 0, size, size);
	root->size = splits;
	c.push_back(root);

	while(!c.empty())
	{
		Part& part = *c.back();
		c.pop_back();

		for(int i = 0; i < 4; ++i)
		{
			part.childs[i] = factory();
			Part& child = *part.childs[i];
			child.size = part.size - 1;
			if(child.size == 0)
			{
				for(int j = 0; j < 4; ++j)
					child.childs[j] = nullptr;
			}
			else
				c.push_back(&child);
		}

		part.childs[LEFT_TOP]->box = part.box.LeftTopPart();
		part.childs[RIGHT_TOP]->box = part.box.RightTopPart();
		part.childs[LEFT_BOTTOM]->box = part.box.LeftBottomPart();
		part.childs[RIGHT_BOTTOM]->box = part.box.RightBottomPart();
	}

	leaf_size = (float)(size / pow(2u, splits));
}

Int2 QuadTree::PosToIndex(const Vec2& pos)
{
	if(pos.x < 0 || pos.y < 0 || pos.x > root->box.v2.x || pos.y > root->box.v2.y)
		return Int2(-1, -1);
	else
		return Int2(int(pos.x / leaf_size), int(pos.y / leaf_size));
}

QuadTree::Part* QuadTree::GetPart(const Int2& pt)
{
	return GetPart(Vec2(leaf_size * pt.x + leaf_size / 2, leaf_size * pt.y + leaf_size / 2));
}

QuadTree::Part* QuadTree::GetPart(const Vec2& pos)
{
	if(pos.x < 0 || pos.y < 0 || pos.x > root->box.v2.x || pos.y > root->box.v2.y)
		return nullptr;

	Part* part = root;
	while(true)
	{
		Vec2 midpoint = part->box.Midpoint();
		int index;
		if(pos.x >= midpoint.x)
		{
			if(pos.y >= midpoint.y)
				index = RIGHT_BOTTOM;
			else
				index = RIGHT_TOP;
		}
		else
		{
			if(pos.y >= midpoint.y)
				index = LEFT_BOTTOM;
			else
				index = LEFT_TOP;
		}

		part = part->childs[index];
		if(part->size == 0u)
			break;
	}

	return part;
}

void QuadTree::ListVisibleParts(FrustumPlanes& frustum, vector<Part*>& parts)
{
	parts.clear();
	c.clear();
	c.push_back(root);

	while(!c.empty())
	{
		Part* part = c.back();
		c.pop_back();
		if(frustum.BoxToFrustum(part->box))
		{
			if(part->size == 0u)
				parts.push_back(part);
			else
			{
				for(int i = 0; i < 4; ++i)
					c.push_back(part->childs[i]);
			}
		}
	}
}
