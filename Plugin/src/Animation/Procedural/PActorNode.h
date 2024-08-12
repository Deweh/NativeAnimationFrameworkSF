#pragma once
#include "PNode.h"

namespace Animation::Procedural
{
	class PActorNode : public PNode
	{
	public:
		virtual PEvaluationResult Evaluate(PNodeInstanceData* a_instanceData, PoseCache& a_poseCache, std::unordered_map<PNode*, PEvaluationResult>& a_results) override;

	private:
		inline static Registration _reg{
			"actor",
			{
				{ "input", PEvaluationType<PoseCache::Handle> }
			},
			{},
			PEvaluationType<PNode*>,
			CreateNodeOfType<PActorNode>
		};
	};
}