#pragma once

//-----------------------------------------------------------------------------
struct Cstring
{
	Cstring(cstring s) : s(s)
	{
		assert(s);
	}
	Cstring(const string& str) : s(str.c_str())
	{
	}

	operator cstring() const
	{
		return s;
	}

	cstring s;
};

inline bool operator == (const string& s1, const Cstring& s2)
{
	return s1 == s2.s;
}
inline bool operator == (const Cstring& s1, const string& s2)
{
	return s2 == s1.s;
}
inline bool operator == (cstring s1, const Cstring& s2)
{
	return strcmp(s1, s2.s) == 0;
}
inline bool operator == (const Cstring& s1, cstring s2)
{
	return strcmp(s1.s, s2) == 0;
}
inline bool operator != (const string& s1, const Cstring& s2)
{
	return s1 != s2.s;
}
inline bool operator != (const Cstring& s1, const string& s2)
{
	return s2 != s1.s;
}
inline bool operator != (cstring s1, const Cstring& s2)
{
	return strcmp(s1, s2.s) != 0;
}
inline bool operator != (const Cstring& s1, cstring s2)
{
	return strcmp(s1.s, s2) != 0;
}

//-----------------------------------------------------------------------------
cstring Format(cstring fmt, ...);
const wchar_t* ToWString(cstring str);
cstring ToString(const wchar_t* wstr);
