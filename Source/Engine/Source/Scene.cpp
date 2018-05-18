#include "EngineCore.h"
#include "Scene.h"
#include "SceneNode.h"
#include "Render.h"
#include "Camera.h"
#include "MeshShader.h"
#include "AnimatedShader.h"
#include "MeshInstance.h"
#include "ParticleEmitter.h"
#include "ParticleShader.h"
#include "QuadTree.h"
#include "ScenePart.h"

Scene::Scene() : fog_color(Color::White), fog_params(1000, 2000, 1000, 0)
{
	camera.reset(new Camera);
}

Scene::~Scene()
{
	DeleteElements(nodes);
	ParticleEmitter::Free(pes);
}

void Scene::Init(Render* render)
{
	assert(render);
	this->render = render;

	mesh_shader.reset(new MeshShader(render));
	mesh_shader->Init();

	animated_shader.reset(new AnimatedShader(render));
	animated_shader->Init();

	particle_shader.reset(new ParticleShader(render));
	particle_shader->Init();
}

void Scene::Draw()
{
	mat_view_proj = camera->GetMatrix(&mat_view);
	frustum_planes.Set(mat_view_proj);
	ListVisibleNodes();
	DrawNodes();
	DrawParticles();
}

void Scene::DrawNodes()
{
	render->SetAlphaBlend(false);
	render->SetDepthState(Render::DEPTH_YES);
	render->SetCulling(true);

	mode = -1;
	mesh_shader->SetParams(fog_color, fog_params);
	animated_shader->SetParams(fog_color, fog_params);
	DrawNodes(visible_nodes, nullptr);
}

void Scene::DrawNodes(vector<SceneNode*>& nodes, const Matrix* parent_matrix)
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
			mat_world = Matrix::Scale(node->scale)
				* Matrix::Rotation(-node->rot.y, node->rot.x, node->rot.z)
				* Matrix::Translation(node->pos);
			if(parent_matrix)
				mat_world *= (*parent_matrix);
		}
		mat_combined = mat_world * mat_view_proj;

		if(node->visible)
		{
			MeshInstance* mesh_inst = node->GetMeshInstance();
			if(mesh_inst)
			{
				if(mode != 1)
				{
					animated_shader->Prepare();
					mode = 1;
				}
				animated_shader->DrawMesh(node->mesh, mesh_inst, mat_combined, node->tint, node->subs);
			}
			else
			{
				if(mode != 0)
				{
					mesh_shader->Prepare();
					mode = 0;
				}
				mesh_shader->DrawMesh(node->mesh, mat_combined, node->tint, node->subs);
			}
		}

		if(!node->childs.empty())
			DrawNodes(node->childs, &mat_world);
	}
}

void Scene::DrawParticles()
{
	if(visible_pes.empty())
		return;

	render->SetAlphaBlend(true);
	render->SetDepthState(Render::DEPTH_READONLY);
	render->SetCulling(false);

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
		else if(frustum_planes.SphereToFrustum(node->pos, node->mesh->head.radius * node->scale))
			visible_nodes.push_back(node);
	}
}

void Scene::InitQuadTree(float size, uint splits)
{
	if(quad_tree)
		return;

	quad_tree.reset(new QuadTree);
	quad_tree->Init(size, splits, [] { new ScenePart; });
}
