#include "PFullAnimationNode.h"
#include "Animation/FileManager.h"

namespace Animation::Procedural
{
	std::unique_ptr<PNodeInstanceData> PFullAnimationNode::CreateInstanceData()
	{
		auto result = std::make_unique<InstanceData>();
		result->context.Resize(anim->data->num_tracks());
		return result;
	}

	PEvaluationResult PFullAnimationNode::Evaluate(PNodeInstanceData* a_instanceData, PoseCache& a_poseCache, PEvaluationContext& a_evalContext)
	{
		auto inst = static_cast<InstanceData*>(a_instanceData);

		if (inputs[0] != UINT64_MAX) {
			inst->speedMod = std::get<float>(a_evalContext.results[inputs[0]]);
		}

		PoseCache::Handle result = a_poseCache.acquire_handle();
		auto resultSpan = result.get_ozz();

		ozz::animation::SamplingJob sampleJob;
		sampleJob.animation = cachedRawAnim;
		sampleJob.context = &inst->context;
		sampleJob.output = resultSpan;
		sampleJob.ratio = inst->localTime;
		sampleJob.Run();

		return result;
	}

	void PFullAnimationNode::AdvanceTime(PNodeInstanceData* a_instanceData, float a_deltaTime)
	{
		auto inst = static_cast<InstanceData*>(a_instanceData);
		float newTime = inst->localTime + (a_deltaTime * (1.0f + inst->speedMod) / cachedRawAnim->duration());

		//Loops within the time ratio 0-1.
		//If the floor is non-zero (outside 0-1), then it's going to loop this frame.
		float flr = floorf(newTime);
		inst->localTime = newTime - flr;
		inst->looped = flr;
	}

	void PFullAnimationNode::Synchronize(PNodeInstanceData* a_instanceData, PNodeInstanceData* a_ownerInstance, float a_correctionDelta)
	{
		auto inst = static_cast<InstanceData*>(a_instanceData);
		auto owner = static_cast<InstanceData*>(a_ownerInstance);

		inst->localTime = owner->localTime;
		inst->speedMod = owner->speedMod;

		if (a_correctionDelta > 0.0f) {
			AdvanceTime(a_instanceData, a_correctionDelta);
		}
	}

	bool PFullAnimationNode::SetCustomValues(const std::span<PEvaluationResult>& a_values, const std::string_view a_skeleton)
	{
		auto file = FileID{ std::get<RE::BSFixedString>(a_values[0]), "" };
		syncId = std::get<uint64_t>(a_values[1]);
		auto loadedFile = Animation::FileManager::GetSingleton()->DemandAnimation(file, a_skeleton, true);
		if (loadedFile == nullptr) {
			return false;
		}

		anim = std::dynamic_pointer_cast<OzzAnimation>(loadedFile);
		bool result = anim != nullptr;
		if (result) {
			cachedRawAnim = anim->data.get();
		}
		return result;
	}
}