#include <Core.h>
#include "Tree.h"

static ObjectPool<Tree::Node> node_pool;

static Tree::Node* GetNode(const Int2& pos, const Int2& size)
{
	Tree::Node* node = node_pool.Get();
	node->pos = pos;
	node->size = size;
	node->childs[0] = nullptr;
	node->childs[1] = nullptr;
	return node;
}

Tree::Tree(int min_size, int max_size, const Int2& size, int split_width) : min_size(min_size), max_size(max_size), split_width(split_width)
{
	min_size_before_split = min_size * 2 + split_width;
	assert(max_size >= min_size_before_split);
	to_check.push_back(GetNode(Int2::Zero, size));
}

Tree::~Tree()
{
	node_pool.Free(nodes);
}

bool Tree::CanSplit(Node& node)
{
	return node.size.x > max_size
		|| node.size.y > max_size
		|| (node.size.x >= min_size_before_split && node.size.y >= min_size_before_split && Rand() % 4 != 0);
}

void Tree::Split(Node& node)
{
	float ratio = float(node.size.x) / node.size.y;
	if(ratio >= 1.25f)
		node.horizontal = true;
	else if(ratio <= 0.75f)
		node.horizontal = false;
	else
		node.horizontal = Rand() % 2 == 0;

	if(node.horizontal)
	{
		int split = Random(min_size, node.size.x - min_size - split_width);
		node.split_pos = Int2(node.pos.x + split, node.pos.y);
		node.split_size = Int2(split_width, node.size.y);
		node.childs[0] = GetNode(node.pos, Int2(split, node.size.y));
		node.childs[1] = GetNode(Int2(node.pos.x + split + split_width, node.pos.y), Int2(node.size.x - split - split_width, node.size.y));
	}
	else
	{
		int split = Random(min_size, node.size.y - min_size - split_width);
		node.split_pos = Int2(node.pos.x, node.pos.y + split);
		node.split_size = Int2(node.size.x, split_width);
		node.childs[0] = GetNode(node.pos, Int2(node.size.x, split));
		node.childs[1] = GetNode(Int2(node.pos.x, node.pos.y + split + split_width), Int2(node.size.x, node.size.y - split - split_width));
	}

	to_check.push_back(node.childs[0]);
	to_check.push_back(node.childs[1]);
}

void Tree::SplitAll()
{
	while(!to_check.empty())
	{
		Node* node = to_check.back();
		to_check.pop_back();
		nodes.push_back(node);
		if(CanSplit(*node))
			Split(*node);
		else
			leafs.push_back(node);
	}
}
