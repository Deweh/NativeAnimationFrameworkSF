#include "PLimitROCNode.h"

namespace Animation::Procedural
{
	std::unique_ptr<PNodeInstanceData> PLimitROCNode::CreateInstanceData()
	{
		return std::make_unique<InstanceData>();
	}

	PEvaluationResult PLimitROCNode::Evaluate(PNodeInstanceData* a_instanceData, PoseCache& a_poseCache, PEvaluationContext& a_evalContext)
	{
		auto inst = static_cast<InstanceData*>(a_instanceData);
		float value = std::get<float>(a_evalContext.results[inputs[0]]);

		if (!inst->initialized) {
			inst->initialized = true;
			inst->previousValue = value;
			return value;
		}

		float maxChange = rateOfChange * inst->timeStep;
		float trueChange = value - inst->previousValue;
		float limitedChange = std::clamp(trueChange, -maxChange, maxChange);
		float result = inst->previousValue + limitedChange;
		inst->previousValue = result;
		return result;
	}

	void PLimitROCNode::AdvanceTime(PNodeInstanceData* a_instanceData, float a_deltaTime)
	{
		static_cast<InstanceData*>(a_instanceData)->timeStep = a_deltaTime;
	}

	bool PLimitROCNode::SetCustomValues(const std::span<PEvaluationResult>& a_values, const std::string_view a_skeleton)
	{
		rateOfChange = std::max(std::get<float>(a_values[0]), 0.0f);
		return true;
	}
}