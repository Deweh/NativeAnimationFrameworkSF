#pragma once
#include "PNode.h"
#include "Animation/Ozz.h"
#include "Animation/FileID.h"

namespace Animation::Procedural
{
	class PFullAnimationNode : public PNodeT<PFullAnimationNode>
	{
	public:
		struct InstanceData : public PNodeInstanceData
		{
			bool looped{ false };
			float localTime{ 0.0f };
			ozz::animation::SamplingJob::Context context;
		};

		std::shared_ptr<OzzAnimation> anim;

		virtual std::unique_ptr<PNodeInstanceData> CreateInstanceData() override;
		virtual PEvaluationResult Evaluate(PNodeInstanceData* a_instanceData, PoseCache& a_poseCache, std::unordered_map<PNode*, PEvaluationResult>& a_results) override;
		virtual void AdvanceTime(PNodeInstanceData* a_instanceData, float a_deltaTime) override;
		virtual bool SetCustomValues(const std::span<PEvaluationResult>& a_values, const std::string_view a_skeleton) override;

		inline static Registration _reg{
			"anim",
			{},
			{
				{ "file", PEvaluationType<std::string> }
			},
			PEvaluationType<PoseCache::Handle>,
			CreateNodeOfType<PFullAnimationNode>
		};
	};
}