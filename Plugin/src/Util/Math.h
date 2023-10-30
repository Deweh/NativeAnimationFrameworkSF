#pragma once
#define PI_F 3.14159265359f

namespace Util
{
	inline static constexpr float DEGREE_TO_RADIAN{ PI_F / 180.0f };
	inline static constexpr float RADIAN_TO_DEGREE{ 180.0f / PI_F };

	float NormalizeSpan(float begin, float end, float val);
}