#pragma once
#include "PNode.h"

namespace Animation::Procedural
{
	class PTwoBoneIKAdjustNode : public PNodeT<PTwoBoneIKAdjustNode>
	{
	public:
		struct InstanceData : public PNodeInstanceData
		{
			bool targetWithinRange = false;
		};

		ozz::math::Float3 midAxis;
		uint16_t startNode;
		uint16_t midNode;
		uint16_t endNode;

		virtual std::unique_ptr<PNodeInstanceData> CreateInstanceData() override;
		virtual PEvaluationResult Evaluate(PNodeInstanceData* a_instanceData, PoseCache& a_poseCache, PEvaluationContext& a_evalContext) override;
		virtual bool SetCustomValues(const std::span<PEvaluationResult>& a_values, const std::string_view a_skeleton) override;

		inline static Registration _reg{
			"ik_2b_adj",
			{
				{ "pose", PEvaluationType<PoseCache::Handle> },
				{ "target", PEvaluationType<ozz::math::Float4> }
			},
			{
				{ "start_node", PEvaluationType<RE::BSFixedString> },
				{ "mid_node", PEvaluationType<RE::BSFixedString> },
				{ "end_node", PEvaluationType<RE::BSFixedString> },
				{ "mid_x", PEvaluationType<float> },
				{ "mid_y", PEvaluationType<float> },
				{ "mid_z", PEvaluationType<float> }
			},
			PEvaluationType<PoseCache::Handle>,
			CreateNodeOfType<PTwoBoneIKAdjustNode>
		};
	};
}