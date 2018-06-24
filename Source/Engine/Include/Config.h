#pragma once

#include "Tokenizer.h"

class Config
{
public:
	enum VarType
	{
		VAR_BOOL,
		VAR_INT,
		VAR_FLOAT,
		VAR_INT2,
		VAR_STRING
	};

	union Any
	{
		Any() {}
		Any(const Any& any) : int2(any.int2) {}
		Any(bool bool_) : bool_(bool_) {}
		Any(int int_) : int_(int_) {}
		Any(float float_) : float_(float_) {}
		Any(const Int2& int2) : int2(int2) {}
		Any(Cstring s)
		{
			str = StringPool.Get();
			*str = s;
		}
		Any& operator = (const Any& any) { int2 = any.int2; return *this; }

		bool bool_;
		int int_;
		float float_;
		Int2 int2;
		string* str;
	};

	struct Var
	{
		VarType type;
		Any value, prev_value;
		cstring name;
		string* name_ptr;

		Var() {}
		Var(cstring name, bool value) : name(name), type(VAR_BOOL), value(value), prev_value(value) {}
		Var(cstring name, int value) : name(name), type(VAR_INT), value(value), prev_value(value) {}
		Var(cstring name, float value) : name(name), type(VAR_FLOAT), value(value), prev_value(value) {}
		Var(cstring name, const Int2& value) : name(name), type(VAR_INT2), value(value), prev_value(value) {}
		Var(cstring name, Cstring str) : name(name), type(VAR_STRING), value(str), prev_value(str) {}
	};

	Config(Cstring path);
	~Config();
	void Load();
	void ParseCommandLine(cstring cmd_line);
	void Save();
	void Add(Var* var);
	Var* TryGet(cstring name);
	Var& Get(cstring name)
	{
		Var* var = TryGet(name);
		assert(var);
		return *var;
	}
	bool GetBool(cstring name)
	{
		Var& var = Get(name);
		assert(var.type == VAR_BOOL);
		return var.value.bool_;
	}
	int GetInt(cstring name)
	{
		Var& var = Get(name);
		assert(var.type == VAR_INT);
		return var.value.int_;
	}
	float GetFloat(cstring name)
	{
		Var& var = Get(name);
		assert(var.type == VAR_FLOAT);
		return var.value.float_;
	}
	const Int2& GetInt2(cstring name)
	{
		Var& var = Get(name);
		assert(var.type == VAR_INT2);
		return var.value.int2;
	}
	const string& GetString(cstring name)
	{
		Var& var = Get(name);
		assert(var.type == VAR_STRING);
		return *var.value.str;
	}
	void SetBool(cstring name, bool value)
	{
		Var& var = Get(name);
		assert(var.type == VAR_BOOL);
		var.value.bool_ = value;
	}
	void SetInt(cstring name, int value)
	{
		Var& var = Get(name);
		assert(var.type == VAR_INT);
		var.value.int_ = value;
	}
	void SetFloat(cstring name, float value)
	{
		Var& var = Get(name);
		assert(var.type == VAR_FLOAT);
		var.value.float_ = value;
	}
	void SetInt2(cstring name, const Int2& value)
	{
		Var& var = Get(name);
		assert(var.type == VAR_INT2);
		var.value.int2 = value;
	}
	void SetString(cstring name, Cstring value)
	{
		Var& var = Get(name);
		assert(var.type == VAR_STRING);
		*var.value.str = value;
	}

	static void SplitCommandLine(cstring cmd_line, vector<string>& out);

private:
	void LoadInternal();
	std::pair<Any, VarType> ParseType();
	bool CanAssignType(std::pair<Any, VarType>& var, VarType expected);
	void AssignType(std::pair<Any, VarType>& v, Var* var);
	bool HaveChanges();

	struct Comparer
	{
		bool operator()(cstring a, cstring b) const
		{
			return std::strcmp(a, b) < 0;
		}
	};
	std::map<cstring, Var*, Comparer> vars;
	Tokenizer t;
	string path;
	bool load_failed;
};
