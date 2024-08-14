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
#include "PoseCache.h"

namespace Animation
{
	class Sequencer;

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
		};

		enum FLAGS : uint16_t
		{
			kNoFlags = 0,
			kPersistent = 1u << 0,
			kUnloaded3D = 1u << 1,
			kActiveIKChains = 1u << 2,

			kTransitioning = 1u << 3,
			kHasGenerator = 1u << 4,

			kLoadingAnimation = 1u << 5,
			kRequiresEyeTrackUpdate = 1u << 6,
			kLoadingSequencerAnimation = 1u << 7,
			kRequiresFaceDataUpdate = 1u << 8
		};

		enum LastPose : uint8_t
		{
			kNoPose,
			kGeneratedPose,
			kBlendedPose
		};

		enum TRANSITION_TYPE : uint8_t
		{
			kNoTransition = 0,
			kGameToGraph = 1,
			kGraphToGame = 2,
			kGraphSnapshotToGame = 3,
			kGeneratorToGenerator = 4
		};

		struct LOADED_DATA
		{
			RE::BGSFadeNode* rootNode = nullptr;
			ozz::animation::SamplingJob::Context context;
			PoseCache poseCache;
			PoseCache::Handle restPose;
			PoseCache::Handle snapshotPose;
			PoseCache::Handle generatedPose;
			PoseCache::Handle blendedPose;
			std::array<ozz::animation::BlendingJob::Layer, 2> blendLayers;
			TransitionData transition;
			LastPose lastPose = kNoPose;
			std::unique_ptr<EyeTrackingData> eyeTrackData;
			std::shared_ptr<Face::MorphData> faceMorphData = nullptr;
			RE::BSFaceGenAnimationData* faceAnimData = nullptr;
			FileID loadingFile;
		};

		struct UNLOADED_DATA
		{
			FileID restoreFile;
		};

		std::mutex lock;
		SFSE::stl::enumeration<FLAGS, uint16_t> flags = kNoFlags;
		RE::NiPointer<RE::TESObjectREFR> target;
		std::shared_ptr<const OzzSkeleton> skeleton;
		Transform rootTransform;
		RE::NiQuaternion rootOrientation;
		std::vector<std::unique_ptr<Node>> nodes;
		std::vector<std::unique_ptr<IKTwoBoneData>> ikJobs;
		std::shared_ptr<SyncInstance> syncInst = nullptr;
		std::unique_ptr<Sequencer> sequencer = nullptr;
		std::unique_ptr<Generator> generator = nullptr;
		std::unique_ptr<LOADED_DATA> loadedData = nullptr;
		std::unique_ptr<UNLOADED_DATA> unloadedData = nullptr;
#ifdef ENABLE_PERFORMANCE_MONITORING
		float lastUpdateMs = 0.0f;
#endif

		Graph();
		virtual ~Graph() noexcept;

		virtual void OnAnimationReady(const FileID& a_id, std::shared_ptr<IAnimationFile> a_anim);
		virtual void OnAnimationRequested(const FileID& a_id);

		void SetSkeleton(std::shared_ptr<const OzzSkeleton> a_descriptor);
		void GetSkeletonNodes(RE::BGSFadeNode* a_rootNode);
		Transform GetCurrentTransform(size_t nodeIdx);
		void Update(float a_deltaTime, bool a_visible);
		IKTwoBoneData* AddIKJob(const std::span<std::string_view, 3> a_nodeNames, const RE::NiTransform& a_initialTargetWorld, const RE::NiPoint3& a_initialPolePtModel, float a_transitionTime);
		bool RemoveIKJob(IKTwoBoneData* a_jobData, float a_transitionTime);
		void StartTransition(std::unique_ptr<Generator> a_dest, float a_transitionTime);
		void AdvanceTransitionTime(float a_deltaTime);
		void UpdateTransition(const ozz::span<ozz::math::SoaTransform>& a_output);
		void PushAnimationOutput(const std::span<ozz::math::SoaTransform>& a_output);
		void PushRootOutput(bool a_visible);
		void UpdateRestPose();
		void SnapshotPose();
		void ResetRootTransform();
		void ResetRootOrientation();
		void MakeSyncOwner();
		void SyncToGraph(Graph* a_grph);
		void StopSyncing();
		void UpdateFaceAnimData();
		void SetNoBlink(bool a_noBlink);
		void SetFaceMorphsControlled(bool a_controlled, float a_transitionTime);
		void DisableEyeTracking();
		void EnableEyeTracking();
		void DetachSequencer(bool a_transitionOut = true);
		void SetLoaded(bool a_loaded);
		XYZTransform GetRootXYZ();
	};
}