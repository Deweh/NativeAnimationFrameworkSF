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

		virtual void Generate(float deltaTime) = 0;
		virtual ~Generator() {}
	};

	template <typename PI = Point3LinearInterpolator, typename RI = QuaternionLinearInterpolator>
	class ClipGenerator : public Generator
	{
	public:
		using PositionTimelineType = Timeline<float, RE::NiPoint3, PI>;
		using RotationTimelineType = Timeline<float, RE::NiQuaternion, RI>;

		std::shared_ptr<OzzAnimation> anim = nullptr;
		Transform previousRoot;

		void InitTimelines()
		{
			/*
			for (auto& tl : rotation) {
				tl.Init();
			}
			for (auto& tl : position) {
				tl.Init();
			}
			*/
		}

		void SetSize(size_t)
		{
			//output.resize((s + 3) / 4);
		}

		virtual void Generate(float deltaTime) override
		{
			if (!paused) {
				localTime += deltaTime;
				if (localTime > duration || localTime < 0.0f) {
					localTime = std::fmodf(std::abs(localTime), duration);
					previousRoot.MakeIdentity();
					rootResetRequired = true;
				}
			}

			ozz::animation::SamplingJob sampleJob;
			sampleJob.animation = anim->data.get();
			sampleJob.context = context;
			sampleJob.output = ozz::make_span(output);
			sampleJob.ratio = localTime / duration;
			sampleJob.Run();
		}

		virtual ~ClipGenerator() {}
	};

	typedef ClipGenerator<Point3LinearInterpolator, QuaternionLinearInterpolator> LinearClipGenerator;
}