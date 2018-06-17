#include "Core.h"
#include "Config.h"
#include "Tokenizer.h"

ObjectPool<string> StringPool;

Config::Config(Cstring path)
{

}

Config::~Config()
{

}

void Config::Load()
{
	Tokenizer t;
	try
	{
		t.Next();
		const string& item = t.MustGetItem();
		Var* var = TryGet(item);
		if(!var)
		{

		}
	}
	catch(Tokenizer::Exception& e)
	{

	}
}

void Config::Save()
{

}

void Config::Add(Var&& var)
{

}

Config::Var* Config::TryGet(cstring name)
{

}

int Config::SplitCommandLine(char* cmd_line, char*** out)
{
	int argc = 0;
	char* str = cmd_line;

	// count argument count
	while(*str)
	{
		while(*str && *str == ' ')
			++str;

		if(*str)
		{
			if(*str == '"')
			{
				++str;
				while(*str && *str != '"')
					++str;
				++str;
				++argc;
			}
			else
			{
				++argc;
				while(*str && *str != ' ')
					++str;
			}
		}
	}

	// split arguments
	char** argv = new char*[argc];
	char** cargv = argv;
	str = cmd_line;
	while(*str)
	{
		while(*str && *str == ' ')
			++str;

		if(*str)
		{
			if(*str == '"')
			{
				++str;
				*cargv = str;
				++cargv;

				while(*str && *str != '"')
					++str;
			}
			else
			{
				*cargv = str;
				++cargv;

				while(*str && *str != ' ')
					++str;
			}
		}

		if(*str)
		{
			*str = 0;
			++str;
		}
	}

	*out = argv;
	return argc;
}
