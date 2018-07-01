#pragma once

template<typename T>
inline T& Add1(vector<T>& v)
{
	v.resize(v.size() + 1);
	return v.back();
}

template<typename Container, typename Element>
void RemoveElement(Container& c, Element e)
{
	for(auto it = c.begin(), end = c.end(); it != end; ++it)
	{
		if(*it == e)
		{
			std::iter_swap(it, end - 1);
			c.pop_back();
			return;
		}
	}
	assert(0);
}

template<typename Container, typename Element>
void DeleteElement(Container& c, Element e)
{
	for(auto it = c.begin(), end = c.end(); it != end; ++it)
	{
		if(*it == e)
		{
			delete e;
			std::iter_swap(it, end - 1);
			c.pop_back();
			return;
		}
	}
	assert(0);
}

template<typename T>
void DeleteElements(T& c)
{
	for(auto& e : c)
		delete e;
	c.clear();
}

template<typename Key, typename Value>
void DeleteElements(unordered_map<Key, Value>& c)
{
	for(auto& e : c)
		delete e.second;
	c.clear();
}

template <typename T, std::size_t N>
constexpr std::size_t countof(T const (&)[N]) noexcept
{
	return N;
}

//-----------------------------------------------------------------------------
template<typename T>
void RandomShuffle(vector<T>& v)
{
	std::random_shuffle(v.begin(), v.end(), MyRand);
}

//-----------------------------------------------------------------------------
// Get N random elements from vector, erase other
template<typename T>
void TakeRandomElements(vector<T>& v, uint count)
{
	if(v.size() <= count)
		RandomShuffle(v);
	else
	{
		uint total = v.size();
		for(uint i = 0; i < count; ++i)
		{
			uint index = Rand() % (total - i) + i;
			std::iter_swap(v.begin() + i, v.begin() + index);
		}
		v.erase(v.begin() + count, v.end());
	}
}

//-----------------------------------------------------------------------------
// Return true if any element matches condition
template<typename T, typename Arg>
inline bool Any(const T& item, const Arg& arg)
{
	return item == arg;
}

template<typename T, typename Arg, typename... Args>
inline bool Any(const T& item, const Arg& arg, const Args&... args)
{
	return item == arg || Any(item, args...);
}

//-----------------------------------------------------------------------------
// Iterate vector and remove elements when returned true
template<typename T, typename Action>
inline void LoopRemove(vector<T>& items, Action action)
{
	items.erase(std::remove_if(items.begin(), items.end(), action), items.end());
}

//-----------------------------------------------------------------------------
// Checks is vector elements are unique, create map on each use - use for debugging/tests only!
template<typename T>
bool IsUnique(vector<T>& v)
{
	std::unordered_map<T, uint> c;
	for(T e : v)
	{
		auto it = c.find(e);
		if(it == c.end())
			c[e] = 1;
		else
			return false;
	}
	return true;
}
