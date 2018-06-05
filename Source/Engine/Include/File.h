#pragma once

//-----------------------------------------------------------------------------
typedef void* FileHandle;
const FileHandle INVALID_FILE_HANDLE = (FileHandle)(IntPointer)-1;

//-----------------------------------------------------------------------------
class FileReader
{
public:
	FileReader() : file(INVALID_FILE_HANDLE), own_handle(false), ok(false)
	{
	}

	explicit FileReader(FileHandle file) : file(file), own_handle(false), ok(false)
	{
	}

	explicit FileReader(cstring filename)
	{
		Open(filename);
	}

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
	void operator >> (T& a)
	{
		Read(&a, sizeof(a));
	}

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

	template<>
	void Read(string& s)
	{
		ReadString1(s);
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

	bool ReadString2(string& s)
	{
		ReadString<word>(s);
	}

	void operator >> (string& s)
	{
		ReadString1(s);
	}

	template<typename T>
	void ReadVector1(vector<T>& v)
	{
		byte count;
		Read(count);
		v.resize(count);
		if(count)
			Read(&v[0], sizeof(T)*count);
	}

	template<typename T>
	void ReadVector2(vector<T>& v)
	{
		word count;
		Read(count);
		v.resize(count);
		if(count)
			Read(&v[0], sizeof(T)*count);
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
	FileWriter() : file(INVALID_FILE_HANDLE), own_handle(true)
	{
	}

	explicit FileWriter(FileHandle file) : file(file), own_handle(false)
	{
	}

	explicit FileWriter(cstring filename) : own_handle(true)
	{
		Open(filename);
	}

	~FileWriter();

	bool Open(cstring filename);
	void Write(const void* ptr, uint size);
	void Flush();
	uint GetSize() const;

	bool IsOpen() const
	{
		return file != INVALID_FILE_HANDLE;
	}

	operator bool() const
	{
		return file != INVALID_FILE_HANDLE;
	}

	template<typename T>
	void operator << (const T& a)
	{
		Write(&a, sizeof(a));
	}

	template<typename T>
	void Write(const T& a)
	{
		Write(&a, sizeof(a));
	}

	template<typename T, typename T2>
	void WriteCasted(const T2& a)
	{
		Write(&a, sizeof(T));
	}

	void WriteString1(const string& s)
	{
		uint length = (uint)s.length();
		assert(length < 256);
		WriteCasted<byte>(length);
		if(length)
			Write(s.c_str(), length);
	}

	void WriteString1(cstring str)
	{
		assert(str);
		uint length = (uint)strlen(str);
		assert(length < 256);
		WriteCasted<byte>(length);
		if(length)
			Write(str, length);
	}

	void WriteString2(const string& s)
	{
		uint length = (uint)s.length();
		assert(length < 256 * 256);
		WriteCasted<word>(length);
		if(length)
			Write(s.c_str(), length);
	}

	void WriteString2(cstring str)
	{
		assert(str);
		uint length = (uint)strlen(str);
		assert(length < 256 * 256);
		Write<word>(length);
		if(length)
			Write(str, length);
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

	template<typename T>
	void WriteVector1(const vector<T>& v)
	{
		WriteCasted<byte>(v.size());
		if(!v.empty())
			Write(&v[0], sizeof(T)*v.size());
	}

	template<typename T>
	void WriteVector2(const vector<T>& v)
	{
		WriteCasted<word>(v.size());
		if(!v.empty())
			Write(&v[0], sizeof(T)*v.size());
	}

private:
	FileHandle file;
	bool own_handle;
};

//-----------------------------------------------------------------------------
namespace io
{
	bool FileExists(Cstring path);
}
