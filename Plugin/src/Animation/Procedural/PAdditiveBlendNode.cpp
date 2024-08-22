#include "PAdditiveBlendNode.h"

namespace Animation::Procedural
{
	PEvaluationResult PAdditiveBlendNode::Evaluate(PNodeInstanceData* a_instanceData, PoseCache& a_poseCache, PEvaluationContext& a_evalContext)
	{
		auto& addInput = std::get<PoseCache::Handle>(a_evalContext.results[inputs[0]]);
		auto& fullInput = std::get<PoseCache::Handle>(a_evalContext.results[inputs[1]]);
		auto& valueInput = std::get<float>(a_evalContext.results[inputs[2]]);
		auto output = a_poseCache.acquire_handle();

		std::array<ozz::animation::BlendingJob::Layer, 1> blendLayers;
		blendLayers[0].weight = valueInput;
		blendLayers[0].transform = addInput.get_ozz();

		ozz::animation::BlendingJob blendJob;
		blendJob.additive_layers = ozz::make_span(blendLayers);
		blendJob.output = output.get_ozz();
		blendJob.rest_pose = fullInput.get_ozz();
		blendJob.threshold = 1.0f;

		blendJob.Run();

		return output;
	}
}