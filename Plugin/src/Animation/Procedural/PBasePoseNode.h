#pragma once
#include "PNode.h"

namespace Animation::Procedural
{
	class PBasePoseNode : public PNodeT<PBasePoseNode>
	{
	public:
		virtual PEvaluationResult Evaluate(PNodeInstanceData* a_instanceData, PoseCache& a_poseCache, PEvaluationContext& a_evalContext) override;

		inline static Registration _reg{
			"base_pose",
			{},
			{},
			PEvaluationType<PoseCache::Handle>,
			CreateNodeOfType<PBasePoseNode>
		};
	};
}