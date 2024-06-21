#include "Timing.h"

namespace Util::Timing
{
	std::chrono::high_resolution_clock::time_point HighResTimeNow()
	{
		return std::chrono::high_resolution_clock::now();
	}

	float HighResTimeDiffMilliSec(const std::chrono::high_resolution_clock::time_point& a_startPoint)
	{
		float microSecs = static_cast<float>(std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - a_startPoint).count());
		return (microSecs / 1000.0f);
	}
}