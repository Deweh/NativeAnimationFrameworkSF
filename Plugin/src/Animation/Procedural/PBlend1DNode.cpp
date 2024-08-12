#include "PBlend1DNode.h"

namespace Animation::Procedural
{
	PEvaluationResult PBlend1DNode::Evaluate(PNodeInstanceData* a_instanceData, PoseCache& a_poseCache, std::unordered_map<PNode*, PEvaluationResult>& a_results)
	{
		auto& pose1Input = std::get<PoseCache::Handle>(a_results[inputs[0]]);
		auto& pose2Input = std::get<PoseCache::Handle>(a_results[inputs[1]]);
		auto& valueInput = std::get<float>(a_results[inputs[2]]);
		auto output = a_poseCache.acquire_handle();

		std::array<ozz::animation::BlendingJob::Layer, 2> blendLayers;
		blendLayers[0].weight = valueInput;
		blendLayers[0].transform = pose1Input.get_ozz();
		blendLayers[1].weight = 1.0f - valueInput;
		blendLayers[1].transform = pose2Input.get_ozz();

		ozz::animation::BlendingJob blendJob;
		blendJob.output = output.get_ozz();
		blendJob.rest_pose = pose1Input.get_ozz();
		blendJob.threshold = 1.0f;

		blendJob.Run();

		return output;
	}
}