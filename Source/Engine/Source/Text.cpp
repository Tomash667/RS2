#include "Core.h"
#include <stdarg.h>

static const uint FORMAT_STRINGS = 8;
static const uint FORMAT_LENGTH = 2048;
static char format_buf[FORMAT_STRINGS][FORMAT_LENGTH];
static int format_marker;

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

const wchar_t* ToWString(cstring str)
{
	assert(str);
	size_t len;
	wchar_t* wstr = (wchar_t*)format_buf[format_marker];
	mbstowcs_s(&len, wstr, FORMAT_LENGTH / 2, str, (FORMAT_LENGTH - 1) / 2);
	format_marker = (format_marker + 1) % FORMAT_STRINGS;
	return wstr;
}

cstring ToString(const wchar_t* wstr)
{
	assert(wstr);
	size_t len;
	char* str = format_buf[format_marker];
	wcstombs_s(&len, str, FORMAT_LENGTH, wstr, FORMAT_LENGTH - 1);
	format_marker = (format_marker + 1) % FORMAT_STRINGS;
	return str;
}
