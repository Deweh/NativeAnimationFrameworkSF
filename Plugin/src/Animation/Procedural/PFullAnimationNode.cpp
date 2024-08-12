#include "PFullAnimationNode.h"
#include "Animation/FileManager.h"

namespace Animation::Procedural
{
	std::unique_ptr<PNodeInstanceData> PFullAnimationNode::CreateInstanceData(const OzzSkeleton* a_skeleton)
	{
		auto result = std::make_unique<InstanceData>();
		result->anim = FileManager::GetSingleton()->DemandAnimation(file, a_skeleton->name);
		result->context.Resize(a_skeleton->data->num_joints());
		return result;
	}

	PEvaluationResult PFullAnimationNode::Evaluate(PNodeInstanceData* a_instanceData, PoseCache& a_poseCache, std::unordered_map<PNode*, PEvaluationResult>& a_results)
	{
		auto inst = static_cast<InstanceData*>(a_instanceData);
		PoseCache::Handle result = a_poseCache.acquire_handle();

		ozz::animation::SamplingJob sampleJob;
		sampleJob.animation = inst->anim->data.get();
		sampleJob.context = &inst->context;
		sampleJob.output = result.get_ozz();
		sampleJob.ratio = inst->localTime / inst->anim->data->duration();
		sampleJob.Run();

		return result;
	}

	void PFullAnimationNode::AdvanceTime(PNodeInstanceData* a_instanceData, float a_deltaTime)
	{
		auto inst = static_cast<InstanceData*>(a_instanceData);
		auto duration = inst->anim->data->duration();
		if (!inst->paused) {
			inst->localTime += a_deltaTime;
			if (inst->localTime > duration || inst->localTime < 0.0f) {
				inst->localTime = std::fmodf(std::abs(inst->localTime), duration);
				inst->looped = true;
			} else {
				inst->looped = false;
			}
		}
	}
}