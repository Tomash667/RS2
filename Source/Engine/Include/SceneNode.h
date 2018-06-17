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
		container(nullptr), scale(1.f), alpha(false), use_matrix(false) {}
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
	union
	{
		struct
		{
			Vec3 pos, rot;
			float scale;
		};
		Matrix mat;
	};
	int subs;
	bool visible, alpha, use_matrix;

private:
	SceneNode* parent;
	MeshPoint* parent_point;
	vector<SceneNode*> childs;
};
