#include "PSpringBoneNode.h"
#include "Settings/Settings.h"
#include "Util/Ozz.h"
#include "ozz/base/maths/simd_quaternion.h"

namespace Animation::Procedural
{
	bool SpringPhysicsJob::Run()
	{
		using namespace ozz::math;
		SimdInt4 invertible;
		const Float4x4 rootInverseWS = Invert(*rootTransform, &invertible);
		if (!AreAllTrue1(invertible)) {
			return false;
		}
		const Float4x4 parentInverseMS = Invert(*parentTransform, &invertible);
		if (!AreAllTrue1(invertible)) {
			return false;
		}

		if (!context->initialized) {
			const SimdFloat4& boneMS = boneTransform->cols[3];
			const SimdFloat4 boneLS = TransformPoint(parentInverseMS, boneMS);
			context->restOffset = boneLS;
			context->physicsPosition = boneMS;
			context->previousPosition = boneMS;
			(*prevRootPos) = rootTransform->cols[3];
			context->initialized = true;
		}

		// Get root movement transformed into model-space.
		const SimdFloat4 relativeRoot = rootTransform->cols[3] - (*prevRootPos);
		const SimdFloat4 worldMovementMS = TransformVector(rootInverseWS, relativeRoot);
		context->accumulatedMovement = context->accumulatedMovement + worldMovementMS;
		(*prevRootPos) = rootTransform->cols[3];

		// Add to accumulated time.
		context->accumulatedTime += context->deltaTime;

		// Calculate required physics steps.
		SubStepConstants constants;
		if (context->accumulatedTime >= FIXED_TIMESTEP) {
			BeginStepUpdate(constants);
		}

		uint8_t stepsPerformed = 0;
		while (context->accumulatedTime >= FIXED_TIMESTEP) {
			ProcessPhysicsStep(constants);
			++stepsPerformed;

			if (stepsPerformed > MAX_STEPS_PER_RUN) [[unlikely]] {
				context->accumulatedTime = 0.0f;
			} else {
				context->accumulatedTime -= FIXED_TIMESTEP;
			}
		}

		// Interpolate between previous step and current step.
		const float ratio = context->accumulatedTime > 0.0f ? (context->accumulatedTime / FIXED_TIMESTEP) : 0.0f;
		const SimdFloat4 interpPosition = Lerp(context->previousPosition, context->physicsPosition, simd_float4::Load1(ratio));

		// Transform back to local space for final output.
		const SimdFloat4 localPos = TransformPoint(parentInverseMS, interpPosition);
		*positionOutput = SetW(localPos, simd_float4::zero());
		return true;
	}

	void SpringPhysicsJob::BeginStepUpdate(SubStepConstants& a_constantsOut)
	{
		using namespace ozz::math;

		// Calculate root acceleration in model-space.
		const float deltaTime = context->deltaTime;
		const SimdFloat4 deltaInvSimd = simd_float4::Load1(1.0f / deltaTime);
		const SimdFloat4 worldVelocity = context->accumulatedMovement * deltaInvSimd;
		const SimdFloat4 worldAcceleration = (worldVelocity - context->prevRootVelocity) * deltaInvSimd;
		context->accumulatedMovement = simd_float4::zero();
		context->prevRootVelocity = worldVelocity;

		// Calculate sub-step-constant forces.
		const SimdFloat4 massSimd = simd_float4::Load1(mass);
		const SimdFloat4 gravityForce = gravity * massSimd;
		const SimdFloat4 inertiaForce = -worldAcceleration * massSimd;
		const float oscillationFreq = std::sqrt(stiffness / mass);

		a_constantsOut.force = gravityForce + inertiaForce;
		a_constantsOut.restOffsetMS = TransformPoint(*parentTransform, context->restOffset);
		a_constantsOut.dampingFactor = simd_float4::Load1(std::expf(-damping * oscillationFreq * FIXED_TIMESTEP));
		a_constantsOut.massInverse = 1.0f / mass;
	}

	void SpringPhysicsJob::ProcessPhysicsStep(const SubStepConstants& a_constants)
	{
		using namespace ozz::math;
		constexpr float deltaTime = FIXED_TIMESTEP;
		constexpr float dtSquared = deltaTime * deltaTime;

		const ozz::math::SimdFloat4 currentPos = context->physicsPosition;
		const ozz::math::SimdFloat4 prevPos = context->previousPosition;

		// Calculate spring forces.
		const SimdFloat4 displacement = currentPos - a_constants.restOffsetMS;
		const SimdFloat4 springForce = displacement * simd_float4::Load1(-stiffness);
		const SimdFloat4 totalForce = springForce + a_constants.force;
		const SimdFloat4 acceleration = totalForce * simd_float4::Load1(a_constants.massInverse);

		// Verlet integration = x(t+dt) = 2x(t) - x(t-dt) + a(t)dt^2
		const SimdFloat4 deltaTimeSq = simd_float4::Load1(dtSquared);
		const SimdFloat4 newPhysicsPosition = currentPos * simd_float4::Load1(2.0f) - prevPos + acceleration * deltaTimeSq;

		// Add damping by scaling position change & update physics position.
		const SimdFloat4 positionDiff = newPhysicsPosition - currentPos;
		context->physicsPosition = currentPos + positionDiff * a_constants.dampingFactor;
		context->previousPosition = currentPos;
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
		springJob.prevRootPos = &a_evalContext.prevRootPos;
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