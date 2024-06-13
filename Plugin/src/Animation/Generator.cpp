#include "Generator.h"

namespace Animation
{
	void Generator::Generate(float) {}
	void Generator::SetOutput(const ozz::span<ozz::math::SoaTransform>& span) { output = span; }
	void Generator::SetContext(ozz::animation::SamplingJob::Context* ctxt) { context = ctxt; }
	void Generator::OnDetaching() {}

	void LinearClipGenerator::Generate(float deltaTime)
	{
		if (!paused) {
			localTime += deltaTime;
			if (localTime > duration || localTime < 0.0f) {
				localTime = std::fmodf(std::abs(localTime), duration);
				rootResetRequired = true;
			}
		}

		ozz::animation::SamplingJob sampleJob;
		sampleJob.animation = anim->data.get();
		sampleJob.context = context;
		sampleJob.output = ozz::make_span(output);
		sampleJob.ratio = localTime / duration;
		sampleJob.Run();
	}

	void AdditiveGenerator::SetRestPose(const std::vector<ozz::math::SoaTransform>& pose)
	{
		restPose = pose;
		if (!baseGenOutput.empty() && restPose.size() > baseGenOutput.size()) {
			restPose.resize(baseGenOutput.size());
		}
	}

	void AdditiveGenerator::Generate(float deltaTime)
	{
		if (!paused) {
			baseGen->Generate(deltaTime);
			std::array<ozz::animation::BlendingJob::Layer, 1> additiveLayers;
			additiveLayers[0].weight = additiveWeight;
			additiveLayers[0].transform = ozz::make_span(baseGenOutput);

			ozz::animation::BlendingJob blendJob;
			blendJob.additive_layers = ozz::make_span(additiveLayers);
			blendJob.output = ozz::make_span(output);
			blendJob.rest_pose = ozz::make_span(restPose);
			blendJob.threshold = 1.0f;

			blendJob.Run();
		}
	}

	void AdditiveGenerator::SetOutput(const ozz::span<ozz::math::SoaTransform>& span)
	{
		Generator::SetOutput(span);
		baseGenOutput.resize(span.size());
		baseGen->SetOutput(ozz::make_span(baseGenOutput));
		if (restPose.size() > baseGenOutput.size()) {
			restPose.resize(baseGenOutput.size());
		}
	}

	void AdditiveGenerator::SetContext(ozz::animation::SamplingJob::Context* ctxt)
	{
		Generator::SetContext(ctxt);
		baseGen->SetContext(ctxt);
	}
}