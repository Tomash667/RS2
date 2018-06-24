#pragma once

//-----------------------------------------------------------------------------
typedef void* FileHandle;
const FileHandle INVALID_FILE_HANDLE = (FileHandle)(IntPointer)-1;

//-----------------------------------------------------------------------------
class FileReader
{
public:
	FileReader() : file(INVALID_FILE_HANDLE), own_handle(false), ok(false) {}
	explicit FileReader(FileHandle file) : file(file), own_handle(false), ok(false) {}
	explicit FileReader(cstring filename) { Open(filename); }
	~FileReader();

	bool Open(cstring filename);
	void Read(void* ptr, uint size);
	void ReadToString(string& s);
	void Skip(uint size);
	uint GetSize() const { return size; }
	uint GetPos() const;

	bool Ensure(uint elements_size) const
	{
		auto pos = GetPos();
		uint offset;
		return ok && checked::add(pos, elements_size, offset) && offset <= size;
	}
	bool Ensure(uint count, uint element_size) const
	{
		auto pos = GetPos();
		uint offset;
		return ok && checked::mad(count, element_size, pos, offset) && offset <= size;
	}

	bool IsOpen() const { return file != INVALID_FILE_HANDLE; }
	bool IsOk() const { return ok; }
	operator bool() const { return IsOk(); }

	template<typename T>
	T Read()
	{
		T a;
		Read(&a, sizeof(T));
		return a;
	}
	template<typename T>
	void Read(T& a)
	{
		Read(&a, sizeof(a));
	}
	template<typename T>
	void operator >> (T& a)
	{
		Read(&a, sizeof(a));
	}

	template<typename T, typename T2>
	void ReadCasted(T2& a)
	{
		a = (T2)Read<T>();
	}

	template<typename SizeType>
	const string& ReadString()
	{
		SizeType len = Read<SizeType>();
		if(!ok || len == 0)
			buf.clear();
		else
		{
			buf.resize(len);
			Read((char*)buf.c_str(), len);
		}
		return buf;
	}
	const string& ReadString1()
	{
		return ReadString<byte>();
	}
	const string& ReadString2()
	{
		return ReadString<word>();
	}
	const string& ReadString4()
	{
		return ReadString<uint>();
	}

	template<typename SizeType>
	void ReadString(string& s)
	{
		SizeType len = Read<SizeType>();
		if(!ok || len == 0)
			s.clear();
		else
		{
			s.resize(len);
			Read((char*)s.c_str(), len);
		}
	}
	void ReadString1(string& s)
	{
		ReadString<byte>(s);
	}
	void ReadString2(string& s)
	{
		ReadString<word>(s);
	}
	void ReadString4(string& s)
	{
		ReadString<uint>(s);
	}
	template<>
	void Read(string& s)
	{
		ReadString1(s);
	}
	void operator >> (string& s)
	{
		ReadString1(s);
	}

	template<typename T>
	void Skip(typename std::enable_if<(sizeof(T) <= 8)>::type* = 0)
	{
		T val;
		Read(val);
	}
	template<typename T>
	void Skip(typename std::enable_if<(sizeof(T) > 8)>::type* = 0)
	{
		Skip(sizeof(T));
	}

	template<typename SizeType, typename T>
	void ReadVector(vector<T>& v)
	{
		SizeType size = Read<SizeType>();
		if(!ok || size == 0)
			v.clear();
		else
		{
			v.resize(size);
			Read(v.data(), sizeof(T) * size);
		}
	}
	template<typename T>
	void ReadVector1(vector<T>& v)
	{
		ReadVector<byte>(v);
	}
	template<typename T>
	void ReadVector2(vector<T>& v)
	{
		ReadVector<word>(v);
	}
	template<typename T>
	void ReadVector4(vector<T>& v)
	{
		ReadVector<uint>(v);
	}
	template<typename T>
	void Read(vector<T>& v)
	{
		ReadVector2(v);
	}
	template<typename T>
	void operator >> (vector<T>& v)
	{
		ReadVector2(v);
	}

private:
	FileHandle file;
	uint size;
	bool own_handle, ok;
	static string buf;
};

//-----------------------------------------------------------------------------
class FileWriter
{
public:
	FileWriter() : file(INVALID_FILE_HANDLE), own_handle(true) {}
	explicit FileWriter(FileHandle file) : file(file), own_handle(false) {}
	explicit FileWriter(cstring filename) : own_handle(true) { Open(filename); }
	~FileWriter();

	bool Open(cstring filename);
	void Write(const void* ptr, uint size);
	void Flush();
	uint GetSize() const;
	bool IsOpen() const { return file != INVALID_FILE_HANDLE; }
	operator bool() const { return IsOpen(); }

	template<typename T>
	void Write(const T& a)
	{
		Write(&a, sizeof(a));
	}
	template<typename T>
	void operator << (const T& a)
	{
		Write(&a, sizeof(a));
	}

	template<typename T, typename T2>
	void WriteCasted(const T2& a)
	{
		Write(&a, sizeof(T));
	}

	template<typename SizeType>
	void WriteString(const string& s)
	{
		assert(s.length() <= std::numeric_limits<SizeType>::max());
		SizeType length = (SizeType)s.length();
		Write(length);
		Write(s.c_str(), length);
	}
	void WriteString1(const string& s)
	{
		WriteString<byte>(s);
	}
	void WriteString2(const string& s)
	{
		WriteString<word>(s);
	}
	void WriteString4(const string& s)
	{
		WriteString<uint>(s);
	}

	template<typename SizeType>
	void WriteString(cstring str)
	{
		assert(str);
		uint length = strlen(str);
		assert(length <= std::numeric_limits<SizeType>::max());
		WriteCasted<SizeType>(length);
		Write(str, length);
	}
	void WriteString1(cstring str)
	{
		WriteString<byte>(str);
	}
	void WriteString2(cstring str)
	{
		WriteString<word>(str);
	}
	void WriteString4(cstring str)
	{
		WriteString<uint>(str);
	}

	template<>
	void Write(const string& s)
	{
		WriteString1(s);
	}
	void Write(cstring str)
	{
		WriteString1(str);
	}
	void operator << (const string& s)
	{
		WriteString1(s);
	}
	void operator << (cstring str)
	{
		assert(str);
		WriteString1(str);
	}

	void Write0()
	{
		WriteCasted<byte>(0);
	}

	template<typename SizeType, typename T>
	void WriteVector(const vector<T>& v)
	{
		assert(v.size() <= (size_t)std::numeric_limits<SizeType>::max());
		SizeType size = (SizeType)v.size();
		Write(size);
		Write(v.data(), size * sizeof(T));
	}
	template<typename T>
	void WriteVector1(const vector<T>& v)
	{
		WriteVector<byte>(v);
	}
	template<typename T>
	void WriteVector2(const vector<T>& v)
	{
		WriteVector<word>(v);
	}
	template<typename T>
	void WriteVector4(const vector<T>& v)
	{
		WriteVector<short>(v);
	}
	template<typename T>
	void Write(const vector<T>& v)
	{
		WriteVector2(v);
	}
	template<typename T>
	void operator << (const vector<T>& v)
	{
		WriteVector2(v);
	}

private:
	FileHandle file;
	bool own_handle;
};

//-----------------------------------------------------------------------------
class TextWriter
{
public:
	explicit TextWriter(Cstring filename) : file(filename)
	{
	}

	operator bool() const
	{
		return file.IsOpen();
	}

	void Write(cstring str)
	{
		file.Write(str, strlen(str));
	}

	void operator << (const string& str)
	{
		file.Write(str.c_str(), str.length());
	}
	void operator << (cstring str)
	{
		file.Write(str, strlen(str));
	}
	void operator << (char c)
	{
		file << c;
	}
	void operator << (int i)
	{
		Write(Format("%d", i));
	}
	void operator << (float f)
	{
		Write(Format("%g", f));
	}
	
private:
	FileWriter file;
};

//-----------------------------------------------------------------------------
namespace io
{
	bool FileExists(Cstring path);
	void DeleteFile(Cstring path);
	bool LoadFileToString(Cstring path, string& str, uint max_size = (uint)-1);
}
