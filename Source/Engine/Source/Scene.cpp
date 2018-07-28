#include "EngineCore.h"
#include "Scene.h"
#include "SceneNode.h"
#include "Render.h"
#include "Camera.h"
#include "MeshShader.h"
#include "MeshInstance.h"
#include "ParticleEmitter.h"
#include "ParticleShader.h"
#include "QuadTree.h"
#include "ScenePart.h"
#include "SkyboxShader.h"
#include "SkyShader.h"
#include "Sky.h"
#include "DebugDrawer.h"


struct ScenePartFactory : QuadTree::Factory
{
	QuadTree::Part* Create() override
	{
		return new ScenePart;
	}

	void Remove(QuadTree::Part* part) override
	{
		delete (ScenePart*)part;
	}
} scene_part_factory;


Scene::Scene() : fog_color(Color::White), fog_params(1000, 2000, 1000), skybox(nullptr), sky(nullptr), light_dir(0, 1, 0), light_color(1, 1, 1),
ambient_color(1, 1, 1), debug_draw_enabled(false)
{
	camera.reset(new Camera);
}

Scene::~Scene()
{
	DeleteElements(nodes);
	ParticleEmitter::Free(pes);
	DeleteElements(mesh_inst_pool);
	delete sky;
}

void Scene::Init(Render* render, ResourceManager* res_mgr)
{
	assert(render);
	this->render = render;

	mesh_shader.reset(new MeshShader(render));
	mesh_shader->Init();

	particle_shader.reset(new ParticleShader(render));
	particle_shader->Init();

	skybox_shader.reset(new SkyboxShader(render));
	skybox_shader->Init();

	sky_shader.reset(new SkyShader(render));
	sky_shader->Init();

	debug_drawer.reset(new DebugDrawer(render, res_mgr));
	debug_drawer->Init();
}

void Scene::Reset()
{
	DeleteElements(nodes);
	ParticleEmitter::Free(pes);
	if(quad_tree)
	{
		quad_tree->ForEach([](QuadTree::Part* part)
		{
			ScenePart* scene_part = (ScenePart*)part;
			scene_part->Reset();
		});
	}
}

void Scene::Draw()
{
	mat_view_proj = camera->GetMatrix(&mat_view);
	frustum_planes.Set(mat_view_proj);
	ListVisibleNodes();
	DrawSkybox();
	DrawNodes();
	DrawParticles();
	if(debug_draw_enabled)
		debug_drawer->Draw(mat_view, mat_view_proj, camera->from, debug_draw_handler);
}

void Scene::DrawSkybox()
{
	if(!skybox && !sky)
		return;

	Matrix mat_combined = Matrix::Translation(camera->from) * mat_view_proj;

	if(skybox)
		skybox_shader->Draw(skybox, mat_combined);
	if(sky)
		sky_shader->Draw(sky, mat_combined);
}

void Scene::DrawNodes()
{
	render->SetAlphaBlend(Render::BLEND_NO);
	render->SetDepthState(Render::DEPTH_YES);
	render->SetCulling(true);
	mesh_shader->Prepare(fog_color, fog_params, light_dir, light_color, ambient_color);

	DrawNodes(visible_nodes, nullptr, Vec4::One);

	if(!visible_alpha_nodes.empty())
	{
		render->SetAlphaBlend(Render::BLEND_NORMAL);
		render->SetDepthState(Render::DEPTH_READONLY);
		DrawNodes(visible_alpha_nodes, nullptr, Vec4::One);
	}
}

void Scene::DrawNodes(vector<SceneNode*>& nodes, const Matrix* parent_matrix, const Vec4& parent_tint)
{
	Matrix mat_world, mat_combined;

	for(SceneNode* node : nodes)
	{
		if(node->parent_point && node->parent_point != SceneNode::USE_PARENT_BONES)
		{
			assert(node->parent && parent_matrix);
			Mesh::Point* point = (Mesh::Point*)node->parent_point;
			mat_world = point->mat
				* node->parent->mesh_inst->GetMatrixBones().at(point->bone)
				* (*parent_matrix);
		}
		else
		{
			// convert right handed rotation to left handed
			if(node->use_matrix)
				mat_world = node->mat;
			else
				mat_world = Matrix::Scale(node->scale)
					* Matrix::Rotation(-node->rot.y, node->rot.x, node->rot.z)
					* Matrix::Translation(node->pos);
			if(parent_matrix)
				mat_world *= (*parent_matrix);
		}
		mat_combined = mat_world * mat_view_proj;

		if(node->visible)
			mesh_shader->DrawMesh(node->mesh, node->GetMeshInstance(), mat_combined, mat_world, node->tint * parent_tint, node->subs);

		if(!node->childs.empty())
			DrawNodes(node->childs, &mat_world, node->tint);
	}
}

void Scene::DrawParticles()
{
	if(visible_pes.empty())
		return;

	particle_shader->Prepare(mat_view, mat_view_proj);
	for(ParticleEmitter* pe : visible_pes)
	{
		if(pe->alive > 0)
			particle_shader->Draw(pe);
	}
}

void Scene::Update(float dt)
{
	UpdateNodes(nodes, dt);

	LoopRemove(pes, [&](ParticleEmitter* pe)
	{
		if(!pe->Update(dt))
		{
			pe->Free();
			return true;
		}
		return false;
	});
}

void Scene::UpdateNodes(vector<SceneNode*>& nodes, float dt)
{
	for(SceneNode* node : nodes)
	{
		if(node->mesh_inst)
			node->mesh_inst->Update(dt);
		if(!node->childs.empty())
			UpdateNodes(node->childs, dt);
	}
}

void Scene::Add(SceneNode* node)
{
	// not null, have mesh or is container, don't have parent
	assert(node && (node->mesh || node->container) && !node->parent);
	nodes.push_back(node);
}

void Scene::Add(ParticleEmitter* pe)
{
	assert(pe);
	pe->Init();
	pes.push_back(pe);
}

void Scene::Remove(SceneNode* node)
{
	assert(node);
	DeleteElement(nodes, node);
}

void Scene::SetFogParams(float start, float end)
{
	assert(start >= 0.f && start < end);
	fog_params.x = start;
	fog_params.y = end;
	fog_params.z = end - start;
}

void Scene::ListVisibleNodes()
{
	visible_nodes.clear();
	visible_alpha_nodes.clear();
	visible_pes.clear();

	ListVisibleNodes(nodes);

	for(ParticleEmitter* pe : pes)
	{
		if(frustum_planes.SphereToFrustum(pe->pos, pe->radius))
			visible_pes.push_back(pe);
	}

	if(quad_tree)
	{
		quad_tree->ListVisibleParts(frustum_planes, (vector<QuadTree::Part*>&)parts);

		for(ScenePart* part : parts)
			ListVisibleNodes(part->nodes);
	}
}

void Scene::ListVisibleNodes(vector<SceneNode*>& nodes)
{
	for(SceneNode* node : nodes)
	{
		if(node->container)
		{
			if(node->container->is_sphere)
			{
				if(frustum_planes.SphereToFrustum(node->pos, node->container->radius))
					ListVisibleNodes(node->childs);
			}
			else
			{
				if(frustum_planes.BoxToFrustum(node->container->box))
					ListVisibleNodes(node->childs);
			}
		}
		else
		{
			bool ok = false;
			if(node->use_matrix)
			{
				Vec3 center = Vec3::TransformZero(node->mat);
				if(frustum_planes.SphereToFrustum(center, node->mesh->head.radius))
					ok = true;
			}
			else
			{
				if(frustum_planes.SphereToFrustum(node->pos, node->mesh->head.radius * node->scale))
					ok = true;
			}

			if(ok)
			{
				if(node->alpha)
					visible_alpha_nodes.push_back(node);
				else
					visible_nodes.push_back(node);
			}
		}
	}
}

void Scene::InitQuadTree(float size, uint splits)
{
	if(quad_tree)
		return;

	quad_tree.reset(new QuadTree);
	quad_tree->Init(size, splits, &scene_part_factory);
}

void Scene::RecycleMeshInstance(SceneNode* node)
{
	assert(node && node->mesh);
	if(node->mesh->IsAnimated())
	{
		if(node->mesh_inst)
		{
			if(node->mesh_inst->GetMesh() == node->mesh)
				return;
			else
			{
				node->mesh_inst->Reset();
				mesh_inst_pool.push_back(node->mesh_inst);
			}
		}

		for(auto it = mesh_inst_pool.begin(), end = mesh_inst_pool.end(); it != end; ++it)
		{
			if((*it)->GetMesh() == node->mesh)
			{
				node->mesh_inst = *it;
				std::iter_swap(it, end - 1);
				mesh_inst_pool.pop_back();
				return;
			}
		}
		node->mesh_inst = new MeshInstance(node->mesh);
	}
	else
	{
		if(node->mesh_inst)
		{
			node->mesh_inst->Reset();
			mesh_inst_pool.push_back(node->mesh_inst);
			node->mesh_inst = nullptr;
		}
	}
}

void Scene::OnChangeResolution(const Int2& wnd_size)
{
	camera->aspect = float(wnd_size.x) / wnd_size.y;
}
