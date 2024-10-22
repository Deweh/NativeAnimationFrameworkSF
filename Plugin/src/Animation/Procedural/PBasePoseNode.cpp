#include "PBasePoseNode.h"

namespace Animation::Procedural
{
	PEvaluationResult PBasePoseNode::Evaluate(PNodeInstanceData* a_instanceData, PoseCache& a_poseCache, PEvaluationContext& a_evalContext)
	{
		auto output = a_poseCache.acquire_handle();
		auto outputSpan = output.get();
		auto restPose = a_evalContext.restPose->get();
		std::copy(restPose.begin(), restPose.end(), outputSpan.begin());
		return output;
	}
}