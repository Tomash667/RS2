#pragma once

struct Tree
{
	struct Node
	{
		bool IsLeaf() const { return childs[0] == nullptr; }

		Int2 pos, size, split_pos, split_size;
		Node* childs[2];
		bool horizontal_split;
	};

	Tree(int min_size, int max_size, int map_size, int split_width);
	~Tree();
	void SplitAll();
	bool CanSplit(Node& node);
	void Split(Node& node);

	vector<Node*> nodes, leafs;

private:
	vector<Node*> to_check;
	int max_size, min_size, min_size_before_split, split_width;
};
