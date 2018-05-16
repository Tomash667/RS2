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

	SceneNode() : parent(nullptr), parent_point(nullptr), mesh(nullptr), mesh_inst(nullptr), tint(Vec3::One), subs(0xFFFFFFFF), visible(true),
		container(nullptr) {}
	~SceneNode();
	void Add(SceneNode* node, MeshPoint* point = nullptr);
	MeshInstance* GetMeshInstance();

	static MeshPoint* USE_PARENT_BONES;

	Mesh* mesh;
	MeshInstance* mesh_inst;
	Container* container;
	Vec3 pos, rot, tint;
	int subs;
	bool visible;

private:
	SceneNode* parent;
	MeshPoint* parent_point;
	vector<SceneNode*> childs;
};
