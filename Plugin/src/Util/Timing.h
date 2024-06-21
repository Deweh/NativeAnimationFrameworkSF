#pragma once

namespace Util::Timing
{
	std::chrono::high_resolution_clock::time_point HighResTimeNow();
	float HighResTimeDiffMilliSec(const std::chrono::high_resolution_clock::time_point& a_startPoint);
}