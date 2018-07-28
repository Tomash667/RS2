#pragma once

class Scene
{
public:
	Scene();
	~Scene();
	void Init(Render* render, ResourceManager* res_mgr);
	void Reset();
	void InitQuadTree(float size, uint splits);
	void Draw();
	void Update(float dt);
	void Add(SceneNode* node);
	void Add(ParticleEmitter* pe);
	void Remove(SceneNode* node);
	void RecycleMeshInstance(SceneNode* node);
	void OnChangeResolution(const Int2& wnd_size);

	void SetAmbientColor(const Vec3& ambient_color) { this->ambient_color = ambient_color; }
	void SetDebugDrawEnabled(bool enabled) { debug_draw_enabled = enabled; }
	void SetDebugDrawHandler(delegate<void(DebugDrawer*)> handler) { debug_draw_handler = handler; }
	void SetFogColor(const Vec3& fog_color) { this->fog_color = fog_color; }
	void SetFogParams(float start, float end);
	void SetLightDir(const Vec3& light_dir) { this->light_dir = light_dir; }
	void SetLightColor(const Vec3& light_color) { this->light_color = light_color; }
	void SetSkybox(Mesh* mesh) { skybox = mesh; }
	void SetSky(Sky* sky) { this->sky = sky; }

	const Vec3& GetAmbientColor() { return ambient_color; }
	Camera* GetCamera() { return camera.get(); }
	const Vec3& GetFogColor() { return fog_color; }
	Vec2 GetFogParams() { return fog_params.XY(); }
	const Vec3& GetLightDir() { return light_dir; }
	const Vec3& GetLightColor() { return light_color; }
	Mesh* GetSkybox() { return skybox; }
	Sky* GetSky() { return sky; }
	const Matrix& GetViewProjectionMatrix() { return mat_view_proj; }
	QuadTree* GetQuadTree() { return quad_tree.get(); }

private:
	void DrawSkybox();
	void DrawNodes();
	void DrawNodes(vector<SceneNode*>& nodes, const Matrix* parent_matrix, const Vec4& parent_tint);
	void DrawParticles();
	void UpdateNodes(vector<SceneNode*>& nodes, float dt);
	void ListVisibleNodes();
	void ListVisibleNodes(vector<SceneNode*>& nodes);

	Render* render;
	unique_ptr<MeshShader> mesh_shader;
	unique_ptr<ParticleShader> particle_shader;
	unique_ptr<SkyboxShader> skybox_shader;
	unique_ptr<SkyShader> sky_shader;
	unique_ptr<DebugDrawer> debug_drawer;
	unique_ptr<Camera> camera;
	vector<SceneNode*> nodes, visible_nodes, visible_alpha_nodes;
	vector<ParticleEmitter*> pes, visible_pes;
	Matrix mat_view, mat_view_proj;
	Vec3 fog_color, fog_params,  light_dir, light_color, ambient_color;
	FrustumPlanes frustum_planes;
	unique_ptr<QuadTree> quad_tree;
	vector<ScenePart*> parts;
	vector<MeshInstance*> mesh_inst_pool;
	Mesh* skybox;
	Sky* sky;
	delegate<void(DebugDrawer*)> debug_draw_handler;
	bool debug_draw_enabled;
};
