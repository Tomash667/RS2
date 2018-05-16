#include "EngineCore.h"
#include "Shader.h"
#include <d3d11_1.h>
#include "InternalHelper.h"

Shader::Shader() : vertex_shader(nullptr), pixel_shader(nullptr), layout(nullptr), vs_buffer(nullptr), ps_buffer(nullptr)
{
}

Shader::~Shader()
{
	SafeRelease(vertex_shader);
	SafeRelease(pixel_shader);
	SafeRelease(layout);
	SafeRelease(vs_buffer);
	SafeRelease(ps_buffer);
}
