#pragma once
#include "PNode.h"

namespace Animation::Procedural
{
	class POneBoneIKNode : public PNodeT<POneBoneIKNode>
	{
	public:
		uint16_t boneIdx;
		ozz::math::Float3 forwardAxis;
		ozz::math::Float3 upAxis;

		virtual PEvaluationResult Evaluate(PNodeInstanceData* a_instanceData, PoseCache& a_poseCache, PEvaluationContext& a_evalContext) override;
		virtual bool SetCustomValues(const std::span<PEvaluationResult>& a_values, const std::string_view a_skeleton) override;

		inline static Registration _reg{
			"ik_1b",
			{
				{ "pose", PEvaluationType<PoseCache::Handle> },
				{ "target", PEvaluationType<ozz::math::Float4> },
				{ "offset", PEvaluationType<ozz::math::Float4> }
			},
			{
				{ "bone", PEvaluationType<RE::BSFixedString> },
				{ "up_x", PEvaluationType<float> },
				{ "up_y", PEvaluationType<float> },
				{ "up_z", PEvaluationType<float> },
				{ "forward_x", PEvaluationType<float> },
				{ "forward_y", PEvaluationType<float> },
				{ "forward_z", PEvaluationType<float> },
			},
			PEvaluationType<PoseCache::Handle>,
			CreateNodeOfType<POneBoneIKNode>
		};
	};
}