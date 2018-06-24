#include "Core.h"
#include <stdarg.h>

static const uint FORMAT_STRINGS = 8;
static const uint FORMAT_LENGTH = 2048;
static char format_buf[FORMAT_STRINGS][FORMAT_LENGTH];
static int format_marker;
static char escape_from[] = { '\n', '\t', '\r', ' ' };
static cstring escape_to[] = { "\\n", "\\t", "\\r", " " };

//=================================================================================================
// Return char if it's on list or 0
char CharInStr(char c, cstring chrs)
{
	assert(chrs);

	while(true)
	{
		char c2 = *chrs++;
		if(c2 == 0)
			return 0;
		if(c == c2)
			return c;
	}
}

//=================================================================================================
cstring Escape(Cstring s, char quote)
{
	cstring str = s.s;
	char* out = format_buf[format_marker];
	char* out_buf = out;
	cstring from = "\n\t\r";
	cstring to = "ntr";

	char c;
	while((c = *str) != 0)
	{
		int index = StrCharIndex(from, c);
		if(index == -1)
		{
			if(c == quote)
				*out++ = '\\';
			*out++ = c;
		}
		else
		{
			*out++ = '\\';
			*out++ = to[index];
		}
		++str;
	}

	*out = 0;
	out_buf[FORMAT_LENGTH - 1] = 0;
	format_marker = (format_marker + 1) % FORMAT_STRINGS;
	return out_buf;
}

//=================================================================================================
cstring Escape(Cstring str, string& out, char quote)
{
	cstring s = str.s;
	out.clear();
	cstring from = "\n\t\r";
	cstring to = "ntr";

	char c;
	while((c = *s) != 0)
	{
		int index = StrCharIndex(from, c);
		if(index == -1)
		{
			if(c == quote)
				out += '\\';
			out += c;
		}
		else
		{
			out += '\\';
			out += to[index];
		}
		++s;
	}

	return out.c_str();
}

//=================================================================================================
cstring EscapeChar(char c)
{
	char* out = format_buf[format_marker];
	for(uint i = 0; i < countof(escape_from); ++i)
	{
		if(c == escape_from[i])
		{
			strcpy(out, escape_to[i]);
			format_marker = (format_marker + 1) % FORMAT_STRINGS;
			return out;
		}
	}

	if(isprint(c))
	{
		out[0] = c;
		out[1] = 0;
	}
	else
		_snprintf_s(out, FORMAT_LENGTH, FORMAT_LENGTH - 1, "0x%u", (uint)c);

	format_marker = (format_marker + 1) % FORMAT_STRINGS;
	return out;
}

//=================================================================================================
cstring EscapeChar(char c, string& out)
{
	cstring esc = EscapeChar(c);
	out = esc;
	return out.c_str();
}

//=================================================================================================
cstring Format(cstring str, ...)
{
	assert(str);

	va_list list;
	va_start(list, str);
	char* cbuf = format_buf[format_marker];
	_vsnprintf_s(cbuf, FORMAT_LENGTH, FORMAT_LENGTH - 1, str, list);
	cbuf[FORMAT_LENGTH - 1] = 0;
	format_marker = (format_marker + 1) % FORMAT_STRINGS;
	va_end(list);

	return cbuf;
}

//=================================================================================================
cstring FormatList(cstring str, va_list list)
{
	assert(str);

	char* cbuf = format_buf[format_marker];
	_vsnprintf_s(cbuf, FORMAT_LENGTH, FORMAT_LENGTH - 1, str, list);
	cbuf[FORMAT_LENGTH - 1] = 0;
	format_marker = (format_marker + 1) % FORMAT_STRINGS;

	return cbuf;
}

//=================================================================================================
// Return index of char c in string
int StrCharIndex(cstring str, char c)
{
	int index = 0;
	do
	{
		if(*str == c)
			return index;
		++index;
		++str;
	}
	while(*str);

	return -1;
}

//=================================================================================================
// Returns true if string contains string
bool StringInString(cstring s1, cstring s2)
{
	while(true)
	{
		if(*s1 == *s2)
		{
			++s1;
			++s2;
			if(*s2 == 0)
				return true;
		}
		else
			return false;
	}
}

//=================================================================================================
bool StringToBool(cstring s, bool& result)
{
	if(_stricmp(s, "true") == 0)
	{
		result = true;
		return true;
	}
	else if(_stricmp(s, "false") == 0)
	{
		result = false;
		return true;
	}
	else
	{
		int value;
		if(!StringToInt(s, value) && value != 0 && value != 1)
			return false;
		result = (value == 1);
		return true;
	}
}

//=================================================================================================
bool StringToFloat(cstring s, float& result)
{
	__int64 i;
	float f;
	if(StringToNumber(s, i, f) != 0)
	{
		result = f;
		return true;
	}
	else
		return false;
}

//=================================================================================================
bool StringToInt(cstring s, int& result)
{
	__int64 i;
	float f;
	if(StringToNumber(s, i, f) != 0 && InRange<int>(i))
	{
		result = (int)i;
		return true;
	}
	else
		return false;
}

//=================================================================================================
int StringToNumber(cstring s, __int64& i, float& f)
{
	assert(s);

	i = 0;
	f = 0;
	uint diver = 10;
	uint digits = 0;
	char c;
	bool sign = false;
	if(*s == '-')
	{
		sign = true;
		++s;
	}

	while((c = *s) != 0)
	{
		if(c == '.')
		{
			++s;
			break;
		}
		else if(c >= '0' && c <= '9')
		{
			i *= 10;
			i += (int)c - '0';
		}
		else
			return 0;
		++s;
	}

	if(c == 0)
	{
		if(sign)
			i = -i;
		f = (float)i;
		return 1;
	}

	while((c = *s) != 0)
	{
		if(c == 'f')
		{
			if(digits == 0)
				return 0;
			break;
		}
		else if(c >= '0' && c <= '9')
		{
			++digits;
			f += ((float)((int)c - '0')) / diver;
			diver *= 10;
		}
		else
			return 0;
		++s;
	}
	f += (float)i;
	if(sign)
	{
		f = -f;
		i = -i;
	}
	return 2;
}

//=================================================================================================
bool StringToUint(cstring s, uint& result)
{
	__int64 i;
	float f;
	if(StringToNumber(s, i, f) != 0 && InRange<uint>(i))
	{
		result = (uint)i;
		return true;
	}
	else
		return false;
}

//=================================================================================================
cstring ToString(const wchar_t* wstr)
{
	assert(wstr);
	size_t len;
	char* str = format_buf[format_marker];
	wcstombs_s(&len, str, FORMAT_LENGTH, wstr, FORMAT_LENGTH - 1);
	format_marker = (format_marker + 1) % FORMAT_STRINGS;
	return str;
}

//=================================================================================================
const wchar_t* ToWString(cstring str)
{
	assert(str);
	size_t len;
	wchar_t* wstr = (wchar_t*)format_buf[format_marker];
	mbstowcs_s(&len, wstr, FORMAT_LENGTH / 2, str, (FORMAT_LENGTH - 1) / 2);
	format_marker = (format_marker + 1) % FORMAT_STRINGS;
	return wstr;
}

//=================================================================================================
bool Unescape(const string& str_in, uint pos, uint size, string& str_out)
{
	str_out.clear();
	str_out.reserve(str_in.length());

	cstring unesc = "nt\\\"'";
	cstring esc = "\n\t\\\"'";
	uint end = pos + size;

	for(; pos < end; ++pos)
	{
		if(str_in[pos] == '\\')
		{
			++pos;
			if(pos == size)
			{
				Error("Unescape error in string \"%.*s\", character '\\' at end of string.", size, str_in.c_str() + pos);
				return false;
			}
			int index = StrCharIndex(unesc, str_in[pos]);
			if(index != -1)
				str_out += esc[index];
			else
			{
				Error("Unescape error in string \"%.*s\", unknown escape sequence '\\%c'.", size, str_in.c_str() + pos, str_in[pos]);
				return false;
			}
		}
		else
			str_out += str_in[pos];
	}

	return true;
}
