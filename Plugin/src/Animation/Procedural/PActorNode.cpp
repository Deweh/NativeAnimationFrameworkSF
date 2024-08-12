#include "PActorNode.h"

namespace Animation::Procedural
{
	std::unique_ptr<PNodeInstanceData> PActorNode::CreateInstanceData(const OzzSkeleton* a_skeleton)
	{
		return nullptr;
	}

	PEvaluationResult PActorNode::Evaluate(PNodeInstanceData* a_instanceData, PoseCache& a_poseCache, std::unordered_map<PNode*, PEvaluationResult>& a_results)
	{
		return inputs[0];
	}

	void PActorNode::AdvanceTime(PNodeInstanceData* a_instanceData, float a_deltaTime)
	{
	}
}