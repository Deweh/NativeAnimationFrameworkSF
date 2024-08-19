#include "General.h"

namespace Util
{
	static std::mt19937 rand_engine;
	static std::mutex rand_lock;

	int GetRandomInt(int a_min, int a_max)
	{
		std::unique_lock l{ rand_lock };
		std::uniform_int_distribution<int> dist(a_min, a_max);
		return dist(rand_engine);
	}

	float GetRandomFloat(float a_min, float a_max)
	{
		std::unique_lock l{ rand_lock };
		std::uniform_real_distribution<float> dist(a_min, a_max);
		return dist(rand_engine);
	}
}