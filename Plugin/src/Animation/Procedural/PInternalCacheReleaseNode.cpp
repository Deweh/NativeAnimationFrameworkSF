#include "PInternalCacheReleaseNode.h"

namespace Animation::Procedural
{
	PEvaluationResult PInternalCacheReleaseNode::Evaluate(PNodeInstanceData* a_instanceData, PoseCache& a_poseCache, std::unordered_map<PNode*, PEvaluationResult>& a_results)
	{
		std::get<PoseCache::Handle>(a_results[inputs[0]]).reset();
		return 0.0f;
	}
}