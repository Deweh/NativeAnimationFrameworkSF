#include "POneBoneIKNode.h"
#include "ozz/animation/runtime/ik_aim_job.h"
#include "ozz/base/maths/simd_quaternion.h"
#include "Util/Ozz.h"
#include "Settings/Settings.h"

namespace Animation::Procedural
{
	PEvaluationResult POneBoneIKNode::Evaluate(PNodeInstanceData* a_instanceData, PoseCache& a_poseCache, PEvaluationContext& a_evalContext)
	{
		// Get node input data.
		PoseCache::Handle& input = std::get<PoseCache::Handle>(a_evalContext.results[inputs[0]]);
		const ozz::math::Float4& target = std::get<ozz::math::Float4>(a_evalContext.results[inputs[1]]);
		const ozz::math::Float4& offset = std::get<ozz::math::Float4>(a_evalContext.results[inputs[2]]);

		// Acquire a pose handle for this node's output and copy the input pose to the output pose - we only need to make 1 correction to the pose.
		PoseCache::Handle output = a_poseCache.acquire_handle();
		auto inputSpan = input.get();
		auto outputSpan = output.get();
		std::copy(inputSpan.begin(), inputSpan.end(), outputSpan.begin());

		// Update model space transforms.
		a_evalContext.UpdateModelSpaceCache(outputSpan, ozz::animation::Skeleton::kNoParent, boneIdx);

		// Setup IK job params & run.
		ozz::animation::IKAimJob ikJob;
		ozz::math::SimdQuaternion correction;
		ikJob.target = ozz::math::simd_float4::Load3PtrU(&target.x);
		ikJob.pole_vector = ozz::math::simd_float4::zero();
		ikJob.forward = ozz::math::simd_float4::Load3PtrU(&forwardAxis.x);
		ikJob.up = ozz::math::simd_float4::Load3PtrU(&upAxis.x);

		ikJob.offset = ozz::math::simd_float4::Load3PtrU(&offset.x);
		ikJob.joint = &a_evalContext.modelSpaceCache[boneIdx];
		ikJob.joint_correction = &correction;

		if (!ikJob.Run())
			return output;

		// Apply IK corrections to the pose.
		Util::Ozz::MultiplySoATransformQuaternion(boneIdx, correction, outputSpan);

		return output;
	}

	bool POneBoneIKNode::SetCustomValues(const std::span<PEvaluationResult>& a_values, const std::string_view a_skeleton)
	{
		const RE::BSFixedString boneName = std::get<RE::BSFixedString>(a_values[0]);
		upAxis = {
			std::get<float>(a_values[1]),
			std::get<float>(a_values[2]),
			std::get<float>(a_values[3]),
		};
		forwardAxis = {
			std::get<float>(a_values[4]),
			std::get<float>(a_values[5]),
			std::get<float>(a_values[6]),
		};

		std::array<std::string_view, 1> nodeNames = { boneName.c_str() };
		auto skeleton = Settings::GetSkeleton(std::string{ a_skeleton });

		std::array<int32_t, 1> nodeIdxs;
		if (!Util::Ozz::GetJointIndexes(skeleton->data.get(), nodeNames, nodeIdxs)) {
			return false;
		}

		boneIdx = nodeIdxs[0];
		return true;
	}
}