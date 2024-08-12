#pragma once
#include "PNode.h"

namespace Animation::Procedural
{
	class PBlend1DNode : public PNode
	{
	public:
		struct InstanceData : public PNodeInstanceData
		{
		};

		virtual std::unique_ptr<PNodeInstanceData> CreateInstanceData(const OzzSkeleton* a_skeleton) override;
		virtual PEvaluationResult Evaluate(PNodeInstanceData* a_instanceData, PoseCache& a_poseCache, std::unordered_map<PNode*, PEvaluationResult>& a_results) override;
		virtual void AdvanceTime(PNodeInstanceData* a_instanceData, float a_deltaTime) override;

	private:
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