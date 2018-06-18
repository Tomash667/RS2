#include "Core.h"
#include <Windows.h>

#undef DeleteFile

static DWORD tmp;
string FileReader::buf;


//-----------------------------------------------------------------------------
FileReader::~FileReader()
{
	if(own_handle && file != INVALID_HANDLE_VALUE)
	{
		CloseHandle(file);
		file = INVALID_HANDLE_VALUE;
		ok = false;
	}
}

bool FileReader::Open(cstring filename)
{
	assert(filename);
	file = CreateFile(filename, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
	own_handle = true;
	if(file != INVALID_HANDLE_VALUE)
	{
		size = GetFileSize(file, nullptr);
		ok = true;
	}
	else
	{
		size = 0;
		ok = false;
	}
	return ok;
}

void FileReader::Read(void* ptr, uint size)
{
	BOOL result = ReadFile(file, ptr, size, &tmp, nullptr);
	assert(result != FALSE);
	ok = (size == tmp);
}

void FileReader::ReadToString(string& s)
{
	DWORD size = GetFileSize(file, nullptr);
	s.resize(size);
	BOOL result = ReadFile(file, (char*)s.c_str(), size, &tmp, nullptr);
	assert(result != FALSE);
	assert(size == tmp);
}

void FileReader::Skip(uint bytes)
{
	ok = (ok && SetFilePointer(file, bytes, nullptr, FILE_CURRENT) != INVALID_SET_FILE_POINTER);
}

uint FileReader::GetPos() const
{
	return (uint)SetFilePointer(file, 0, nullptr, FILE_CURRENT);
}


//-----------------------------------------------------------------------------
FileWriter::~FileWriter()
{
	if(own_handle && file != INVALID_HANDLE_VALUE)
	{
		CloseHandle(file);
		file = INVALID_HANDLE_VALUE;
	}
}

bool FileWriter::Open(cstring filename)
{
	assert(filename);
	file = CreateFile(filename, GENERIC_WRITE, FILE_SHARE_READ, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
	return (file != INVALID_HANDLE_VALUE);
}

void FileWriter::Write(const void* ptr, uint size)
{
	WriteFile(file, ptr, size, &tmp, nullptr);
	assert(size == tmp);
}

void FileWriter::Flush()
{
	FlushFileBuffers(file);
}

uint FileWriter::GetSize() const
{
	return GetFileSize(file, nullptr);
}


//-----------------------------------------------------------------------------
namespace io
{
	bool FileExists(Cstring path)
	{
		DWORD attrib = GetFileAttributes(path);
		if(attrib == INVALID_FILE_ATTRIBUTES)
			return false;
		return !IS_SET(attrib, FILE_ATTRIBUTE_DIRECTORY);
	}

	void DeleteFile(Cstring path)
	{
		DeleteFileA(path);
	}

	bool LoadFileToString(Cstring path, string& str, uint max_size)
	{
		HANDLE file = CreateFile(path, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
		if(file == INVALID_HANDLE_VALUE)
			return false;

		uint file_size = (uint)GetFileSize(file, nullptr);
		uint size = min(file_size, max_size);
		str.resize(size);

		ReadFile(file, (char*)str.c_str(), size, &tmp, nullptr);

		CloseHandle(file);

		return size == tmp;
	}
}
