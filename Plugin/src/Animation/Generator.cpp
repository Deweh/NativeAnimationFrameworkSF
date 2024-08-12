#include "Generator.h"

namespace Animation
{
	void Generator::Generate(PoseCache&) {}
	bool Generator::HasFaceAnimation() { return false; }
	void Generator::SetFaceMorphData(Face::MorphData* morphData){}
	void Generator::SetOutput(PoseCache::Handle* hndl) { output = hndl; }
	void Generator::SetContext(ozz::animation::SamplingJob::Context* ctxt) { context = ctxt; }
	void Generator::OnDetaching() {}
	void Generator::AdvanceTime(float deltaTime) { localTime += deltaTime * speed; }
	const std::string_view Generator::GetSourceFile() { return ""; }

	LinearClipGenerator::LinearClipGenerator(const std::shared_ptr<OzzAnimation>& a_anim)
	{
		anim = a_anim;
		duration = anim->data->duration();
	}

	void LinearClipGenerator::Generate(PoseCache& cache)
	{
		ozz::animation::SamplingJob sampleJob;
		sampleJob.animation = anim->data.get();
		sampleJob.context = context;
		sampleJob.output = output->get_ozz();
		sampleJob.ratio = localTime / duration;
		sampleJob.Run();

		if (anim->faceData != nullptr && faceMorphData != nullptr) {
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
			localTime += deltaTime * speed;
			if (localTime > duration || localTime < 0.0f) {
				localTime = std::fmodf(std::abs(localTime), duration);
				rootResetRequired = true;
			}
		}
	}

	const std::string_view LinearClipGenerator::GetSourceFile()
	{
		return anim->extra.id.file.QPath();
	}

	ProceduralGenerator::ProceduralGenerator(const std::shared_ptr<Procedural::PGraph>& a_graph, const OzzSkeleton* a_skeleton)
	{
		pGraph = a_graph;
		a_graph->InitInstanceData(pGraphInstance, a_skeleton);
	}

	void ProceduralGenerator::Generate(PoseCache& cache)
	{
		auto result = pGraph->Evaluate(pGraphInstance, cache);
		auto outSpan = output->get();
		std::copy(result.begin(), result.end(), outSpan.begin());
	}

	void ProceduralGenerator::AdvanceTime(float deltaTime)
	{
		pGraph->AdvanceTime(pGraphInstance, deltaTime);
	}
}