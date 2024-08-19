#pragma once
#include "PNode.h"

namespace Animation::Procedural
{
	class PSmoothedRandNode : public PNodeT<PSmoothedRandNode>
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
		float edgeThreshold;

		virtual std::unique_ptr<PNodeInstanceData> CreateInstanceData() override;
		virtual PEvaluationResult Evaluate(PNodeInstanceData* a_instanceData, PoseCache& a_poseCache, PEvaluationContext& a_evalContext) override;
		virtual void AdvanceTime(PNodeInstanceData* a_instanceData, float a_deltaTime) override;
		virtual void Synchronize(PNodeInstanceData* a_instanceData, PNodeInstanceData* a_ownerInstance, float a_correctionDelta) override;
		virtual bool SetCustomValues(const std::span<PEvaluationResult>& a_values, const std::string_view a_skeleton) override;
		void UpdateTargetValue(InstanceData* a_instanceData);

		inline static Registration _reg{
			"smooth_rand",
			{},
			{
				{ "dur_min", PEvaluationType<float> },
				{ "dur_max", PEvaluationType<float> },
				{ "diff_min", PEvaluationType<float> },
				{ "diff_max", PEvaluationType<float> },
				{ "delay_min", PEvaluationType<float> },
				{ "delay_max", PEvaluationType<float> },
				{ "edge", PEvaluationType<float> },
				{ "syncId", PEvaluationType<uint64_t> }
			},
			PEvaluationType<float>,
			CreateNodeOfType<PSmoothedRandNode>
		};
	};
}