#include "PTwoBoneIKAdjustNode.h"
#include "Settings/Settings.h"
#include "Util/Ozz.h"

namespace Animation::Procedural
{
	std::unique_ptr<PNodeInstanceData> PTwoBoneIKAdjustNode::CreateInstanceData()
	{
		return std::make_unique<InstanceData>();
	}

	PEvaluationResult PTwoBoneIKAdjustNode::Evaluate(PNodeInstanceData* a_instanceData, PoseCache& a_poseCache, PEvaluationContext& a_evalContext)
	{
		// Get node input data.
		PoseCache::Handle& input = std::get<PoseCache::Handle>(a_evalContext.results[inputs[0]]);
		float xOffset = std::get<float>(a_evalContext.results[inputs[1]]);
		float yOffset = std::get<float>(a_evalContext.results[inputs[2]]);
		float zOffset = std::get<float>(a_evalContext.results[inputs[3]]);

		// Acquire a pose handle for this node's output and copy the input pose to the output pose - we only need to make 3 corrections to the pose.
		PoseCache::Handle output = a_poseCache.acquire_handle();
		auto inputSpan = input.get();
		auto outputSpan = output.get();
		std::copy(inputSpan.begin(), inputSpan.end(), outputSpan.begin());

		// Calculate model-space matrices from the pose, and extract the model-space position & rotation of the IK end node.
		a_evalContext.UpdateModelSpaceCache(outputSpan);
		const ozz::math::Float4x4& endNodeMS = a_evalContext.modelSpaceCache[endNode];
		const ozz::math::SimdFloat4& endMSPosition = endNodeMS.cols[3];
		const ozz::math::SimdQuaternion endMSRotation = Util::Ozz::ToNormalizedQuaternion(endNodeMS);

		// Setup IK job params & run.
		ozz::animation::IKTwoBoneJob ikJob;
		ikJob.target = endMSPosition + ozz::math::simd_float4::Load(xOffset, yOffset, zOffset, 0.0f);
		ikJob.pole_vector = ozz::math::simd_float4::zero();
		ikJob.mid_axis = ozz::math::simd_float4::Load3PtrU(&midAxis.x);

		ikJob.soften = 0.95f;
		ikJob.twist_angle = 0.0f;

		ikJob.start_joint = &a_evalContext.modelSpaceCache[startNode];
		ikJob.mid_joint = &a_evalContext.modelSpaceCache[midNode];
		ikJob.end_joint = &a_evalContext.modelSpaceCache[endNode];

		ozz::math::SimdQuaternion corrections[2];
		ikJob.start_joint_correction = &corrections[0];
		ikJob.mid_joint_correction = &corrections[1];
		ikJob.reached = &static_cast<InstanceData*>(a_instanceData)->targetWithinRange;

		if (!ikJob.Run())
			return output;

		// Apply IK corrections to the pose, then update the chain's model-space matrices
		// so that the end node's original model-space rotation can be reapplied to the pose.
		Util::Ozz::MultiplySoATransformQuaternion(startNode, corrections[0], outputSpan);
		Util::Ozz::MultiplySoATransformQuaternion(midNode, corrections[1], outputSpan);
		a_evalContext.UpdateModelSpaceCache(outputSpan, startNode, endNode);
		Util::Ozz::ApplyMSRotationToLocal(endMSRotation, endNode, outputSpan, a_evalContext.modelSpaceCache, a_evalContext.skeleton);

		return output;
	}

	bool PTwoBoneIKAdjustNode::SetCustomValues(const std::span<PEvaluationResult>& a_values, const std::string_view a_skeleton)
	{
		const RE::BSFixedString startName = std::get<RE::BSFixedString>(a_values[0]);
		const RE::BSFixedString midName = std::get<RE::BSFixedString>(a_values[1]);
		const RE::BSFixedString endName = std::get<RE::BSFixedString>(a_values[2]);
		const float midX = std::get<float>(a_values[3]);
		const float midY = std::get<float>(a_values[4]);
		const float midZ = std::get<float>(a_values[5]);
		midAxis = ozz::math::Float3(midX, midY, midZ);

		std::array<std::string_view, 3> nodeNames = {
			startName.c_str(),
			midName.c_str(),
			endName.c_str()
		};

		auto skeleton = Settings::GetSkeleton(std::string{ a_skeleton });

		std::array<int32_t, 3> nodeIdxs;
		if (!Util::Ozz::GetJointIndexes(skeleton->data.get(), nodeNames, nodeIdxs))
			return false;

		startNode = nodeIdxs[0];
		midNode = nodeIdxs[1];
		endNode = nodeIdxs[2];
		return true;
	}
}