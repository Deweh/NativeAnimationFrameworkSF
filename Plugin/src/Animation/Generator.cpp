#include "Generator.h"

namespace Animation
{
	void Generator::Generate(float) {}
	bool Generator::HasFaceAnimation() { return false; }
	void Generator::SetFaceMorphData(Face::MorphData* morphData){}
	void Generator::SetOutput(const ozz::span<ozz::math::SoaTransform>& span) { output = span; }
	void Generator::SetContext(ozz::animation::SamplingJob::Context* ctxt) { context = ctxt; }
	void Generator::OnDetaching() {}
	void Generator::AdvanceTime(float deltaTime) { localTime += deltaTime; }

	void LinearClipGenerator::Generate(float deltaTime)
	{
		AdvanceTime(deltaTime);

		ozz::animation::SamplingJob sampleJob;
		sampleJob.animation = anim->data.get();
		sampleJob.context = context;
		sampleJob.output = ozz::make_span(output);
		sampleJob.ratio = localTime / duration;
		sampleJob.Run();

		if (anim->faceData != nullptr) {
			ozz::animation::FloatTrackSamplingJob trackSampleJob;
			trackSampleJob.ratio = sampleJob.ratio;
			auto d = faceMorphData->lock();
			for (size_t i = 0; i < RE::BSFaceGenAnimationData::morphSize; i++) {
				trackSampleJob.result = &d->current[i];
				trackSampleJob.track = &anim->faceData->tracks[i];
				trackSampleJob.Run();
			}
		}
	}

	bool LinearClipGenerator::HasFaceAnimation()
	{
		return anim->faceData != nullptr;
	}

	void LinearClipGenerator::SetFaceMorphData(Face::MorphData* morphData)
	{
		faceMorphData = morphData;
	}

	void LinearClipGenerator::AdvanceTime(float deltaTime)
	{
		if (!paused) {
			localTime += deltaTime;
			if (localTime > duration || localTime < 0.0f) {
				localTime = std::fmodf(std::abs(localTime), duration);
				rootResetRequired = true;
			}
		}
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

	bool AdditiveGenerator::HasFaceAnimation()
	{
		return baseGen->HasFaceAnimation();
	}

	void AdditiveGenerator::SetFaceMorphData(Face::MorphData* morphData)
	{
		baseGen->SetFaceMorphData(morphData);
	}

	void AdditiveGenerator::AdvanceTime(float deltaTime)
	{
		baseGen->AdvanceTime(deltaTime);
	}
}