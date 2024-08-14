#include "PFixedValueNode.h"

namespace Animation::Procedural
{
	PEvaluationResult PFixedValueNode::Evaluate(PNodeInstanceData* a_instanceData, PoseCache& a_poseCache, std::unordered_map<PNode*, PEvaluationResult>& a_results)
	{
		return value;
	}

	bool PFixedValueNode::SetCustomValues(const std::span<PEvaluationResult>& a_values, const std::string_view a_skeleton)
	{
		value = std::get<float>(a_values[0]);
		return true;
	}
}