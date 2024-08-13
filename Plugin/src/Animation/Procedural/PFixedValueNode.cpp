#include "PFixedValueNode.h"

namespace Animation::Procedural
{
	PEvaluationResult PFixedValueNode::Evaluate(PNodeInstanceData* a_instanceData, PoseCache& a_poseCache, std::unordered_map<PNode*, PEvaluationResult>& a_results)
	{
		return value;
	}

	void PFixedValueNode::SetCustomValues(const std::span<PEvaluationResult>& a_values)
	{
		value = std::get<float>(a_values[0]);
	}
}