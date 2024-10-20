#include "PSpringBoneNode.h"
#include "Settings/Settings.h"
#include "Util/Ozz.h"
#include "ozz/base/maths/simd_quaternion.h"

namespace Animation::Procedural
{
	bool SpringPhysicsJob::Run()
	{
		using namespace ozz::math;
		const SimdQuaternion rootRot{ ToQuaternion(*rootTransform) };

		// Rotate the root position by the root rotation's inverse so that it's oriented in model-space.
		const SimdFloat4 rootWS = TransformVector(Conjugate(rootRot), rootTransform->cols[3]);
		const SimdFloat4 parentWS = rootWS + parentTransform->cols[3];

		if (!context->initialized) {
			context->restOffset = boneTransform->cols[3] - parentTransform->cols[3];
			context->physicsPositionWS = rootWS + boneTransform->cols[3];
			context->previousPositionWS = context->physicsPositionWS;
			context->initialized = true;
		}

		const SimdFloat4 currentPos = context->physicsPositionWS;
		const SimdFloat4 prevPos = context->previousPositionWS;

		// Calculate spring forces.
		const SimdFloat4 displacement = currentPos - (parentWS + context->restOffset);
		const SimdFloat4 springForce = displacement * simd_float4::Load1(-stiffness);
		const SimdFloat4 gravityForce = gravity * simd_float4::Load1(mass);
		const SimdFloat4 totalForce = springForce + gravityForce;
		const SimdFloat4 acceleration = totalForce * simd_float4::Load1(1.0f / mass);

		// Verlet integration = x(t+dt) = 2x(t) - x(t-dt) + a(t)dt^2
		const float deltaTime = context->deltaTime;
		const SimdFloat4 deltaTimeSq = simd_float4::Load1(deltaTime * deltaTime);
		const SimdFloat4 newPhysicsPosition = currentPos * simd_float4::Load1(2.0f) - prevPos + acceleration * deltaTimeSq;

		// Add damping by scaling position change.
		const float oscillationFreq = std::sqrt(stiffness / mass);
		const SimdFloat4 positionDiff = newPhysicsPosition - currentPos;
		const SimdFloat4 dampingFactor = simd_float4::Load1(std::expf(-damping * oscillationFreq * deltaTime));
		const SimdFloat4 dampedNewPosition = currentPos + positionDiff * dampingFactor;

		// Update physics position.
		context->previousPositionWS = currentPos;
		context->physicsPositionWS = dampedNewPosition;

		// Transform back to local space for final output.
		SimdInt4 invertible;
		const Float4x4 parentInverseMS = Invert(*parentTransform, &invertible);
		if (!AreAllTrue1(invertible)) {
			return false;
		}

		SimdFloat4 localPos = context->physicsPositionWS - rootWS;
		localPos = TransformPoint(parentInverseMS, localPos);
		*positionOutput = SetW(localPos, simd_float4::zero());
		return true;
	}


	void PSpringBoneNode::AdvanceTime(PNodeInstanceData* a_instanceData, float a_deltaTime)
	{
		static_cast<InstanceData*>(a_instanceData)->context.deltaTime = a_deltaTime;
	}

	std::unique_ptr<PNodeInstanceData> PSpringBoneNode::CreateInstanceData()
	{
		return std::make_unique<InstanceData>();
	}

	PEvaluationResult PSpringBoneNode::Evaluate(PNodeInstanceData* a_instanceData, PoseCache& a_poseCache, PEvaluationContext& a_evalContext)
	{
		auto inst = static_cast<InstanceData*>(a_instanceData);
		constexpr float valMin = 0.00001f;

		// Get node input data.
		PoseCache::Handle& input = std::get<PoseCache::Handle>(a_evalContext.results[inputs[0]]);
		const float stiffness = std::max(valMin, std::get<float>(a_evalContext.results[inputs[1]]));
		const float damping = std::max(valMin, std::get<float>(a_evalContext.results[inputs[2]]));
		const float mass = std::max(valMin, std::get<float>(a_evalContext.results[inputs[3]]));
		const ozz::math::Float4& gravity = std::get<ozz::math::Float4>(a_evalContext.results[inputs[4]]);

		// Acquire a pose handle for this node's output and copy the input pose to the output pose - we only need to make 1 correction to the pose.
		PoseCache::Handle output = a_poseCache.acquire_handle();
		auto inputSpan = input.get();
		auto outputSpan = output.get();
		std::copy(inputSpan.begin(), inputSpan.end(), outputSpan.begin());

		// If effectively paused, don't run any calculations as a deltaTime of 0 will break velocity updates.
		if (inst->context.deltaTime < 0.00001f)
			return output;

		// Update model-space cache.
		a_evalContext.UpdateModelSpaceCache(outputSpan, ozz::animation::Skeleton::kNoParent, boneIdx);

		// Setup spring job params & run.
		SpringPhysicsJob springJob;
		springJob.stiffness = stiffness;
		springJob.damping = damping;
		springJob.mass = mass;
		springJob.gravity = ozz::math::simd_float4::Load3PtrU(&gravity.x);

		springJob.boneTransform = &a_evalContext.modelSpaceCache[boneIdx];
		springJob.parentTransform = &a_evalContext.modelSpaceCache[parentIdx];
		springJob.rootTransform = a_evalContext.rootTransform;
		springJob.context = &inst->context;

		ozz::math::SimdFloat4 positionOutput;
		springJob.positionOutput = &positionOutput;

		if (!springJob.Run())
			return output;

		Util::Ozz::ApplySoATransformTranslation(boneIdx, positionOutput, outputSpan);
		return output;
	}

	bool PSpringBoneNode::SetCustomValues(const std::span<PEvaluationResult>& a_values, const std::string_view a_skeleton)
	{
		const RE::BSFixedString& boneName = std::get<RE::BSFixedString>(a_values[0]);

		std::array<std::string_view, 1> nodeNames = { boneName.c_str() };
		auto skeleton = Settings::GetSkeleton(std::string{ a_skeleton });

		std::array<int32_t, 1> nodeIdxs;
		if (!Util::Ozz::GetJointIndexes(skeleton->data.get(), nodeNames, nodeIdxs)) {
			return false;
		}

		boneIdx = nodeIdxs[0];
		int sParent = skeleton->data->joint_parents()[boneIdx];
		if (sParent == ozz::animation::Skeleton::kNoParent) {
			return false;
		}

		parentIdx = sParent;
		return true;
	}
}