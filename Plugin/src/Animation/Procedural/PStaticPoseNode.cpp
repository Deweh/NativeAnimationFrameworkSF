#include "PStaticPoseNode.h"
#include "Animation/FileManager.h"

namespace Animation::Procedural
{
	PEvaluationResult PStaticPoseNode::Evaluate(PNodeInstanceData* a_instanceData, PoseCache& a_poseCache, PEvaluationContext& a_evalContext)
	{
		PoseCache::Handle result = a_poseCache.acquire_handle();
		auto resultSpan = result.get();
		std::copy(pose.begin(), pose.end(), resultSpan.begin());

		return result;
	}

	bool PStaticPoseNode::SetCustomValues(const std::span<PEvaluationResult>& a_values, const std::string_view a_skeleton)
	{
		auto file = FileID{ std::get<RE::BSFixedString>(a_values[0]), "" };
		auto loadedFile = Animation::FileManager::GetSingleton()->DemandAnimation(file, a_skeleton, true);
		if (loadedFile == nullptr) {
			return false;
		}

		auto anim = std::dynamic_pointer_cast<OzzAnimation>(loadedFile);
		if (anim == nullptr) {
			return false;
		}

		pose.reserve(anim->data->num_soa_tracks());
		pose.resize(anim->data->num_soa_tracks());

		ozz::animation::SamplingJob::Context context;
		context.Resize(anim->data->num_tracks());

		ozz::animation::SamplingJob sampleJob;
		sampleJob.animation = anim->data.get();
		sampleJob.context = &context;
		sampleJob.output = ozz::make_span(pose);
		sampleJob.ratio = 0.0f;
		return sampleJob.Run();
	}
}