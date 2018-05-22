#pragma once

class Scene
{
public:
	Scene();
	~Scene();
	void Init(Render* render);
	void InitQuadTree(float size, uint splits);
	void Draw();
	void Update(float dt);
	void Add(SceneNode* node);
	void Add(ParticleEmitter* pe);
	void Remove(SceneNode* node);
	void RecycleMeshInstance(SceneNode* node);

	void SetFogColor(const Vec4& fog_color) { this->fog_color = fog_color; }
	void SetFogParams(float start, float end);

	Camera* GetCamera() { return camera.get(); }
	const Matrix& GetViewProjectionMatrix() { return mat_view_proj; }
	QuadTree* GetQuadTree() { return quad_tree.get(); }

private:
	void DrawNodes();
	void DrawNodes(vector<SceneNode*>& nodes, const Matrix* parent_matrix);
	void DrawParticles();
	void UpdateNodes(vector<SceneNode*>& nodes, float dt);
	void ListVisibleNodes();
	void ListVisibleNodes(vector<SceneNode*>& nodes);

	Render* render;
	unique_ptr<MeshShader> mesh_shader;
	unique_ptr<ParticleShader> particle_shader;
	unique_ptr<Camera> camera;
	vector<SceneNode*> nodes, visible_nodes, visible_alpha_nodes;
	vector<ParticleEmitter*> pes, visible_pes;
	Matrix mat_view, mat_view_proj;
	Vec4 fog_color, fog_params;
	FrustumPlanes frustum_planes;
	unique_ptr<QuadTree> quad_tree;
	vector<ScenePart*> parts;
	vector<MeshInstance*> mesh_inst_pool;
};
