#include "Generator.h"
#include "Util/String.h"
#include "Procedural/PVariableNode.h"

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
	void Generator::Synchronize(Generator* a_other, float a_correctionDelta) {}
	GenType Generator::GetType() { return GenType::kBase; }

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
		sampleJob.ratio = localTime;
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
		if (paused)
			return;

		float newTime = localTime + (deltaTime * speed / duration);

		//Loops within the time ratio 0-1.
		//If the floor is non-zero (outside 0-1), then it's going to loop this frame.
		float flr = floorf(newTime);
		localTime = newTime - flr;
		rootResetRequired = flr;
	}

	const std::string_view LinearClipGenerator::GetSourceFile()
	{
		return anim->extra.id.file.QPath();
	}

	void LinearClipGenerator::Synchronize(Generator* a_other, float a_correctionDelta)
	{
		if (a_other->GetType() != GenType::kLinear)
			return;

		localTime = a_other->localTime + a_correctionDelta;
	}

	GenType LinearClipGenerator::GetType()
	{
		return GenType::kLinear;
	}

	ProceduralGenerator::ProceduralGenerator(const std::shared_ptr<Procedural::PGraph>& a_graph)
	{
		pGraph = a_graph;
		a_graph->InitInstanceData(pGraphInstance);
	}

	bool ProceduralGenerator::SetVariable(const std::string_view a_name, float a_value)
	{
		if (auto iter = pGraphInstance.variableMap.find(Util::String::ToLower(a_name)); iter != pGraphInstance.variableMap.end()) {
			iter->second->value = a_value;
			return true;
		} else {
			return false;
		}
	}

	float ProceduralGenerator::GetVariable(const std::string_view a_name)
	{
		if (auto iter = pGraphInstance.variableMap.find(Util::String::ToLower(a_name)); iter != pGraphInstance.variableMap.end()) {
			return iter->second->value;
		} else {
			return 0.0f;
		}
	}

	void ProceduralGenerator::ForEachVariable(const std::function<void(const std::string_view, float&)> a_func)
	{
		for (auto& v : pGraphInstance.variableMap) {
			a_func(v.first, v.second->value);
		}
	}

	void ProceduralGenerator::Generate(PoseCache& cache)
	{
		auto result = pGraph->Evaluate(pGraphInstance, cache);
		auto outSpan = output->get();
		std::copy(result.begin(), result.end(), outSpan.begin());
	}

	void ProceduralGenerator::AdvanceTime(float deltaTime)
	{
		rootResetRequired = pGraph->AdvanceTime(pGraphInstance, (deltaTime * speed) * static_cast<float>(!paused));
	}

	const std::string_view ProceduralGenerator::GetSourceFile()
	{
		return pGraph->extra.id.file.QPath();
	}

	void ProceduralGenerator::Synchronize(Generator* a_other, float a_correctionDelta)
	{
		if (a_other->GetType() != GenType::kProcedural)
			return;

		auto otherProcGen = static_cast<ProceduralGenerator*>(a_other);
		pGraph->Synchronize(pGraphInstance, otherProcGen->pGraphInstance, otherProcGen->pGraph.get(), a_correctionDelta);
	}

	GenType ProceduralGenerator::GetType()
	{
		return GenType::kProcedural;
	}
}