#include "MorphAnimation.h"

namespace Animation
{
	constexpr float MorphToU8{ 255.0f };
	constexpr float U8ToMorph{ 1.0f / 255.0f };

	QuantizedMorphValue::QuantizedMorphValue(float a_val)
	{
		set(a_val);
	}

	void QuantizedMorphValue::operator=(float a_rhs)
	{
		set(a_rhs);
	}

	QuantizedMorphValue::operator float() const
	{
		return get();
	}

	float QuantizedMorphValue::get() const
	{
		return static_cast<float>(_impl) * U8ToMorph;
	}

	void QuantizedMorphValue::set(float a_val)
	{
		_impl = static_cast<uint8_t>(a_val * MorphToU8);
	}

	float MorphContext::Step(const MorphAnimation* a_anim, float a_ratio)
	{
	}

	void MorphContext::Invalidate()
	{
	}

	void MorphAnimation::Sample(float a_time, MorphContext& a_context, const std::span<ozz::math::SimdFloat4>& a_output)
	{
	}
}