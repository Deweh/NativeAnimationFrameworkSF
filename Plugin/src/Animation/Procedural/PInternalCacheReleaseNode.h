#pragma once
#include "PNode.h"

namespace Animation::Procedural
{
	class PInternalCacheReleaseNode : public PNode
	{
	public:
		virtual PEvaluationResult Evaluate(PNodeInstanceData* a_instanceData, PoseCache& a_poseCache, std::unordered_map<PNode*, PEvaluationResult>& a_results) override;
	};
}