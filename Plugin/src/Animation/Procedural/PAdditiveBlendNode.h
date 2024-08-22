#pragma once
#include "PNode.h"

namespace Animation::Procedural
{
	class PAdditiveBlendNode : public PNodeT<PAdditiveBlendNode>
	{
	public:
		virtual PEvaluationResult Evaluate(PNodeInstanceData* a_instanceData, PoseCache& a_poseCache, PEvaluationContext& a_evalContext) override;

		inline static Registration _reg{
			"blend_add",
			{
				{ "add", PEvaluationType<PoseCache::Handle> },
				{ "full", PEvaluationType<PoseCache::Handle> },
				{ "val", PEvaluationType<float> }
			},
			{},
			PEvaluationType<PoseCache::Handle>,
			CreateNodeOfType<PAdditiveBlendNode>
		};
	};
}