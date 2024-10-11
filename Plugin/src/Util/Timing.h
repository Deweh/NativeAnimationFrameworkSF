#pragma once

namespace Util::Timing
{
	std::chrono::high_resolution_clock::time_point HighResTimeNow();
	float HighResTimeDiffMilliSec(const std::chrono::high_resolution_clock::time_point& a_startPoint);
}

#ifdef ENABLE_PERFORMANCE_MONITORING
#	define PERF_TIMER_START(name) std::chrono::high_resolution_clock::time_point name = Util::Timing::HighResTimeNow()
#	define PERF_TIMER_END(startName, diffName) float diffName = Util::Timing::HighResTimeDiffMilliSec(startName)
#	define PERF_TIMER_COPY_VALUE(diffName, targetName) targetName = diffName
#else
#	define PERF_TIMER_START(name)
#	define PERF_TIMER_END(startName, diffName)
#	define PERF_TIMER_COPY_VALUE(diffName, targetName)
#endif