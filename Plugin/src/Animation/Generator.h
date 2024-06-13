#pragma once
#include "Transform.h"
#include "Timeline.h"
#include "Interpolation.h"
#include "Ozz.h"

namespace Animation
{
	class Generator
	{
	public:
		bool rootResetRequired = false;
		bool paused = false;
		float localTime = 0.0f;
		float duration = 0.0f;
		Transform localRootTransform;
		std::span<ozz::math::SoaTransform> output;
		ozz::animation::SamplingJob::Context* context = nullptr;

		virtual void Generate(float deltaTime);
		virtual void SetOutput(const ozz::span<ozz::math::SoaTransform>& span);
		virtual void SetContext(ozz::animation::SamplingJob::Context* ctxt);
		virtual void OnDetaching();

		virtual ~Generator() = default;
	};

	class LinearClipGenerator : public Generator
	{
	public:
		std::shared_ptr<OzzAnimation> anim = nullptr;

		virtual void Generate(float deltaTime) override;
		virtual ~LinearClipGenerator() = default;
	};

	class AdditiveGenerator : public Generator
	{
	public:
		float additiveWeight = 1.0f;
		std::vector<ozz::math::SoaTransform> restPose;
		std::vector<ozz::math::SoaTransform> baseGenOutput;
		std::unique_ptr<Generator> baseGen = nullptr;

		void SetRestPose(const std::vector<ozz::math::SoaTransform>& pose);
		virtual void Generate(float deltaTime) override;
		virtual void SetOutput(const ozz::span<ozz::math::SoaTransform>& span) override;
		virtual void SetContext(ozz::animation::SamplingJob::Context* ctxt) override;
		virtual ~AdditiveGenerator() = default;
	};
}