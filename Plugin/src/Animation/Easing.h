#pragma once

namespace Animation
{
	template <typename T>
	struct CubicInOutEase
	{
		T operator()(T x) {
			auto _2 = static_cast<T>(2);
			return x < static_cast<T>(0.5) ? static_cast<T>(4) * x * x * x : static_cast<T>(1) - std::pow(static_cast<T>(-2) * x + _2, static_cast<T>(3)) / _2;
		}
	};
}