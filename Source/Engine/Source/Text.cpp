#include "Core.h"
#include <stdarg.h>

static const uint FORMAT_STRINGS = 8;
static const uint FORMAT_LENGTH = 2048;
static char format_buf[FORMAT_STRINGS][FORMAT_LENGTH];
static int format_marker;

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

//=================================================================================================
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
