#pragma once
#include "Animation/PoseCache.h"
#include "Animation/Ozz.h"

namespace Animation::Procedural
{
	using PEvaluationResult = std::variant<float, PoseCache::Handle>;

	struct PNodeInstanceData
	{
		virtual ~PNodeInstanceData() = default;
	};

	class PNode
	{
	public:
		std::vector<PNode*> inputs;

		virtual std::unique_ptr<PNodeInstanceData> CreateInstanceData(const OzzSkeleton* a_skeleton) = 0;
		virtual PEvaluationResult Evaluate(PNodeInstanceData* a_instanceData, PoseCache& a_poseCache, std::unordered_map<PNode*, PEvaluationResult>& a_results) = 0;
		virtual void AdvanceTime(PNodeInstanceData* a_instanceData, float a_deltaTime) = 0;
	};
}