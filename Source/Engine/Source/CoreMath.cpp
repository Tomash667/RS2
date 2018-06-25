#include "Core.h"

#define FLOAT_ALMOST_ZERO(F) ((absolute_cast<unsigned>(F) & 0x7f800000L) == 0)

std::minstd_rand internal::rng;

const Int2 Int2::Zero = { 0,0 };
const Int2 Int2::MaxValue = { std::numeric_limits<int>::max(), std::numeric_limits<int>::max() };

const Rect Rect::Zero = { 0,0,0,0 };

const Vec2 Vec2::Zero = { 0.f, 0.f };
const Vec2 Vec2::One = { 1.f, 1.f };
const Vec2 Vec2::UnitX = { 1.f, 0.f };
const Vec2 Vec2::UnitY = { 0.f, 1.f };

const Vec3 Vec3::Zero = { 0.f, 0.f, 0.f };
const Vec3 Vec3::One = { 1.f, 1.f, 1.f };
const Vec3 Vec3::UnitX = { 1.f, 0.f, 0.f };
const Vec3 Vec3::UnitY = { 0.f, 1.f, 0.f };
const Vec3 Vec3::UnitZ = { 0.f, 0.f, 1.f };
const Vec3 Vec3::Up = { 0.f, 1.f, 0.f };
const Vec3 Vec3::Down = { 0.f, -1.f, 0.f };
const Vec3 Vec3::Right = { 1.f, 0.f, 0.f };
const Vec3 Vec3::Left = { -1.f, 0.f, 0.f };
const Vec3 Vec3::Forward = { 0.f, 0.f, -1.f };
const Vec3 Vec3::Backward = { 0.f, 0.f, 1.f };

const Vec4 Vec4::Zero = { 0.f, 0.f, 0.f, 0.f };
const Vec4 Vec4::One = { 1.f, 1.f, 1.f, 1.f };
const Vec4 Vec4::UnitX = { 1.f, 0.f, 0.f, 0.f };
const Vec4 Vec4::UnitY = { 0.f, 1.f, 0.f, 0.f };
const Vec4 Vec4::UnitZ = { 0.f, 0.f, 1.f, 0.f };
const Vec4 Vec4::UnitW = { 0.f, 0.f, 0.f, 1.f };

const Box2d Box2d::Zero = { 0.f, 0.f, 0.f, 0.f };
const Box2d Box2d::Unit = { 0.f, 0.f, 1.f, 1.f };

const Matrix Matrix::IdentityMatrix = {
	1.f, 0.f, 0.f, 0.f,
	0.f, 1.f, 0.f, 0.f,
	0.f, 0.f, 1.f, 0.f,
	0.f, 0.f, 0.f, 1.f
};

const Quat Quat::Identity = { 0.f, 0.f, 0.f, 1.f };


//=================================================================================================
void Srand()
{
	std::random_device rdev;
	internal::rng.seed(rdev());
}

//=================================================================================================
float Angle(float x1, float y1, float x2, float y2)
{
	float x = x2 - x1;
	float y = y2 - y1;

	if(x == 0)
	{
		if(y == 0)
			return 0;
		else if(y > 0)
			return PI / 2.f;
		else
			return PI * 3.f / 2.f;
	}
	else if(y == 0)
	{
		if(x > 0)
			return 0;
		else
			return PI;
	}
	else
	{
		if(x < 0)
			return atan(y / x) + PI;
		else if(y < 0)
			return atan(y / x) + (2 * PI);
		else
			return atan(y / x);
	}
}

//=================================================================================================
float ShortestArc(float a, float b)
{
	if(fabs(b - a) < PI)
		return b - a;
	if(b > a)
		return b - a - PI * 2.0f;
	return b - a + PI * 2.0f;
}

//=================================================================================================
void LerpAngle(float& angle, float from, float to, float t)
{
	if(to > angle)
	{
		while(to - angle > PI)
			to -= PI * 2;
	}
	else
	{
		while(to - angle < -PI)
			to += PI * 2;
	}

	angle = from + t * (to - from);
}

struct MATRIX33
{
	Vec3 v[3];

	Vec3& operator [] (int n)
	{
		return v[n];
	}
};

//=================================================================================================
bool Oob::Collide(const Oob& a, const Oob& b)
{
	const float EPSILON = std::numeric_limits<float>::epsilon();

	float ra, rb;
	MATRIX33 R, AbsR;
	// Compute rotation matrix expressing b in a’s coordinate frame
	for(int i = 0; i < 3; i++)
		for(int j = 0; j < 3; j++)
			R[i][j] = a.u[i].Dot(b.u[j]);
	// Compute translation vector t
	Vec3 t = b.c - a.c;
	// Bring translation into a’s coordinate frame
	t = Vec3(t.Dot(a.u[0]), t.Dot(a.u[2]), t.Dot(a.u[2]));
	// Compute common subexpressions. Add in an epsilon term to
	// counteract arithmetic errors when two edges are parallel and
	// their cross product is (near) null (see text for details)
	for(int i = 0; i < 3; i++)
		for(int j = 0; j < 3; j++)
			AbsR[i][j] = abs(R[i][j]) + EPSILON;
	// Test axes L = A0, L = A1, L = A2
	for(int i = 0; i < 3; i++) {
		ra = a.e[i];
		rb = b.e[0] * AbsR[i][0] + b.e[1] * AbsR[i][1] + b.e[2] * AbsR[i][2];
		if(abs(t[i]) > ra + rb) return false;
	}
	// Test axes L = B0, L = B1, L = B2
	for(int i = 0; i < 3; i++) {
		ra = a.e[0] * AbsR[0][i] + a.e[1] * AbsR[1][i] + a.e[2] * AbsR[2][i];
		rb = b.e[i];
		if(abs(t[0] * R[0][i] + t[1] * R[1][i] + t[2] * R[2][i]) > ra + rb) return false;
	}
	// Test axis L = A0 x B0
	ra = a.e[1] * AbsR[2][0] + a.e[2] * AbsR[1][0];
	rb = b.e[1] * AbsR[0][2] + b.e[2] * AbsR[0][1];
	if(abs(t[2] * R[1][0] - t[1] * R[2][0]) > ra + rb) return false;
	// Test axis L = A0 x B1
	ra = a.e[1] * AbsR[2][1] + a.e[2] * AbsR[1][1];
	rb = b.e[0] * AbsR[0][2] + b.e[2] * AbsR[0][0];
	if(abs(t[2] * R[1][1] - t[1] * R[2][1]) > ra + rb) return false;
	// Test axis L = A0 x B2
	ra = a.e[1] * AbsR[2][2] + a.e[2] * AbsR[1][2];
	rb = b.e[0] * AbsR[0][1] + b.e[1] * AbsR[0][0];
	if(abs(t[2] * R[1][2] - t[1] * R[2][2]) > ra + rb) return false;
	// Test axis L = A1 x B0
	ra = a.e[0] * AbsR[2][0] + a.e[2] * AbsR[0][0];
	rb = b.e[1] * AbsR[1][2] + b.e[2] * AbsR[1][1];
	if(abs(t[0] * R[2][0] - t[2] * R[0][0]) > ra + rb) return false;
	// Test axis L = A1 x B1
	ra = a.e[0] * AbsR[2][1] + a.e[2] * AbsR[0][1];
	rb = b.e[0] * AbsR[1][2] + b.e[2] * AbsR[1][0];
	if(abs(t[0] * R[2][1] - t[2] * R[0][1]) > ra + rb) return false;
	// Test axis L = A1 x B2
	ra = a.e[0] * AbsR[2][2] + a.e[2] * AbsR[0][2];
	rb = b.e[0] * AbsR[1][1] + b.e[1] * AbsR[1][0];
	if(abs(t[0] * R[2][2] - t[2] * R[0][2]) > ra + rb) return false;
	// Test axis L = A2 x B0
	ra = a.e[0] * AbsR[1][0] + a.e[1] * AbsR[0][0];
	rb = b.e[1] * AbsR[2][2] + b.e[2] * AbsR[2][1];
	if(abs(t[1] * R[0][0] - t[0] * R[1][0]) > ra + rb) return false;
	// Test axis L = A2 x B1
	ra = a.e[0] * AbsR[1][1] + a.e[1] * AbsR[0][1];
	rb = b.e[0] * AbsR[2][2] + b.e[2] * AbsR[2][0];
	if(abs(t[1] * R[0][1] - t[0] * R[1][1]) > ra + rb) return false;
	// Test axis L = A2 x B2
	ra = a.e[0] * AbsR[1][2] + a.e[1] * AbsR[0][2];
	rb = b.e[0] * AbsR[2][1] + b.e[1] * AbsR[2][0];
	if(abs(t[1] * R[0][2] - t[0] * R[1][2]) > ra + rb) return false;
	// Since no separating axis is found, the OBBs must be intersecting
	return true;
}

//=================================================================================================
Vec3 RandomPointInsideSphere(float r)
{
	Vec3 pos;
	do
	{
		pos.x = Random(-r, r);
		pos.y = Random(-r, r);
		pos.z = Random(-r, r);
	} while(pos.x * pos.x + pos.y * pos.y + pos.z * pos.z > r * r);
	return pos;
}

//=================================================================================================
bool CircleToRectangle(float circlex, float circley, float radius, float rectx, float recty, float w, float h)
{
	//
	//        /|\ -h
	//         |
	//         |
	//  -w <--(x,y)--> w
	//         |
	//         |
	//        \|/  h
	float dist_x = abs(circlex - rectx);
	float dist_y = abs(circley - recty);

	if((dist_x > (w + radius)) || (dist_y > (h + radius)))
		return false;

	if((dist_x <= w) || (dist_y <= h))
		return true;

	float dx = dist_x - w;
	float dy = dist_y - h;

	return (dx*dx + dy * dy) <= (radius*radius);
}

//=================================================================================================
bool RectangleToRectangle(const Box2d& box1, const Box2d& box2)
{
	return (box1.v1.x <= box2.v2.x) && (box1.v2.x >= box2.v1.x) && (box1.v1.y <= box2.v2.y) && (box1.v2.y >= box2.v1.y);
}

//=================================================================================================
bool RayToBox(const Vec3& ray_pos, const Vec3& ray_dir, const Box &box, float* out_t)
{
	bool inside = true;

	float xt;
	if(ray_pos.x < box.v1.x)
	{
		xt = box.v1.x - ray_pos.x;
		xt /= ray_dir.x;
		inside = false;
	}
	else if(ray_pos.x > box.v2.x)
	{
		xt = box.v2.x - ray_pos.x;
		xt /= ray_dir.x;
		inside = false;
	}
	else
		xt = -1.0f;

	float yt;
	if(ray_pos.y < box.v1.y)
	{
		yt = box.v1.y - ray_pos.y;
		yt /= ray_dir.y;
		inside = false;
	}
	else if(ray_pos.y > box.v2.y)
	{
		yt = box.v2.y - ray_pos.y;
		yt /= ray_dir.y;
		inside = false;
	}
	else
		yt = -1.0f;

	float zt;
	if(ray_pos.z < box.v1.z)
	{
		zt = box.v1.z - ray_pos.z;
		zt /= ray_dir.z;
		inside = false;
	}
	else if(ray_pos.z > box.v2.z)
	{
		zt = box.v2.z - ray_pos.z;
		zt /= ray_dir.z;
		inside = false;
	}
	else
		zt = -1.0f;

	if(inside)
	{
		*out_t = 0.0f;
		return true;
	}

	// Select the farthest plane - this is the plane of intersection
	int plane = 0;

	float t = xt;
	if(yt > t)
	{
		plane = 1;
		t = yt;
	}

	if(zt > t)
	{
		plane = 2;
		t = zt;
	}

	// Check if the point of intersection lays within the box face
	switch(plane)
	{
	case 0: // ray intersects with yz plane
		{
			float y = ray_pos.y + ray_dir.y * t;
			if(y < box.v1.y || y > box.v2.y)
				return false;
			float z = ray_pos.z + ray_dir.z * t;
			if(z < box.v1.z || z > box.v2.z)
				return false;
		}
		break;
	case 1: // ray intersects with xz plane
		{
			float x = ray_pos.x + ray_dir.x * t;
			if(x < box.v1.x || x > box.v2.x)
				return false;
			float z = ray_pos.z + ray_dir.z * t;
			if(z < box.v1.z || z > box.v2.z)
				return false;
		}
		break;
	default:
	case 2: // ray intersects with xy plane
		{
			float x = ray_pos.x + ray_dir.x * t;
			if(x < box.v1.x || x > box.v2.x)
				return false;
			float y = ray_pos.y + ray_dir.y * t;
			if(y < box.v1.y || y > box.v2.y)
				return false;
		}
		break;
	}

	*out_t = t;
	return true;
}

//=================================================================================================
bool Plane::Intersect3Planes(const Plane& P1, const Plane& P2, const Plane& P3, Vec3& OutP)
{
	float fDet;
	float MN[9] = { P1.x, P1.y, P1.z, P2.x, P2.y, P2.z, P3.x, P3.y, P3.z };
	float IMN[9] = { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };
	float MD[3] = { -P1.w, -P2.w , -P3.w };

	IMN[0] = MN[4] * MN[8] - MN[5] * MN[7];
	IMN[3] = -(MN[3] * MN[8] - MN[5] * MN[6]);
	IMN[6] = MN[3] * MN[7] - MN[4] * MN[6];

	fDet = MN[0] * IMN[0] + MN[1] * IMN[3] + MN[2] * IMN[6];

	if(FLOAT_ALMOST_ZERO(fDet))
		return false;

	IMN[1] = -(MN[1] * MN[8] - MN[2] * MN[7]);
	IMN[4] = MN[0] * MN[8] - MN[2] * MN[6];
	IMN[7] = -(MN[0] * MN[7] - MN[1] * MN[6]);
	IMN[2] = MN[1] * MN[5] - MN[2] * MN[4];
	IMN[5] = -(MN[0] * MN[5] - MN[2] * MN[3]);
	IMN[8] = MN[0] * MN[4] - MN[1] * MN[3];

	fDet = 1.0f / fDet;

	IMN[0] *= fDet;
	IMN[1] *= fDet;
	IMN[2] *= fDet;
	IMN[3] *= fDet;
	IMN[4] *= fDet;
	IMN[5] *= fDet;
	IMN[6] *= fDet;
	IMN[7] *= fDet;
	IMN[8] *= fDet;

	OutP.x = IMN[0] * MD[0] + IMN[1] * MD[1] + IMN[2] * MD[2];
	OutP.y = IMN[3] * MD[0] + IMN[4] * MD[1] + IMN[5] * MD[2];
	OutP.z = IMN[6] * MD[0] + IMN[7] * MD[1] + IMN[8] * MD[2];

	return true;
}

//=================================================================================================
void FrustumPlanes::Set(const Matrix& worldViewProj)
{
	// Left clipping plane
	planes[0].x = worldViewProj._14 + worldViewProj._11;
	planes[0].y = worldViewProj._24 + worldViewProj._21;
	planes[0].z = worldViewProj._34 + worldViewProj._31;
	planes[0].w = worldViewProj._44 + worldViewProj._41;
	planes[0].Normalize();

	// Right clipping plane
	planes[1].x = worldViewProj._14 - worldViewProj._11;
	planes[1].y = worldViewProj._24 - worldViewProj._21;
	planes[1].z = worldViewProj._34 - worldViewProj._31;
	planes[1].w = worldViewProj._44 - worldViewProj._41;
	planes[1].Normalize();

	// Top clipping plane
	planes[2].x = worldViewProj._14 - worldViewProj._12;
	planes[2].y = worldViewProj._24 - worldViewProj._22;
	planes[2].z = worldViewProj._34 - worldViewProj._32;
	planes[2].w = worldViewProj._44 - worldViewProj._42;
	planes[2].Normalize();

	// Bottom clipping plane
	planes[3].x = worldViewProj._14 + worldViewProj._12;
	planes[3].y = worldViewProj._24 + worldViewProj._22;
	planes[3].z = worldViewProj._34 + worldViewProj._32;
	planes[3].w = worldViewProj._44 + worldViewProj._42;
	planes[3].Normalize();

	// Near clipping plane
	planes[4].x = worldViewProj._13;
	planes[4].y = worldViewProj._23;
	planes[4].z = worldViewProj._33;
	planes[4].w = worldViewProj._43;
	planes[4].Normalize();

	// Far clipping plane
	planes[5].x = worldViewProj._14 - worldViewProj._13;
	planes[5].y = worldViewProj._24 - worldViewProj._23;
	planes[5].z = worldViewProj._34 - worldViewProj._33;
	planes[5].w = worldViewProj._44 - worldViewProj._43;
	planes[5].Normalize();
}

//=================================================================================================
void FrustumPlanes::GetPoints(Vec3* points) const
{
	assert(points);

	Plane::Intersect3Planes(planes[4], planes[0], planes[3], points[0]);
	Plane::Intersect3Planes(planes[4], planes[1], planes[3], points[1]);
	Plane::Intersect3Planes(planes[4], planes[0], planes[2], points[2]);
	Plane::Intersect3Planes(planes[4], planes[1], planes[2], points[3]);
	Plane::Intersect3Planes(planes[5], planes[0], planes[3], points[4]);
	Plane::Intersect3Planes(planes[5], planes[1], planes[3], points[5]);
	Plane::Intersect3Planes(planes[5], planes[0], planes[2], points[6]);
	Plane::Intersect3Planes(planes[5], planes[1], planes[2], points[7]);
}

//=================================================================================================
void FrustumPlanes::GetPoints(const Matrix& worldViewProj, Vec3* points)
{
	assert(points);

	Matrix worldViewProjInv;
	worldViewProj.Inverse(worldViewProjInv);

	Vec3 P[] = {
		Vec3(-1.f, -1.f, 0.f), Vec3(+1.f, -1.f, 0.f),
		Vec3(-1.f, +1.f, 0.f), Vec3(+1.f, +1.f, 0.f),
		Vec3(-1.f, -1.f, 1.f), Vec3(+1.f, -1.f, 1.f),
		Vec3(-1.f, +1.f, 1.f), Vec3(+1.f, +1.f, 1.f) };

	for(int i = 0; i < 8; ++i)
		points[i] = Vec3::Transform(P[i], worldViewProjInv);
}

//=================================================================================================
bool FrustumPlanes::PointInFrustum(const Vec3 &p) const
{
	for(int i = 0; i < 6; ++i)
	{
		if(planes[i].DotCoordinate(p) <= 0.f)
			return false;
	}

	return true;
}

//=================================================================================================
bool FrustumPlanes::BoxToFrustum(const Box& box) const
{
	Vec3 vmin;

	for(int i = 0; i < 6; i++)
	{
		if(planes[i].x <= 0.0f)
			vmin.x = box.v1.x;
		else
			vmin.x = box.v2.x;

		if(planes[i].y <= 0.0f)
			vmin.y = box.v1.y;
		else
			vmin.y = box.v2.y;

		if(planes[i].z <= 0.0f)
			vmin.z = box.v1.z;
		else
			vmin.z = box.v2.z;

		if(planes[i].DotCoordinate(vmin) < 0.0f)
			return false;
	}

	return true;
}

//=================================================================================================
bool FrustumPlanes::BoxToFrustum(const Box2d& box) const
{
	Vec3 vmin;

	for(int i = 0; i < 6; i++)
	{
		if(planes[i].x <= 0.0f)
			vmin.x = box.v1.x;
		else
			vmin.x = box.v2.x;

		if(planes[i].y <= 0.0f)
			vmin.y = 0.f;
		else
			vmin.y = 25.f;

		if(planes[i].z <= 0.0f)
			vmin.z = box.v1.y;
		else
			vmin.z = box.v2.y;

		if(planes[i].DotCoordinate(vmin) < 0.0f)
			return false;
	}

	return true;
}

//=================================================================================================
bool FrustumPlanes::BoxInFrustum(const Box& box) const
{
	if(!PointInFrustum(box.v1)) return false;
	if(!PointInFrustum(box.v2)) return false;
	if(!PointInFrustum(Vec3(box.v2.x, box.v1.y, box.v1.z))) return false;
	if(!PointInFrustum(Vec3(box.v1.x, box.v2.y, box.v1.z))) return false;
	if(!PointInFrustum(Vec3(box.v2.x, box.v2.y, box.v1.z))) return false;
	if(!PointInFrustum(Vec3(box.v1.x, box.v1.y, box.v2.z))) return false;
	if(!PointInFrustum(Vec3(box.v2.x, box.v1.y, box.v2.z))) return false;
	if(!PointInFrustum(Vec3(box.v1.x, box.v2.y, box.v2.z))) return false;

	return true;
}

//=================================================================================================
bool FrustumPlanes::SphereToFrustum(const Vec3& sphere_center, float sphere_radius) const
{
	sphere_radius = -sphere_radius;

	for(int i = 0; i < 6; ++i)
	{
		if(planes[i].DotCoordinate(sphere_center) <= sphere_radius)
			return false;
	}

	return true;
}

//=================================================================================================
bool FrustumPlanes::SphereInFrustum(const Vec3& sphere_center, float sphere_radius) const
{
	for(int i = 0; i < 6; ++i)
	{
		if(planes[i].DotCoordinate(sphere_center) < sphere_radius)
			return false;
	}

	return true;
}

//=================================================================================================
// Real Time Collision Detection page 197
// Intersect segment S(t)=sa+t(sb-sa), 0<=t<=1 against cylinder specified by p, q and r
bool RayToCylinder(const Vec3& sa, const Vec3& sb, const Vec3& p, const Vec3& q, float r, float& t)
{
	Vec3 d = q - p, m = sa - p, n = sb - sa;
	float md = m.Dot(d);
	float nd = n.Dot(d);
	float dd = d.Dot(d);
	// Test if segment fully outside either endcap of cylinder
	if(md < 0.0f && md + nd < 0.0f)
		return false; // Segment outside 'p' side of cylinder
	if(md > dd && md + nd > dd)
		return false; // Segment outside 'q' side of cylinder
	float nn = n.Dot(n);
	float mn = m.Dot(n);
	float a = dd * nn - nd * nd;
	float k = m.Dot(m) - r * r;
	float c = dd * k - md * md;
	if(IsZero(a))
	{
		// Segment runs parallel to cylinder axis
		if(c > 0.0f)
			return false; // 'a' and thus the segment lie outside cylinder
		// Now known that segment intersects cylinder; figure out how it intersects
		if(md < 0.0f)
			t = -mn / nn; // Intersect segment against 'p' endcap
		else if(md > dd)
			t = (nd - mn) / nn; // Intersect segment against 'q' endcap
		else
			t = 0.0f; // 'a' lies inside cylinder
		return true;
	}
	float b = dd * mn - nd * md;
	float discr = b * b - a * c;
	if(discr < 0.0f)
		return false; // No real roots; no intersection
	t = (-b - sqrt(discr)) / a;
	if(t < 0.0f || t > 1.0f)
		return false; // Intersection lies outside segment
	if(md + t * nd < 0.0f)
	{
		// Intersection outside cylinder on 'p' side
		if(nd <= 0.0f)
			return false; // Segment pointing away from endcap
		t = -md / nd;
		// Keep intersection if Dot(S(t) - p, S(t) - p) <= r /\ 2
		return k + 2 * t * (mn + t * nn) <= 0.0f;
	}
	else if(md + t * nd > dd)
	{
		// Intersection outside cylinder on 'q' side
		if(nd >= 0.0f)
			return false; // Segment pointing away from endcap
		t = (dd - md) / nd;
		// Keep intersection if Dot(S(t) - q, S(t) - q) <= r /\ 2
		return k + dd - 2 * md + t * (2 * (mn - nd) + t * nn) <= 0.0f;
	}
	// Segment intersects cylinder between the endcaps; t is correct
	return true;
}

//=================================================================================================
void math::SphericalToCartesian(Vec3& out, float yaw, float pitch, float r)
{
	float sy = sinf(yaw), cy = cosf(yaw);
	float sp = sinf(pitch), cp = cosf(pitch);

	out.x = r * cp * cy;
	out.y = r * sp;
	out.z = -r * cp * sy;
}
