#pragma once

#undef CreateWindow

#ifdef _DEBUG
#	define C(x) { HRESULT hr = x; assert(SUCCEEDED(hr)); }
#else
#	define C(x) x
#endif

template<typename T>
void SafeRelease(T* ptr)
{
	if(ptr)
		ptr->Release();
}
