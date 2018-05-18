#pragma once

struct Resource
{
	enum class Type
	{
		Texture,
		Mesh,
		Font,
		Sound,
		Music
	};

	Resource() {}
	Resource(cstring name, Type type) : name(name), type(type) {}
	virtual ~Resource() {}

	string name;
	Type type;
};
