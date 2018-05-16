#include "Core.h"
#include <Windows.h>

//=================================================================================================
Timer::Timer(bool start) : started(false)
{
	if(start)
		Start();
}

//=================================================================================================
void Timer::Start()
{
	LARGE_INTEGER qwTicksPerSec, qwTime;
	QueryPerformanceFrequency(&qwTicksPerSec);
	QueryPerformanceCounter(&qwTime);
	ticks_per_sec = (double)qwTicksPerSec.QuadPart;
	last_time = qwTime.QuadPart;
	started = true;
}

//=================================================================================================
float Timer::Tick()
{
	assert(started);

	float delta;
	LARGE_INTEGER qwTime;

	QueryPerformanceCounter(&qwTime);
	delta = (float)((double)(qwTime.QuadPart - last_time) / ticks_per_sec);
	last_time = qwTime.QuadPart;

	if(delta < 0)
		delta = 0;

	return delta;
}

//=================================================================================================
void Timer::Reset()
{
	LARGE_INTEGER qwTime;
	QueryPerformanceCounter(&qwTime);
	last_time = qwTime.QuadPart;
}
