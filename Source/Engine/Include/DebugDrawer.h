#pragma once

class DebugDrawer
{
public:
	DebugDrawer(Render* render);
	~DebugDrawer();
	void Init();
	void Draw(const Matrix& mat_view_proj, delegate<void(DebugDrawer*)> handler);
	void DrawQuad(const Vec3 pos[4]);

	void SetColor(Color color);

	Color GetColor() { return color; }

private:
	Render* render;
	unique_ptr<DebugShader> shader;
	Color color;
};
