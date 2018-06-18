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
char CharInStr(char c, cstring chrs);
cstring Escape(Cstring str, char quote = '"');
cstring Escape(Cstring str, string& out, char quote = '"');
cstring EscapeChar(char c);
cstring EscapeChar(char c, string& out);
cstring Format(cstring fmt, ...);
cstring FormatList(cstring fmt, va_list lis);
string& Ltrim(string& str);
string& Rtrim(string& str);
int StrCharIndex(cstring str, char c);
bool StringInString(cstring s1, cstring s2);
bool StringToBool(cstring s, bool& result);
bool StringToFloat(cstring s, float& result);
bool StringToInt(cstring s, int& result);
int StringToNumber(cstring s, __int64& i, float& f); // 0-invalid, 1-int, 2-float
bool StringToUint(cstring s, uint& result);
cstring ToString(const wchar_t* wstr);
const wchar_t* ToWString(cstring str);
string& Trim(string& str);
string Trimmed(const string& str);
bool Unescape(const string& str_in, uint pos, uint length, string& str_out);
bool Unescape(const string& str_in, string& str_out);

//-----------------------------------------------------------------------------
inline string& Ltrim(string& str)
{
	str.erase(str.begin(), find_if(str.begin(), str.end(), [](char& ch)->bool { return !isspace(ch); }));
	return str;
}

inline string& Rtrim(string& str)
{
	str.erase(find_if(str.rbegin(), str.rend(), [](char& ch)->bool { return !isspace(ch); }).base(), str.end());
	return str;
}

inline string& Trim(string& str)
{
	return Ltrim(Rtrim(str));
}

inline string Trimmed(const string& str)
{
	string s = str;
	Trim(s);
	return s;
}

inline bool Unescape(const string& str_in, string& str_out)
{
	return Unescape(str_in, 0u, str_in.length(), str_out);
}
