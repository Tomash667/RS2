#pragma once

//-----------------------------------------------------------------------------
// Allocators
template<typename T>
struct IAllocator
{
	virtual T* Create() = 0;
	virtual void Destroy(T* item) = 0;
};

namespace internal
{
	template<typename T>
	class AllocateHelper
	{
	public:
		template<typename Q = T>
		static typename std::enable_if<std::is_abstract<Q>::value || !std::is_default_constructible<T>::value, Q>::type* Allocate()
		{
			return nullptr;
		}

		template<typename Q = T>
		static typename std::enable_if<!std::is_abstract<Q>::value && std::is_default_constructible<T>::value, Q>::type* Allocate()
		{
			return new T;
		}
	};

	template<typename T>
	struct StandardAllocator : IAllocator<T>
	{
		T* Create() override
		{
			return AllocateHelper<T>::Allocate();
		}

		void Destroy(T* item) override
		{
			delete item;
		}
	};

	template<typename T>
	struct ComAllocator : IAllocator<T>
	{
		T* Create() override
		{
			return nullptr;
		}

		void Destroy(T* item) override
		{
			if(item)
				item->Release();
		}
	};
}

//-----------------------------------------------------------------------------
// RAII for simple pointer
template<typename T, typename Allocator = internal::StandardAllocator<T>>
class Ptr
{
	static_assert(std::is_base_of<IAllocator<T>, Allocator>::value, "Allocator must inherit from IAllocator.");
public:
	Ptr(nullptr_t) : ptr(nullptr)
	{
	}
	Ptr(T* ptr) : ptr(ptr)
	{
	}
	template<typename U = T>
	Ptr(typename std::enable_if<!std::is_abstract<U>::value && std::is_default_constructible<U>::value>::type* = nullptr)
	{
		ptr = allocator.Create();
	}
	template<typename U = T>
	Ptr(typename std::enable_if<std::is_abstract<U>::value || !std::is_default_constructible<U>::value>::type* = nullptr) : ptr(nullptr)
	{
	}
	~Ptr()
	{
		if(ptr)
			allocator.Destroy(ptr);
	}
	void operator = (T* new_ptr)
	{
		if(ptr)
			allocator.Destroy(ptr);
		ptr = new_ptr;
	}
	operator T* ()
	{
		return ptr;
	}
	operator T** ()
	{
		return &ptr;
	}
	T* operator -> ()
	{
		return ptr;
	}
	void Ensure()
	{
		if(!ptr)
			ptr = allocator.Create();
	}
	T* Pin()
	{
		T* t = ptr;
		ptr = nullptr;
		return t;
	}
	T*& Get()
	{
		return ptr;
	}

private:
	T* ptr;
	Allocator allocator;
};

template<typename T>
using CPtr = Ptr<T, internal::ComAllocator<T>>;

//-----------------------------------------------------------------------------
// Object pool
//-----------------------------------------------------------------------------
#ifdef _DEBUG
// helper to check for object pool leaks
struct ObjectPoolLeakManager
{
	struct CallStackEntry;
	~ObjectPoolLeakManager();
	void Register(void* ptr);
	void Unregister(void* ptr);
	static ObjectPoolLeakManager instance;
private:
	vector<CallStackEntry*> call_stack_pool;
	unordered_map<void*, CallStackEntry*> call_stacks;
};
#endif

// object pool
template<typename T>
struct ObjectPool
{
	ObjectPool() : destroyed(false)
	{
	}

	~ObjectPool()
	{
		Cleanup();
		destroyed = true;
	}

	T* Get()
	{
		T* t;
		if(pool.empty())
			t = new T;
		else
		{
			t = pool.back();
			pool.pop_back();
		}
#ifdef _DEBUG
		ObjectPoolLeakManager::instance.Register(t);
#endif
		return t;
	}

	void VerifyElement(T* t)
	{
		for(T* e : pool)
			assert(t != e);
	}

	void CheckDuplicates(vector<T*>& elems)
	{
		for(uint i = 0, count = elems.size(); i < count; ++i)
		{
			T* t = elems[i];
			if(!t)
				continue;
			for(uint j = i + 1; j < count; ++j)
				assert(t != elems[j]);
		}
	}

	void Free(T* e)
	{
		assert(e);
#ifdef _DEBUG
		VerifyElement(e);
		ObjectPoolLeakManager::instance.Unregister(e);
#endif
		pool.push_back(e);
	}

	void Free(vector<T*>& elems)
	{
		if(elems.empty())
			return;

#ifdef _DEBUG
		CheckDuplicates(elems);
		for(T* e : elems)
		{
			assert(e);
			VerifyElement(e);
			ObjectPoolLeakManager::instance.Unregister(e);
		}
#endif

		pool.insert(pool.end(), elems.begin(), elems.end());
		elems.clear();
	}

	void SafeFree(T* e)
	{
		if(!destroyed)
			Free(e);
		else
		{
			assert(e);
#ifdef _DEBUG
			ObjectPoolLeakManager::instance.Unregister(e);
#endif
			delete e;
		}
	}

	void SafeFree(vector<T*>& elems)
	{
#ifdef _DEBUG
		CheckDuplicates(elems);
#endif
		if(!destroyed)
		{
			for(T* e : elems)
			{
				if(e)
				{
#ifdef _DEBUG
					VerifyElement(e);
					ObjectPoolLeakManager::instance.Unregister(e);
#endif
					pool.push_back(e);
				}
			}
		}
		else
		{
			for(T* e : elems)
			{
				if(e)
				{
#ifdef _DEBUG
					ObjectPoolLeakManager::instance.Unregister(e);
#endif
					delete e;
				}
			}
		}
		elems.clear();
	}

	void Cleanup()
	{
		DeleteElements(pool);
	}

private:
	vector<T*> pool;
	bool destroyed;
};

template<typename T>
class ObjectPoolProxy
{
public:
	static T* Get() { return GetPool().Get(); }
	static void Free(T* t) { GetPool().Free(t); }
	static void Free(vector<T*>& ts) { GetPool().Free(ts); }
	static void SafeFree(vector <T*>& ts) { GetPool().SafeFree(ts); }
	static void Cleanup() { GetPool().Cleanup(); }
	virtual void Free() { Free((T*)this); }

private:
	static ObjectPool<T>& GetPool() { static ObjectPool<T> pool; return pool; }
};

namespace internal
{
	template<typename T>
	struct ObjectPoolAllocator : IAllocator<T>
	{
		static_assert(std::is_base_of<ObjectPoolProxy<T>, T>::value, "T must inherit from ObjectPoolProxy<T>");

		T* Create()
		{
			return T::Get();
		}

		void Destroy(T* item)
		{
			T::Free(item);
		}
	};
}

//-----------------------------------------------------------------------------
extern ObjectPool<string> StringPool;

//-----------------------------------------------------------------------------
// String using StringPool
struct LocalString
{
	LocalString()
	{
		s = StringPool.Get();
		s->clear();
	}

	LocalString(cstring str)
	{
		assert(str);
		s = StringPool.Get();
		*s = str;
	}

	LocalString(cstring str, cstring str_to)
	{
		s = StringPool.Get();
		uint len = str_to - str;
		s->resize(len);
		memcpy((char*)s->data(), str, len);
	}

	LocalString(const string& str)
	{
		s = StringPool.Get();
		*s = str;
	}

	~LocalString()
	{
		if(s)
			StringPool.Free(s);
	}

	void operator = (cstring str)
	{
		*s = str;
	}

	void operator = (const string& str)
	{
		*s = str;
	}

	char at_back(uint offset) const
	{
		assert(offset < s->size());
		return s->at(s->size() - 1 - offset);
	}

	void pop(uint count)
	{
		assert(s->size() > count);
		s->resize(s->size() - count);
	}

	void operator += (cstring str)
	{
		*s += str;
	}

	void operator += (const string& str)
	{
		*s += str;
	}

	void operator += (char c)
	{
		*s += c;
	}

	operator cstring() const
	{
		return s->c_str();
	}

	string& operator * ()
	{
		return *s;
	}

	string& get_ref()
	{
		return *s;
	}

	string* get_ptr()
	{
		return s;
	}

	string* operator -> ()
	{
		return s;
	}

	const string* operator -> () const
	{
		return s;
	}

	bool operator == (cstring str) const
	{
		return *s == str;
	}

	bool operator == (const string& str) const
	{
		return *s == str;
	}

	bool operator == (const LocalString& str) const
	{
		return *s == *str.s;
	}

	bool operator != (cstring str) const
	{
		return *s != str;
	}

	bool operator != (const string& str) const
	{
		return *s != str;
	}

	bool operator != (const LocalString& str) const
	{
		return *s != *str.s;
	}

	bool empty() const
	{
		return s->empty();
	}

	cstring c_str() const
	{
		return s->c_str();
	}

	void clear()
	{
		s->clear();
	}

	uint length() const
	{
		return s->length();
	}

	string* Pin()
	{
		string* str = s;
		s = nullptr;
		return str;
	}

private:
	string * s;
};
