#pragma once

struct Camera
{
	Camera();
	Matrix GetMatrix(Matrix* mat_view = nullptr);

	Vec3 from, to, up;
	float fov, aspect, znear, zfar;
};
