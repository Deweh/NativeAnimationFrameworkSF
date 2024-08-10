#pragma once

namespace Animation
{
	class QuantizedMorphValue
	{
	public:
		QuantizedMorphValue() = default;
		QuantizedMorphValue(float a_val);

		void operator=(float a_rhs);
		operator float() const;

		float get() const;
		void set(float a_val);

	private:
		uint8_t _impl;
	};

	class MorphAnimation;

	struct MorphContext
	{
		float ratio;
		std::span<ozz::math::Float2> cachedKeys;
		std::span<uint16_t> entries;
		const MorphAnimation* animation{ nullptr };

		float Step(const MorphAnimation* a_anim, float a_ratio);
		void Invalidate();
	};

	struct MorphTrack
	{
		std::span<QuantizedMorphValue> keys;
		std::span<uint16_t> timeIdxs;
	};

	class MorphAnimation
	{
		std::span<MorphTrack> tracks;
		std::span<float> timepoints;
		float duration;

		void Sample(float a_time, MorphContext& a_context, const std::span<ozz::math::SimdFloat4>& a_output);
	};
}