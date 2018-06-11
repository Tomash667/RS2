#pragma once

class QmshLoader
{
public:
	QmshLoader(ResourceManager* res_mgr, ID3D11Device* device, ID3D11DeviceContext* device_context);
	Mesh* Load(cstring name, cstring path);
	Mesh* Create(MeshInfo* mesh_info);

private:
	void LoadInternal(Mesh& mesh, FileReader& f);
	void CreateInternal(Mesh& mesh, MeshInfo& info);

	ResourceManager* res_mgr;
	ID3D11Device* device;
	ID3D11DeviceContext* device_context;
	vector<byte> buf;
};
