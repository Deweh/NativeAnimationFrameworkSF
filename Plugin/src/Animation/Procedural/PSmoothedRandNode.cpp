#include "PSmoothedRandNode.h"
#include "Util/General.h"
#include "Animation/Easing.h"

namespace Animation::Procedural
{
	std::unique_ptr<PNodeInstanceData> PSmoothedRandNode::CreateInstanceData()
	{
		return std::make_unique<InstanceData>();
	}

	PEvaluationResult PSmoothedRandNode::Evaluate(PNodeInstanceData* a_instanceData, PoseCache& a_poseCache, PEvaluationContext& a_evalContext)
	{
		auto inst = static_cast<InstanceData*>(a_instanceData);
		if (inst->state == RandState::kTransitioning) {
			Animation::CubicInOutEase<float> ease;
			return inst->localTime == 0.0f ? inst->startValue : std::lerp(inst->startValue, inst->targetValue, ease(inst->localTime / inst->duration));
		} else {
			return inst->targetValue;
		}
	}

	void PSmoothedRandNode::AdvanceTime(PNodeInstanceData* a_instanceData, float a_deltaTime)
	{
		auto inst = static_cast<InstanceData*>(a_instanceData);
		inst->localTime += a_deltaTime;
		if (inst->localTime > inst->duration || inst->localTime < 0.0f) {
			inst->localTime = 0.0f;

			if (inst->state == RandState::kTransitioning) {
				inst->duration = delayMin + Util::GetRandomFloat(0.0f, 1.0f) * (delayMax - delayMin);
				inst->localTime = 0.0f;
				inst->state = RandState::kDelaying;
				inst->startValue = inst->targetValue;
			} else {
				inst->duration = durMin + Util::GetRandomFloat(0.0f, 1.0f) * (durMax - durMin);
				inst->localTime = 0.0f;
				inst->state = RandState::kTransitioning;
				UpdateTargetValue(inst);
			}
		}
	}

	void PSmoothedRandNode::Synchronize(PNodeInstanceData* a_instanceData, PNodeInstanceData* a_ownerInstance, float a_correctionDelta)
	{
		auto inst = static_cast<InstanceData*>(a_instanceData);
		auto owner = static_cast<InstanceData*>(a_ownerInstance);

		inst->duration = owner->duration;
		inst->localTime = owner->localTime;
		inst->startValue = owner->startValue;
		inst->state = owner->state;
		inst->targetValue = owner->targetValue;

		if (a_correctionDelta > 0.0f) {
			AdvanceTime(a_instanceData, a_correctionDelta);
		}
	}

	bool PSmoothedRandNode::SetCustomValues(const std::span<PEvaluationResult>& a_values, const std::string_view a_skeleton)
	{
		durMin = std::get<float>(a_values[0]);
		durMax = std::get<float>(a_values[1]);
		diffMin = std::get<float>(a_values[2]);
		diffMax = std::get<float>(a_values[3]);
		delayMin = std::get<float>(a_values[4]);
		delayMax = std::get<float>(a_values[5]);
		edgeThreshold = std::get<float>(a_values[6]);
		syncId = std::get<uint64_t>(a_values[7]);

		durMin = std::max(0.1f, durMin);
		durMax = std::max(0.1f, durMax);

		if (durMin > durMax) {
			std::swap(durMin, durMax);
		}

		if (diffMin > diffMax) {
			std::swap(diffMin, diffMax);
		}

		if (delayMin > delayMax) {
			std::swap(delayMin, delayMax);
		}

		edgeThreshold = std::clamp(edgeThreshold, 0.01f, 0.49f);

		return true;
	}

	void PSmoothedRandNode::UpdateTargetValue(InstanceData* a_instanceData)
	{
		float difference = diffMin + Util::GetRandomFloat(0.0f, 1.0f) * (diffMax - diffMin);
		float edgeBiasProbability;
		
		float currentValue = a_instanceData->targetValue;
		if (currentValue < edgeThreshold) {
			edgeBiasProbability = 0.5f + (edgeThreshold - currentValue) / edgeThreshold * 0.5f;
		} else if (currentValue > (1.0f - edgeThreshold)) {
			edgeBiasProbability = 0.5f - (currentValue - (1.0f - edgeThreshold)) / edgeThreshold * 0.5f;
		} else {
			edgeBiasProbability = 0.5f;
		}

		bool addDifference = Util::GetRandomFloat(0.0f, 1.0f) < edgeBiasProbability;
		float newValue = addDifference ? currentValue + difference : currentValue - difference;
		a_instanceData->targetValue = std::max(0.0f, std::min(1.0f, newValue));
	}
}