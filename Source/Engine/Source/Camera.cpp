#include "EngineCore.h"
#include "Camera.h"

Camera::Camera() : from(0, 1, -3), to(0, 0, 0), up(0, 1, 0), fov(PI / 4), aspect(1024.f / 768), znear(0.1f), zfar(100.f)
{
}

Matrix Camera::GetMatrix(Matrix* mat_view)
{
	Matrix m = Matrix::CreateLookAt(from, to, up);
	if(mat_view)
		*mat_view = m;
	return m * Matrix::CreatePerspectiveFieldOfView(fov, aspect, znear, zfar);
}
