#pragma once

#include "Core.h"

// engine
class DebugDrawer;
class Engine;
class GameHandler;
class Gui;
class Input;
class Render;
class ResourceManager;
class Scene;
class SoundManager;
class Window;

// entities
struct Camera;
struct MeshInstance;
struct ParticleEmitter;
struct SceneNode;
struct ScenePart;
struct Shader;
struct Sky;

// resources
struct Font;
struct Mesh;
struct MeshBuilder;
struct MeshPoint;
struct Music;
struct Resource;
struct Sound;
struct Texture;

// gui controls
struct Button;
struct CheckBox;
struct Container;
struct Control;
struct DialogBox;
struct DropDownList;
struct Label;
struct ListBox;
struct Panel;
struct ProgressBar;
struct ScrollBar;
struct Sprite;
struct TextBox;

// internal classes
class DebugShader;
class FontLoader;
class GuiShader;
class MeshShader;
class ParticleShader;
class QmshLoader;
class SkyboxShader;
class SkyShader;
class SoundLoader;
class TextureLoader;

// directx
struct IDXGIFactory;
struct IDXGIAdapter;
struct IDXGISwapChain;
struct ID3D11Device;
struct ID3D11DeviceContext;
struct ID3D11RenderTargetView;
struct ID3D11DepthStencilView;
struct ID3D11DepthStencilState;
struct ID3D11RasterizerState;
struct ID3D11VertexShader;
struct ID3D11PixelShader;
struct ID3D11InputLayout;
struct ID3D11Buffer;
struct ID3D10Blob;
struct ID3D11ShaderResourceView;
struct ID3D11SamplerState;
struct ID3D11Texture2D;
struct ID3D11BlendState;
struct D3D11_INPUT_ELEMENT_DESC;
typedef ID3D10Blob ID3DBlob;

// fmod
namespace FMOD
{
	class Channel;
	class ChannelGroup;
	class Sound;
	class System;
}
