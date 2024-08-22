#include "PTransformRangeNode.h"

namespace Animation::Procedural
{
	PEvaluationResult PTransformRangeNode::Evaluate(PNodeInstanceData* a_instanceData, PoseCache& a_poseCache, PEvaluationContext& a_evalContext)
	{
		float value = std::get<float>(a_evalContext.results[inputs[0]]);
		float result = (value - oldMin) * scale + newMin;
		return std::clamp(result, newMin, newMax);
	}

	bool PTransformRangeNode::SetCustomValues(const std::span<PEvaluationResult>& a_values, const std::string_view a_skeleton)
	{
		oldMin = std::get<float>(a_values[0]);
		float oldMax = std::get<float>(a_values[1]);
		newMin = std::get<float>(a_values[2]);
		newMax = std::get<float>(a_values[3]);
		scale = (newMax - newMin) / (oldMax - oldMin);
		return true;
	}
}