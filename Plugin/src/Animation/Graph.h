#pragma once
#include "Generator.h"
#include "Settings/SkeletonDescriptor.h"
#include "Easing.h"
#include "Node.h"
#include "Ozz.h"

namespace Animation
{
	class Graph
	{
	public:
		struct TransitionData
		{
			int startLayer;
			int endLayer;
			float localTime = 0.0f;
			float duration = 0.0f;
			std::function<void()> onEnd = nullptr;
			CubicInOutEase<float> ease;
		};

		enum FLAGS : uint16_t
		{
			kNoFlags = 0,
			kTemporary = 1u << 0,
			kUnloaded3D = 1u << 1,
			kNoActiveIKChains = 1u << 2,

			kTransitioning = 1u << 3,
			kHasGenerator = 1u << 4
		};

		enum TRANSITION_TYPE : uint8_t
		{
			kNoTransition = 0,
			kGameToGraph = 1,
			kGraphToGame = 2,
			kGraphSnapshotToGame = 3,
			kGeneratorToGenerator = 4
		};

		std::mutex lock;
		SFSE::stl::enumeration<FLAGS, uint16_t> flags = kNoFlags;
		TRANSITION_TYPE transitionType = kNoTransition;
		RE::NiPointer<RE::TESObjectREFR> target;

		std::shared_ptr<const OzzSkeleton> skeleton;
		std::vector<std::unique_ptr<Node>> nodes;
		RE::BGSFadeNode* rootNode = nullptr;
		Transform rootTransform;
		RE::NiQuaternion rootOrientation;

		ozz::animation::SamplingJob::Context context;
		std::vector<ozz::math::SoaTransform> restPose;
		std::vector<ozz::math::SoaTransform> snapshotPose;
		std::vector<ozz::math::SoaTransform> generatedPose;
		std::vector<ozz::math::SoaTransform> blendedPose;
		std::array<ozz::animation::BlendingJob::Layer, 2> blendLayers;
		std::unique_ptr<Generator> generator = nullptr;
		TransitionData transition;

		Graph();

		void SetSkeleton(std::shared_ptr<const OzzSkeleton> a_descriptor);
		void GetSkeletonNodes(RE::BGSFadeNode* a_rootNode);
		Transform GetCurrentTransform(size_t nodeIdx);
		void Update(float a_deltaTime);
		void StartTransition(std::unique_ptr<Generator> a_dest, float a_transitionTime);
		void UpdateTransition(float a_deltaTime);
		void PushOutput(const std::vector<ozz::math::SoaTransform>& a_output);
		void UpdateRestPose();
		void SnapshotCurrentPose();
		void ResetRootTransform();
		void ResetRootOrientation();
		XYZTransform GetRootXYZ();
	};
}