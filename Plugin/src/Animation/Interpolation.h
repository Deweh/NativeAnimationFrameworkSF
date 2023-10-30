#pragma once

namespace Animation
{
	template <typename ITP, typename E>
	struct EasedInterpolator
	{
		ITP interpolator;
		E easing;

		void operator()(const auto& prev, auto next, auto t, auto& outVal) {
			t = easing(t);
			interpolator(prev, next, t, outVal);
		}
	};

	struct QuaternionLinearInterpolator
	{
		void operator()(const RE::NiQuaternion& prev, RE::NiQuaternion next, float t, RE::NiQuaternion& outVal);
	};

	struct Point3LinearInterpolator
	{
		void operator()(const RE::NiPoint3& prev, const RE::NiPoint3& next, float t, RE::NiPoint3& outVal);
	};
}