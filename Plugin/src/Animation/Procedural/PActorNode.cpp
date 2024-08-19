#include "PActorNode.h"

namespace Animation::Procedural
{
	PEvaluationResult PActorNode::Evaluate(PNodeInstanceData* a_instanceData, PoseCache& a_poseCache, PEvaluationContext& a_evalContext)
	{
		return inputs[0];
	}
}