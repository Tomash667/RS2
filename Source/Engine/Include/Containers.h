#pragma once

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
