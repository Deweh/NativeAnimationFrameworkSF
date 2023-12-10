#pragma once
#include "Transform.h"
#include "Timeline.h"
#include "Interpolation.h"

namespace Animation
{
	class Generator
	{
	public:
		bool rootResetRequired = false;
		bool paused = false;
		float localTime = 0.0f;
		float duration = 0.0f;
		std::vector<Transform> output;

		virtual void Generate(float deltaTime) = 0;
		virtual ~Generator() {}
	};

	template <typename PI = Point3LinearInterpolator, typename RI = QuaternionLinearInterpolator>
	class ClipGenerator : public Generator
	{
	public:
		using PositionTimelineType = Timeline<float, RE::NiPoint3, PI>;
		using RotationTimelineType = Timeline<float, RE::NiQuaternion, RI>;

		std::vector<PositionTimelineType> position;
		std::vector<RotationTimelineType> rotation;
		Transform previousRoot;

		void InitTimelines()
		{
			for (auto& tl : rotation) {
				tl.Init();
			}
			for (auto& tl : position) {
				tl.Init();
			}
		}

		void SetSize(size_t s)
		{
			output.resize(s);
			position.resize(s);
			rotation.resize(s);
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

			for (size_t i = 0; i < output.size(); i++) {
				auto& result = output[i];
				position[i].GetValueAtTime(localTime, result.translate);
				rotation[i].GetValueAtTime(localTime, result.rotate);
			}

			if (!output.empty()) {
				auto& rootTransform = output[0];
				Transform temp = rootTransform;
				rootTransform = rootTransform - previousRoot;
				previousRoot = temp;
			}
		}

		virtual ~ClipGenerator() {}
	};

	typedef ClipGenerator<Point3LinearInterpolator, QuaternionLinearInterpolator> LinearClipGenerator;
}