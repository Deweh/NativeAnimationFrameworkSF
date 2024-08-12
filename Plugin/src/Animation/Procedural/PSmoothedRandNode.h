#pragma once
#include "PNode.h"

namespace Animation::Procedural
{
	class PSmoothedRandNode : public PNode
	{
	public:
		enum class RandState : uint8_t
		{
			kTransitioning,
			kDelaying
		};

		struct InstanceData : public PNodeInstanceData
		{
			RandState state = RandState::kDelaying;
			float startValue{ 0.5f };
			float targetValue{ 0.5f };
			float duration{ 0.1f };
			float localTime{ 0.0f };
		};

		float durMin;
		float durMax;
		float diffMin;
		float diffMax;
		float delayMin;
		float delayMax;

		virtual std::unique_ptr<PNodeInstanceData> CreateInstanceData(const OzzSkeleton* a_skeleton) override;
		virtual PEvaluationResult Evaluate(PNodeInstanceData* a_instanceData, PoseCache& a_poseCache, std::unordered_map<PNode*, PEvaluationResult>& a_results) override;
		virtual void AdvanceTime(PNodeInstanceData* a_instanceData, float a_deltaTime) override;
		void UpdateTargetValue(InstanceData* a_instanceData);

	private:
		inline static Registration _reg{
			"smooth_rand",
			{},
			{
				{ "dur_min", PEvaluationType<float> },
				{ "dur_max", PEvaluationType<float> },
				{ "diff_min", PEvaluationType<float> },
				{ "diff_max", PEvaluationType<float> },
				{ "delay_min", PEvaluationType<float> },
				{ "delay_max", PEvaluationType<float> }
			},
			PEvaluationType<float>,
			CreateNodeOfType<PSmoothedRandNode>
		};
	};
}