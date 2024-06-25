#pragma once
#include "Generator.h"
#include "Settings/SkeletonDescriptor.h"
#include "Easing.h"
#include "Node.h"
#include "Ozz.h"
#include "IKTwoBoneData.h"
#include "FileManager.h"
#include "SyncInstance.h"
#include "Face/Manager.h"

namespace Animation
{
	class Graph : public FileRequesterBase
	{
	public:
		struct TransitionData
		{
			int startLayer = 0;
			int endLayer = 0;
			float localTime = 0.0f;
			float duration = 0.0f;
			float queuedDuration = 0.0f;
			std::function<void()> onEnd = nullptr;
			CubicInOutEase<float> ease;
		};

		struct EyeTrackingData
		{
			RE::NiNode* lEye = nullptr;
			RE::NiNode* rEye = nullptr;
			RE::NiNode* eyeTarget = nullptr;
			size_t originalLIdx = 0;
			size_t originalRIdx = 0;
		};

		enum FLAGS : uint16_t
		{
			kNoFlags = 0,
			kTemporary = 1u << 0,
			kUnloaded3D = 1u << 1,
			kNoActiveIKChains = 1u << 2,

			kTransitioning = 1u << 3,
			kHasGenerator = 1u << 4,

			kLoadingAnimation = 1u << 5,
			kRequiresEyeTrackUpdate = 1u << 6
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
		std::vector<std::unique_ptr<IKTwoBoneData>> ikJobs;
		std::unique_ptr<Generator> generator = nullptr;
		TransitionData transition;
		std::shared_ptr<SyncInstance> syncInst = nullptr;
		std::shared_ptr<Face::MorphData> faceMorphData = nullptr;
		RE::BSFaceGenAnimationData* faceAnimData = nullptr;
		std::unique_ptr<EyeTrackingData> eyeTrackData;
		FileID activeFile;

		Graph();
		virtual ~Graph() noexcept;

		virtual void OnAnimationReady(const FileID& a_id, std::shared_ptr<OzzAnimation> a_anim);
		virtual void OnAnimationRequested(const FileID& a_id);

		void SetSkeleton(std::shared_ptr<const OzzSkeleton> a_descriptor);
		void GetSkeletonNodes(RE::BGSFadeNode* a_rootNode);
		Transform GetCurrentTransform(size_t nodeIdx);
		void Update(float a_deltaTime);
		IKTwoBoneData* AddIKJob(const std::span<std::string_view, 3> a_nodeNames, const RE::NiTransform& a_initialTargetWorld, const RE::NiPoint3& a_initialPolePtModel, float a_transitionTime);
		bool RemoveIKJob(IKTwoBoneData* a_jobData, float a_transitionTime);
		void StartTransition(std::unique_ptr<Generator> a_dest, float a_transitionTime);
		void UpdateTransition(float a_deltaTime);
		void PushOutput(const std::vector<ozz::math::SoaTransform>& a_output);
		void UpdateRestPose();
		void SnapshotBlend();
		void SnapshotGenerator();
		void ResetRootTransform();
		void ResetRootOrientation();
		void MakeSyncOwner();
		void SyncToGraph(Graph* a_grph);
		void StopSyncing();
		void UpdateFaceAnimData();
		void SetNoBlink(bool a_noBlink);
		void SetFaceMorphsControlled(bool a_controlled, float a_transitionTime);
		XYZTransform GetRootXYZ();
	};
}