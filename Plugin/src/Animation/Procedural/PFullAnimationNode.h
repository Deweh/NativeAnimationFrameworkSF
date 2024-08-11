#pragma once
#include "PNode.h"
#include "Animation/Ozz.h"
#include "Animation/FileID.h"

namespace Animation::Procedural
{
	class PFullAnimationNode : public PNode
	{
	public:
		struct InstanceData : public PNodeInstanceData
		{
			bool looped{ false };
			bool paused{ false };
			float localTime{ 0.0f };
			std::shared_ptr<OzzAnimation> anim;
			ozz::animation::SamplingJob::Context context;
		};

		FileID file;

		virtual std::unique_ptr<PNodeInstanceData> CreateInstanceData(const OzzSkeleton* a_skeleton) override;
		virtual PEvaluationResult Evaluate(PNodeInstanceData* a_instanceData, PoseCache& a_poseCache, std::unordered_map<PNode*, PEvaluationResult>& a_results) override;
		virtual void AdvanceTime(PNodeInstanceData* a_instanceData, float a_deltaTime) override;
	};
}