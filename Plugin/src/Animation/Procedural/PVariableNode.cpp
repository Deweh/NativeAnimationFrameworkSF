#include "PVariableNode.h"
#include "Util/String.h"

namespace Animation::Procedural
{
	std::unique_ptr<PNodeInstanceData> PVariableNode::CreateInstanceData()
	{
		auto result = std::make_unique<InstanceData>();
		result->value = defaultValue;
		return result;
	}

	PEvaluationResult PVariableNode::Evaluate(PNodeInstanceData* a_instanceData, PoseCache& a_poseCache, PEvaluationContext& a_evalContext)
	{
		return static_cast<InstanceData*>(a_instanceData)->value;
	}

	void PVariableNode::Synchronize(PNodeInstanceData* a_instanceData, PNodeInstanceData* a_ownerInstance, float a_correctionDelta)
	{
		static_cast<InstanceData*>(a_instanceData)->value = static_cast<InstanceData*>(a_ownerInstance)->value;
	}

	bool PVariableNode::SetCustomValues(const std::span<PEvaluationResult>& a_values, const std::string_view a_skeleton)
	{
		name = std::get<RE::BSFixedString>(a_values[0]);
		defaultValue = std::get<float>(a_values[1]);
		Util::String::TransformToLower(name);
		syncId = std::hash<std::string>{}(name);
		return true;
	}
}