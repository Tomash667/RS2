#pragma once

//-----------------------------------------------------------------------------
#define _CRT_SECURE_NO_WARNINGS
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#define STRICT
#ifndef _DEBUG
#	define NDEBUG
#	define _SECURE_SCL 0
#	define _HAS_ITERATOR_DEBUGGING 0
#	define DEBUG_BOOL false
#else
#	include <vld.h>
#	define DEBUG_BOOL true
#endif

//-----------------------------------------------------------------------------
#include <vector>
#include <set>
#include <unordered_map>
#include <map>
#include <string>
#include <algorithm>
#include <memory>
#include <random>

//-----------------------------------------------------------------------------
#define BIT(bit) (1<<(bit))
#define IS_SET(flaga,bit) (((flaga) & (bit)) != 0)
#define IS_CLEAR(flaga,bit) (((flaga) & (bit)) == 0)
#define IS_ALL_SET(flaga,bity) (((flaga) & (bity)) == (bity))
#define SET_BIT(flaga,bit) ((flaga) |= (bit))
#define CLEAR_BIT(flaga,bit) ((flaga) &= ~(bit))
#define SET_BIT_VALUE(flaga,bit,wartos) { if(wartos) SET_BIT(flaga,bit); else CLEAR_BIT(flaga,bit); }
#define COPY_BIT(flaga,flaga2,bit) { if(((flaga2) & (bit)) != 0) SET_BIT(flaga,bit); else CLEAR_BIT(flaga,bit); }
#define FLT10(x) (float(int((x)*10))/10)
#define FLT100(x) (float(int((x)*100))/100)
#ifndef STRING
#	define _STRING(str) #str
#	define STRING(str) _STRING(str)
#endif
#define _JOIN(a,b) a##b
#define JOIN(a,b) _JOIN(a,b)

//-----------------------------------------------------------------------------
using std::min;
using std::max;
using std::vector;
using std::string;
using std::unordered_map;
using std::unique_ptr;

//-----------------------------------------------------------------------------
// typedefs
typedef unsigned char byte;
typedef unsigned short word;
typedef unsigned int uint;
typedef __int64 int64;
typedef unsigned __int64 uint64;
typedef const char* cstring;
#ifdef X64
typedef __int64 IntPointer;
typedef unsigned __int64 UIntPointer;
#else
typedef int IntPointer;
typedef unsigned int UIntPointer;
#endif

//-----------------------------------------------------------------------------
template <typename destT, typename srcT>
destT &absolute_cast(srcT &v)
{
	return reinterpret_cast<destT&>(v);
}
template <typename destT, typename srcT>
const destT &absolute_cast(const srcT &v)
{
	return reinterpret_cast<const destT&>(v);
}

//-----------------------------------------------------------------------------
#include <DirectXMath.h>
#include "FastFunc.h"
#include "CoreMemory.h"
#include "CoreMath.h"
#include "Containers.h"
#include "Text.h"
#include "File.h"
#include "Timer.h"
#include "Logger.h"
#include "Color.h"

//-----------------------------------------------------------------------------
struct QuadTree;
class Config;
namespace tokenizer
{
	class Tokenizer;
};
using tokenizer::Tokenizer;
template<typename T>
using delegate = ssvu::FastFunc<T>;
