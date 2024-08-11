#include "General.h"

namespace Util
{
	int GetRandomInt(int a_min, int a_max)
	{
		static std::mt19937 rand_engine;
		static std::mutex rand_lock;
		std::unique_lock l{ rand_lock };
		std::uniform_int_distribution<int> dist(a_min, a_max);
		return dist(rand_engine);
	}
}