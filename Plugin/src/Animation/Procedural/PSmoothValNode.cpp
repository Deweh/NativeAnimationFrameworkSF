#include "PSmoothValNode.h"

namespace Animation::Procedural
{
	std::unique_ptr<PNodeInstanceData> PSmoothValNode::CreateInstanceData()
	{
		return std::make_unique<InstanceData>();
	}

	PEvaluationResult PSmoothValNode::Evaluate(PNodeInstanceData* a_instanceData, PoseCache& a_poseCache, PEvaluationContext& a_evalContext)
	{
		auto inst = static_cast<InstanceData*>(a_instanceData);
		float value = std::get<float>(a_evalContext.results[inputs[0]]);

		if (!inst->initialized) {
			inst->initialized = true;
			inst->previousValue = value;
			return value;
		}

		float result = std::lerp(inst->previousValue, value, percentPerSec * inst->timeStep);
		inst->previousValue = result;
		return result;
	}

	void PSmoothValNode::AdvanceTime(PNodeInstanceData* a_instanceData, float a_deltaTime)
	{
		static_cast<InstanceData*>(a_instanceData)->timeStep = a_deltaTime;
	}

	bool PSmoothValNode::SetCustomValues(const std::span<PEvaluationResult>& a_values, const std::string_view a_skeleton)
	{
		percentPerSec = std::clamp(std::get<float>(a_values[0]), 0.0f, 1.0f);
		return true;
	}
}