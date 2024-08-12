#include "PActorNode.h"

namespace Animation::Procedural
{
	PEvaluationResult PActorNode::Evaluate(PNodeInstanceData* a_instanceData, PoseCache& a_poseCache, std::unordered_map<PNode*, PEvaluationResult>& a_results)
	{
		return inputs[0];
	}
}