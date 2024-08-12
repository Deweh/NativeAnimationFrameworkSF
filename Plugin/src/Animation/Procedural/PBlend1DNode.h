#pragma once
#include "PNode.h"

namespace Animation::Procedural
{
	class PBlend1DNode : public PNodeT<PBlend1DNode>
	{
	public:
		virtual PEvaluationResult Evaluate(PNodeInstanceData* a_instanceData, PoseCache& a_poseCache, std::unordered_map<PNode*, PEvaluationResult>& a_results) override;

		inline static Registration _reg{
			"blend_1d",
			{
				{ "1", PEvaluationType<PoseCache::Handle> },
				{ "2", PEvaluationType<PoseCache::Handle> },
				{ "val", PEvaluationType<float> }
			},
			{},
			PEvaluationType<PoseCache::Handle>,
			CreateNodeOfType<PBlend1DNode>
		};
	};
}