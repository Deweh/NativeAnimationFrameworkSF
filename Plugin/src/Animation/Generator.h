#pragma once
#include "Transform.h"
#include "Ozz.h"
#include "Face/Manager.h"
#include "Procedural/PGraph.h"
#include "SyncInstance.h"

namespace Animation
{
	enum class GenType : uint8_t
	{
		kBase,
		kLinear,
		kProcedural
	};

	class Generator
	{
	public:
		bool looped = false;
		bool paused = false;
		float localTime = 0.0f;
		float duration = 0.0f;
		float speed = 1.0f;

		virtual std::span<ozz::math::SoaTransform> Generate(PoseCache& cache);
		virtual bool HasFaceAnimation();
		virtual void SetFaceMorphData(Face::MorphData* morphData);
		virtual void SetOutput(const std::span<ozz::math::Float4x4>& a_modelSpaceCache, const ozz::animation::Skeleton* a_skeleton, PoseCache::Handle* a_restPose);
		virtual void SetRootTransform(const ozz::math::Float4x4* a_transform);
		virtual void OnDetaching();
		virtual void AdvanceTime(float deltaTime);
		virtual const std::string_view GetSourceFile();
		virtual void Synchronize(Generator* a_other, float a_correctionDelta);
		virtual bool RequiresRestPose();
		virtual GenType GetType();

		virtual ~Generator() = default;
	};

	class ProceduralGenerator : public Generator
	{
	public:
		std::shared_ptr<Procedural::PGraph> pGraph;
		Procedural::PGraph::InstanceData pGraphInstance;

		ProceduralGenerator(const std::shared_ptr<Procedural::PGraph>& a_graph);

		bool SetVariable(const std::string_view a_name, float a_value);
		float GetVariable(const std::string_view a_name);
		void ForEachVariable(const std::function<void(const std::string_view, float&)> a_func);
		virtual std::span<ozz::math::SoaTransform> Generate(PoseCache& cache) override;
		virtual void SetOutput(const std::span<ozz::math::Float4x4>& a_modelSpaceCache, const ozz::animation::Skeleton* a_skeleton, PoseCache::Handle* a_restPose) override;
		virtual void SetRootTransform(const ozz::math::Float4x4* a_transform) override;
		virtual void AdvanceTime(float deltaTime) override;
		virtual const std::string_view GetSourceFile() override;
		virtual void Synchronize(Generator* a_other, float a_correctionDelta) override;
		virtual bool RequiresRestPose() override;
		virtual GenType GetType() override;

		virtual ~ProceduralGenerator() = default;
	};

	class LinearClipGenerator : public Generator
	{
	public:
		ozz::animation::SamplingJob::Context context;
		std::shared_ptr<OzzAnimation> anim = nullptr;
		Face::MorphData* faceMorphData = nullptr;
		PoseCache::Handle output;

		LinearClipGenerator(const std::shared_ptr<OzzAnimation>& a_anim);

		virtual std::span<ozz::math::SoaTransform> Generate(PoseCache& cache) override;
		virtual bool HasFaceAnimation() override;
		virtual void SetFaceMorphData(Face::MorphData* morphData) override;
		virtual void AdvanceTime(float deltaTime) override;
		virtual const std::string_view GetSourceFile() override;
		virtual void Synchronize(Generator* a_other, float a_correctionDelta);
		virtual GenType GetType();

		virtual ~LinearClipGenerator() = default;
	};
}