#pragma once

struct Tree
{
	struct Node
	{
		bool IsLeaf() const { return childs[0] == nullptr; }

		Int2 pos, size, split_pos, split_size;
		Node* childs[2];
		bool horizontal;
	};

	Tree(int min_size, int max_size, int size, int split_width);
	~Tree();
	bool CanSplit(Node& node);
	void Split(Node& node);
	void SplitAll();

	vector<Node*> nodes, leafs;

private:
	vector<Node*> to_check;
	int min_size, max_size, min_size_before_split, split_width;
};
