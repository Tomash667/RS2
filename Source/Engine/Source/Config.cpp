#include "Core.h"
#include "Config.h"
#include "Tokenizer.h"

ObjectPool<string> StringPool;

cstring var_type_name[] = {
	"bool",
	"int",
	"float",
	"int2",
	"string"
};


Config::Config(Cstring path) : path(path), load_failed(false)
{
	t.AddKeyword("true", 0);
	t.AddKeyword("false", 1);
}

Config::~Config()
{
	for(std::pair<const cstring, Var*>& it : vars)
	{
		Var* var = it.second;
		if(var->name_ptr)
			StringPool.Free(var->name_ptr);
		if(var->type == VAR_STRING)
		{
			StringPool.Free(var->value.str);
			StringPool.Free(var->prev_value.str);
		}
		delete var;
	}
}

void Config::Load()
{
	load_failed = false;

	try
	{
		LoadInternal();
	}
	catch(Tokenizer::Exception& e)
	{
		Error("Failed to load configuration: %s", e.ToString());
		load_failed = true;
	}
}

void Config::LoadInternal()
{
	if(!t.FromFile(path.c_str()))
	{
		// file don't exists
		load_failed = true;
		return;
	}

	if(!t.Next())
		return;

	while(true)
	{
		try
		{
			const string& item = t.MustGetItem();
			Var* var = TryGet(item.c_str());
			if(!var)
			{
				var = new Var;
				var->name_ptr = StringPool.Get();
				*var->name_ptr = item;
				var->name = var->name_ptr->c_str();
				t.Next();
				t.AssertSymbol('=');
				t.Next();
				std::pair<Any, VarType> v = ParseType();
				var->value = var->prev_value = v.first;
				var->type = v.second;
				if(var->type == VAR_STRING)
				{
					string* s = StringPool.Get();
					*s = *var->prev_value.str;
					var->prev_value.str = s;
				}
				vars[var->name] = var;
			}
			else
			{
				t.Next();
				t.AssertSymbol('=');
				t.Next();
				std::pair<Any, VarType> v = ParseType();
				if(!CanAssignType(v, var->type))
				{
					Warn("Config var '%s' type mismatch (%s and %s), ignoring value.",
						var->name, var_type_name[var->type], var_type_name[v.second]);
					continue;
				}
				AssignType(v, var);
			}

			if(!t.Next())
				break;
		}
		catch(Tokenizer::Exception& e)
		{
			load_failed = true;
			Error("Error while parsing configuration: %s", e.ToString());
			if(!t.SkipTo([](Tokenizer& t) { return t.IsItem(); }))
				break;
		}
	}
}

void Config::Save()
{
	if(!HaveChanges())
		return;

	TextWriter f(path);
	for(std::pair<const cstring, Var*>& v : vars)
	{
		Var* var = v.second;
		f << var->name;
		f << " = ";
		switch(var->type)
		{
		case VAR_BOOL:
			f << (var->value.bool_ ? "true" : "false");
			var->prev_value.bool_ = var->value.bool_;
			break;
		case VAR_INT:
			f << var->value.int_;
			var->prev_value.int_ = var->value.int_;
			break;
		case VAR_FLOAT:
			f << var->value.float_;
			var->prev_value.float_ = var->value.float_;
			break;
		case VAR_STRING:
			f << Escape(*var->value.str);
			*var->prev_value.str = *var->value.str;
			break;
		case VAR_INT2:
			f << Format("{%d;%d}", var->value.int2.x, var->value.int2.y);
			var->prev_value.int2 = var->value.int2;
			break;
		}
		f << "\n";
	}

	load_failed = false;
}

bool Config::HaveChanges()
{
	if(load_failed)
		return true;

	for(std::pair<const cstring, Var*>& v : vars)
	{
		Var& var = *v.second;
		switch(var.type)
		{
		case VAR_BOOL:
			if(var.value.bool_ != var.prev_value.bool_)
				return true;
			break;
		default:
			assert(0);
		case VAR_INT:
			if(var.value.int_ != var.prev_value.int_)
				return true;
			break;
		case VAR_FLOAT:
			if(var.value.float_ != var.prev_value.float_)
				return true;
			break;
		case VAR_INT2:
			if(var.value.int2 != var.prev_value.int2)
				return true;
			break;
		case VAR_STRING:
			if(*var.value.str != *var.prev_value.str)
				return true;
			break;
		}
	}

	return false;
}

void Config::Add(Var* var)
{
	assert(vars.find(var->name) == vars.end());
	var->name_ptr = nullptr;
	vars[var->name] = var;
}

Config::Var* Config::TryGet(cstring name)
{
	auto it = vars.find(name);
	if(it == vars.end())
		return nullptr;
	else
		return it->second;
}

std::pair<Config::Any, Config::VarType> Config::ParseType()
{
	Any any;
	VarType type;
	switch(t.GetToken())
	{
	case tokenizer::T_KEYWORD:
		any.bool_ = t.GetKeywordId() == 0;
		type = VAR_BOOL;
		break;
	case tokenizer::T_INT:
		any.int_ = t.GetInt();
		type = VAR_INT;
		break;
	case tokenizer::T_FLOAT:
		any.float_ = t.GetFloat();
		type = VAR_FLOAT;
		break;
	case tokenizer::T_STRING:
		any.str = StringPool.Get();
		*any.str = t.GetString();
		break;
	case tokenizer::T_SYMBOL:
		if(t.IsSymbol('{'))
		{
			t.Next();
			type = VAR_INT2;
			any.int2.x = t.MustGetInt();
			t.Next();
			t.AssertSymbol(';');
			t.Next();
			any.int2.y = t.MustGetInt();
			t.Next();
			t.AssertSymbol('}');
		}
		else
			t.Unexpected();
		break;
	default:
		t.Unexpected();
		break;
	}
	return std::make_pair(any, type);
}

bool Config::CanAssignType(std::pair<Any, VarType>& var, VarType expected)
{
	if(var.second == expected)
		return true;
	switch(expected)
	{
	case VAR_BOOL: // pass int 0/1 as bool
		return var.second == VAR_INT && ::Any(var.first.int_, 0, 1);
	case VAR_FLOAT: // pass int as float
		return var.second == VAR_INT;
	default:
		return false;
	}
}

void Config::AssignType(std::pair<Any, VarType>& v, Var* var)
{
	if(var->type == VAR_STRING)
	{
		*var->value.str = *v.first.str;
		*var->prev_value.str = *v.first.str;
		StringPool.Free(v.first.str);
	}
	else if(var->type == v.second)
		var->value = var->prev_value = v.first;
	else
	{
		switch(var->type)
		{
		case VAR_BOOL: // pass int 0/1 as bool
			var->value.bool_ = v.first.int_ == 1;
			var->prev_value.bool_ = var->value.bool_;
			break;
		case VAR_FLOAT: // pass int as float
			var->value.float_ = (float)v.first.int_;
			var->prev_value.float_ = var->value.float_;
		default:
			assert(0);
			break;
		}
	}
}

void Config::ParseCommandLine(cstring cmd_line)
{
	t.FromString(cmd_line);
	if(!t.Next())
		return;

	while(true)
	{
		try
		{
			if(t.IsSymbol('+'))
			{
				t.Next();
				const string& item = t.MustGetItem();
				Var* var = TryGet(item.c_str());
				if(!var)
				{
					Warn("Ignoring command line var '%s'.", item.c_str());
					t.Next();
					continue;
				}
				t.Next();
				t.AssertSymbol('=');
				t.Next();
				std::pair<Any, VarType> v = ParseType();
				if(CanAssignType(v, var->type))
					AssignType(v, var);
				else
					Warn("Command line var '%s' type mismatch (%s and %s), ignoring value.",
						var->name, var_type_name[var->type], var_type_name[v.second]);
			}
			if(!t.Next())
				return;
		}
		catch(Tokenizer::Exception& e)
		{
			Warn("Error while parsing command line: %s", e.ToString());
			if(!t.SkipTo([](Tokenizer& t) {return t.IsItem(); }))
				return;
		}
	}
}

void Config::SplitCommandLine(cstring cmd_line, vector<string>& out)
{
	cstring s = cmd_line;
	while(*s)
	{
		while(*s && *s == ' ')
			++s;

		if(*s)
		{
			if(*s == '"')
			{
				++s;
				cstring start = s;
				while(*s && *s != '"')
					++s;
				uint len = s - start;
				if(len > 0)
					out.push_back(string(start, len));
				if(*s)
					++s;
			}
			else
			{
				cstring start = s;
				while(*s && *s != ' ')
					++s;
				out.push_back(string(start, s - start));
				++s;
			}
		}
	}
}
