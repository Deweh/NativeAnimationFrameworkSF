#pragma once
#include "PNode.h"

namespace Animation::Procedural
{
	class PFixedValueNode : public PNodeT<PFixedValueNode>
	{
	public:
		float value;

		virtual PEvaluationResult Evaluate(PNodeInstanceData* a_instanceData, PoseCache& a_poseCache, std::unordered_map<PNode*, PEvaluationResult>& a_results) override;
		virtual void SetCustomValues(const std::span<PEvaluationResult>& a_values) override;

		inline static Registration _reg{
			"fixed_val",
			{},
			{
				{ "val", PEvaluationType<float> }
			},
			PEvaluationType<float>,
			CreateNodeOfType<PFixedValueNode>
		};
	};
}