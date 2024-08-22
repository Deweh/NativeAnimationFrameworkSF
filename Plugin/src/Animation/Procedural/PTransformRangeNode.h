#pragma once
#include "PNode.h"

namespace Animation::Procedural
{
	class PTransformRangeNode : public PNodeT<PTransformRangeNode>
	{
	public:
		float scale;
		float oldMin;
		float newMin;
		float newMax;

		virtual PEvaluationResult Evaluate(PNodeInstanceData* a_instanceData, PoseCache& a_poseCache, PEvaluationContext& a_evalContext) override;
		virtual bool SetCustomValues(const std::span<PEvaluationResult>& a_values, const std::string_view a_skeleton) override;

		inline static Registration _reg{
			"transform_range",
			{
				{ "input", PEvaluationType<float> }
			},
			{
				{ "oldMin", PEvaluationType<float> },
				{ "oldMax", PEvaluationType<float> },
				{ "newMin", PEvaluationType<float> },
				{ "newMax", PEvaluationType<float> }
			},
			PEvaluationType<float>,
			CreateNodeOfType<PTransformRangeNode>
		};
	};
}