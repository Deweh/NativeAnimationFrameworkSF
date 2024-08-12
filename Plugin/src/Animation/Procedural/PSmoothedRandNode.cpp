#include "PSmoothedRandNode.h"
#include "Util/General.h"

namespace Animation::Procedural
{
	std::unique_ptr<PNodeInstanceData> PSmoothedRandNode::CreateInstanceData(const OzzSkeleton* a_skeleton)
	{
		return std::make_unique<InstanceData>();
	}

	PEvaluationResult PSmoothedRandNode::Evaluate(PNodeInstanceData* a_instanceData, PoseCache& a_poseCache, std::unordered_map<PNode*, PEvaluationResult>& a_results)
	{
		auto inst = static_cast<InstanceData*>(a_instanceData);
		if (inst->state == RandState::kTransitioning) {
			return std::lerp(inst->startValue, inst->targetValue, inst->localTime / inst->duration);
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
				float delayRand = static_cast<float>(Util::GetRandomInt(0, 100)) * 0.01f;

				inst->duration = ((delayMax - delayMin) * delayRand) + delayMin;
				inst->localTime = 0.0f;
				inst->state = RandState::kDelaying;
				inst->startValue = inst->targetValue;
			} else {
				float durRand = static_cast<float>(Util::GetRandomInt(0, 100)) * 0.01f;

				inst->duration = ((durMax - durMin) * durRand) + durMin;
				inst->localTime = 0.0f;
				inst->state = RandState::kTransitioning;
				UpdateTargetValue(inst);
			}
		}
	}

	void PSmoothedRandNode::UpdateTargetValue(InstanceData* a_instanceData)
	{
		float minMaxDiff = diffMax - diffMin;
		float halfDiff = minMaxDiff * 0.5f;

		float diffRand = static_cast<float>(Util::GetRandomInt(0, 100)) * 0.01f;
		float diff = (((minMaxDiff * diffRand) + diffMin) - halfDiff) + a_instanceData->startValue;
		a_instanceData->targetValue = std::clamp(diff, 0.0f, 1.0f);
	}
}