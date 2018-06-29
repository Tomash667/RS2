#include "EngineCore.h"
#include "Sky.h"

Sky::Sky() : time(0.5f), clouds_offset(0, 0), wind_vec(-0.25, 0.12f), clouds_scale_inv(1.f), tex_clouds_noise(nullptr), clouds_sharpness(5.f),
clouds_threshold(0.7f), clouds_color{ Vec4(1,1,1,1), Vec4(1,1,0,1) }, stars_visibility(0.f)
{
	// FIXME - no yellow clouds
	horizon_color = Vec4(0, 0, 0, 1);
	zenith_color = Vec4(0, 0, 0, 1);
	clouds_color[0] = Vec4(1, 0, 0, 1);
	clouds_color[1] = Vec4(0, 0, 1, 1);
}

void Sky::Update(float dt)
{
	clouds_offset += wind_vec * dt;
}