#pragma once
#include "PNode.h"

namespace Animation::Procedural
{
	class PActorNode : public PNodeT<PActorNode>
	{
	public:
		virtual PEvaluationResult Evaluate(PNodeInstanceData* a_instanceData, PoseCache& a_poseCache, PEvaluationContext& a_evalContext) override;

		inline static Registration _reg{
			"actor",
			{
				{ "input", PEvaluationType<PoseCache::Handle> }
			},
			{},
			PEvaluationType<uint64_t>,
			CreateNodeOfType<PActorNode>
		};
	};
}