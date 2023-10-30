#pragma once
#include "Transform.h"
#include "Timeline.h"
#include "Interpolation.h"

namespace Animation
{
	class Generator
	{
	public:
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
				while (localTime >= duration) {
					localTime -= duration;
				}
			}

			for (size_t i = 0; i < output.size(); i++) {
				auto& result = output[i];
				position[i].GetValueAtTime(localTime, result.translate);
				rotation[i].GetValueAtTime(localTime, result.rotate);
			}
		}

		virtual ~ClipGenerator() {}
	};

	typedef ClipGenerator<Point3LinearInterpolator, QuaternionLinearInterpolator> LinearClipGenerator;
}