#pragma once

struct Sky
{
	Sky() : time(0.5f) {}

	void SetTime(float time)
	{
		assert(InRange(time, 0.f, 1.f));
		this->time = time;
	}

	float GetTime() const { return time; }

private:
	float time;
};
