#include "PInternalCacheReleaseNode.h"

namespace Animation::Procedural
{
	PEvaluationResult PInternalCacheReleaseNode::Evaluate(PNodeInstanceData* a_instanceData, PoseCache& a_poseCache, PEvaluationContext& a_evalContext)
	{
		std::get<PoseCache::Handle>(a_evalContext.results[inputs[0]]).reset();
		return 0.0f;
	}
}