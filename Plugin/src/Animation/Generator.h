#pragma once
#include "Transform.h"
#include "Ozz.h"
#include "Face/Manager.h"
#include "Procedural/PGraph.h"

namespace Animation
{
	class Generator
	{
	public:
		bool rootResetRequired = false;
		bool paused = false;
		float localTime = 0.0f;
		float duration = 0.0f;
		float speed = 1.0f;
		Transform localRootTransform;
		PoseCache::Handle* output;
		ozz::animation::SamplingJob::Context* context = nullptr;

		virtual void Generate(PoseCache& cache);
		virtual bool HasFaceAnimation();
		virtual void SetFaceMorphData(Face::MorphData* morphData);
		virtual void SetOutput(PoseCache::Handle* hndl);
		virtual void SetContext(ozz::animation::SamplingJob::Context* ctxt);
		virtual void OnDetaching();
		virtual void AdvanceTime(float deltaTime);
		virtual const std::string_view GetSourceFile();

		virtual ~Generator() = default;
	};

	class ProceduralGenerator : public Generator
	{
	public:
		std::shared_ptr<Procedural::PGraph> pGraph;
		Procedural::PGraph::InstanceData pGraphInstance;

		ProceduralGenerator(const std::shared_ptr<Procedural::PGraph>& a_graph, const OzzSkeleton* a_skeleton);

		virtual void Generate(PoseCache& cache);
		virtual void AdvanceTime(float deltaTime);

		virtual ~ProceduralGenerator() = default;
	};

	class LinearClipGenerator : public Generator
	{
	public:
		std::shared_ptr<OzzAnimation> anim = nullptr;
		Face::MorphData* faceMorphData = nullptr;

		LinearClipGenerator(const std::shared_ptr<OzzAnimation>& a_anim);

		virtual void Generate(PoseCache& cache) override;
		virtual bool HasFaceAnimation() override;
		virtual void SetFaceMorphData(Face::MorphData* morphData) override;
		virtual void AdvanceTime(float deltaTime) override;
		virtual const std::string_view GetSourceFile() override;
		virtual ~LinearClipGenerator() = default;
	};
}