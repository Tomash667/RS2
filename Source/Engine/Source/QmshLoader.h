#pragma once

class QmshLoader
{
public:
	QmshLoader(ResourceManager* res_mgr, ID3D11Device* device, ID3D11DeviceContext* device_context);
	Mesh* Load(cstring name, cstring path, bool raw);
	Mesh* Create(MeshBuilder* mesh_builder);

private:
	void LoadInternal(Mesh& mesh, FileReader& f, bool raw);
	void CreateInternal(Mesh& mesh, MeshBuilder& builder);

	ResourceManager* res_mgr;
	ID3D11Device* device;
	ID3D11DeviceContext* device_context;
	vector<byte> buf;
	string dir;
};
