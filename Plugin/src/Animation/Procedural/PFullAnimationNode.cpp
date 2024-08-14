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

	PEvaluationResult PFullAnimationNode::Evaluate(PNodeInstanceData* a_instanceData, PoseCache& a_poseCache, std::unordered_map<PNode*, PEvaluationResult>& a_results)
	{
		auto inst = static_cast<InstanceData*>(a_instanceData);
		PoseCache::Handle result = a_poseCache.acquire_handle();

		ozz::animation::SamplingJob sampleJob;
		sampleJob.animation = anim->data.get();
		sampleJob.context = &inst->context;
		sampleJob.output = result.get_ozz();
		sampleJob.ratio = inst->localTime / anim->data->duration();
		sampleJob.Run();

		return result;
	}

	void PFullAnimationNode::AdvanceTime(PNodeInstanceData* a_instanceData, float a_deltaTime)
	{
		auto inst = static_cast<InstanceData*>(a_instanceData);
		auto duration = anim->data->duration();
		inst->localTime += a_deltaTime;
		if (inst->localTime > duration || inst->localTime < 0.0f) {
			inst->localTime = std::fmodf(std::abs(inst->localTime), duration);
			inst->looped = true;
		} else {
			inst->looped = false;
		}
	}

	bool PFullAnimationNode::SetCustomValues(const std::span<PEvaluationResult>& a_values, const std::string_view a_skeleton)
	{
		auto file = FileID{ std::get<std::string>(a_values[0]), "" };
		auto loadedFile = Animation::FileManager::GetSingleton()->DemandAnimation(file, a_skeleton, true);
		if (loadedFile == nullptr) {
			return false;
		}

		anim = std::dynamic_pointer_cast<OzzAnimation>(loadedFile);
		return anim != nullptr;
	}
}