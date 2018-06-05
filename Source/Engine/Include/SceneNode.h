#pragma once

struct SceneNode
{
	friend class Scene;

	struct Container
	{
		Container() {}

		union
		{
			Box box;
			float radius;
		};
		bool is_sphere;
	};

	SceneNode() : parent(nullptr), parent_point(nullptr), mesh(nullptr), mesh_inst(nullptr), tint(Vec4::One), subs(0xFFFFFFFF), visible(true),
		container(nullptr), scale(1.f), alpha(false) {}
	~SceneNode();
	void Add(SceneNode* node, MeshPoint* point = nullptr);

	void SetParentPoint(MeshPoint* point);

	MeshInstance* GetMeshInstance();
	SceneNode* GetParent() { return parent; }
	MeshPoint* GetParentPoint() { return parent_point; }

	static MeshPoint* USE_PARENT_BONES;

	Mesh* mesh;
	MeshInstance* mesh_inst;
	Container* container;
	Vec4 tint;
	Vec3 pos, rot;
	float scale;
	int subs;
	bool visible, alpha;

private:
	SceneNode* parent;
	MeshPoint* parent_point;
	vector<SceneNode*> childs;
};
