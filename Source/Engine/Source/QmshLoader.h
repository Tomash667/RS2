#pragma once

class QmshLoader
{
public:
	QmshLoader(ResourceManager* res_mgr, ID3D11Device* device, ID3D11DeviceContext* device_context);
	Mesh* Load(cstring name, cstring path);

private:
	void LoadInternal(Mesh& mesh, FileReader& f);

	ResourceManager* res_mgr;
	ID3D11Device* device;
	ID3D11DeviceContext* device_context;
	vector<byte> buf;
};
