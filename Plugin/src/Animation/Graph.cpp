#include "Graph.h"
#include "Util/Math.h"
#include "Tasks/MainLoop.h"
#include "Node.h"
#include "Face/Manager.h"
#include "Util/Timing.h"
#include "Sequencer.h"
#include "GraphManager.h"

namespace Animation
{
	Graph::Graph()
	{
		flags.set(FLAGS::kUnloaded3D);
	}

	Graph::~Graph() noexcept
	{
		StopSyncing();
		if (loadedData) {
			Face::Manager::GetSingleton()->OnAnimDataChange(loadedData->faceAnimData, nullptr);
		}
		EnableEyeTracking();
	}

	void Graph::OnAnimationReady(const FileID& a_id, std::shared_ptr<IAnimationFile> a_anim)
	{
		std::unique_lock l{ lock };
		if (!loadedData) {
			return;
		}

		if (sequencer && sequencer->OnAnimationReady(a_id, a_anim)) {
			return;
		}

		if (flags.all(FLAGS::kLoadingAnimation)) {
			if (a_id != loadedData->loadingFile) {
				return;
			}

			flags.reset(FLAGS::kLoadingAnimation);
			if (a_anim != nullptr) {
				StartTransition(a_anim->CreateGenerator(), loadedData->transition.queuedDuration);
			}
		}
	}

	void Graph::OnAnimationRequested(const FileID& a_id)
	{
		if (!loadedData) {
			return;
		}

		if (sequencer && sequencer->OnAnimationRequested(a_id)) {
			return;
		}

		if (flags.all(FLAGS::kLoadingAnimation)) {
			FileManager::GetSingleton()->CancelAnimationRequest(loadedData->loadingFile, weak_from_this());
		} else {
			flags.set(FLAGS::kLoadingAnimation);
		}
		
		loadedData->loadingFile = a_id;
	}

	void Graph::SetSkeleton(std::shared_ptr<const OzzSkeleton> a_descriptor)
	{
		skeleton = a_descriptor;
	}

	void Graph::GetSkeletonNodes(RE::BGSFadeNode* a_rootNode) {
		bool isLoaded = a_rootNode != nullptr;

		nodes.clear();
		EnableEyeTracking();
		SetLoaded(isLoaded);

		if (isLoaded) {
			auto& l = loadedData;
			l->rootNode = a_rootNode;

			if (l->eyeTrackData) {
				l->eyeTrackData->lEye = nullptr;
				l->eyeTrackData->rEye = nullptr;
				l->eyeTrackData->eyeTarget = nullptr;
			}

			ResetRootTransform();
			for (auto& name : skeleton->data->joint_names()) {
				RE::NiNode* n = a_rootNode->GetObjectByName(name);
				if (!n) {
					nodes.push_back(std::make_unique<NullNode>());
				} else {
					nodes.push_back(std::make_unique<GameNode>(n));
				}
			}

			if (l->eyeTrackData) {
				l->eyeTrackData->eyeTarget = a_rootNode->GetObjectByName("Eye_Target");
				l->eyeTrackData->lEye = a_rootNode->GetObjectByName("L_Eye");
				l->eyeTrackData->rEye = a_rootNode->GetObjectByName("R_Eye");
				flags.set(FLAGS::kRequiresEyeTrackUpdate);
			}
			flags.set(FLAGS::kRequiresFaceDataUpdate);
		}
	}

	Transform Graph::GetCurrentTransform(size_t nodeIdx)
	{
		if (nodeIdx < nodes.size()) {
			return nodes[nodeIdx]->GetLocal();
		}
		return Transform();
	}

	void Graph::Update(float a_deltaTime, bool a_visible) {
#ifdef ENABLE_PERFORMANCE_MONITORING
		auto start = Util::Timing::HighResTimeNow();
#endif

		if (!loadedData || !generator) {
			return;
		}

		if (flags.any(FLAGS::kRequiresEyeTrackUpdate)) {
			DisableEyeTracking();
			flags.reset(FLAGS::kRequiresEyeTrackUpdate);
		}

		if (flags.any(FLAGS::kRequiresFaceDataUpdate)) {
			RE::BSFaceGenAnimationData* oldFaceAnimData = loadedData->faceAnimData;
			UpdateFaceAnimData();
			Face::Manager::GetSingleton()->OnAnimDataChange(oldFaceAnimData, loadedData->faceAnimData);
			SetNoBlink(true);

			if (generator->HasFaceAnimation()) {
				SetFaceMorphsControlled(true, loadedData->transition.queuedDuration);
				generator->SetFaceMorphData(loadedData->faceMorphData.get());
			}
			flags.reset(FLAGS::kRequiresFaceDataUpdate);
		}

		generator->AdvanceTime(a_deltaTime);
		if (sequencer) {
			sequencer->Update();
		}

		if (syncInst) {
			bool syncEnabled = true;
			size_t sequencePhase = UINT64_MAX;

			if (sequencer) {
				syncEnabled = sequencer->flags.none(Sequencer::FLAG::kPausedForLoading);
				sequencePhase = std::distance(sequencer->phases.begin(), sequencer->currentPhase);
			}

			auto syncOwner = syncInst->GetOwner();
			if (!syncOwner) [[unlikely]] {
				syncInst = nullptr;
			} else {
				syncInst->NotifyGraphUpdateFinished(this);
				if (syncOwner != this) {
					syncInst->VisitOwner([&](Graph* owner, bool a_ownerUpdated) {
						if (owner->generator != nullptr) {
							generator->Synchronize(owner->generator.get(), a_ownerUpdated ? 0.0f : a_deltaTime * owner->generator->speed);
						}
					});
				}	
			}
		}

		if (a_visible) {
			generator->Generate(loadedData->poseCache);

			if (flags.any(FLAGS::kTransitioning)) {
				auto blendPose = loadedData->blendedPose.get();
				AdvanceTransitionTime(a_deltaTime);
				UpdateTransition(ozz::make_span(blendPose));
				loadedData->lastPose = kBlendedPose;
				PushAnimationOutput(blendPose);
			} else {
				loadedData->lastPose = kGeneratedPose;
				PushAnimationOutput(loadedData->generatedPose.get());
			}
		} else if (flags.any(FLAGS::kTransitioning)) {
			AdvanceTransitionTime(a_deltaTime);
		}

		PushRootOutput(a_visible);

#ifdef ENABLE_PERFORMANCE_MONITORING
		lastUpdateMs = Util::Timing::HighResTimeDiffMilliSec(start);
#endif
	}

	IKTwoBoneData* Graph::AddIKJob(const std::span<std::string_view, 3> a_nodeNames, const RE::NiTransform& a_initialTargetWorld, const RE::NiPoint3& a_initialPolePtModel, float a_transitionTime)
	{
		ikJobs.emplace_back(std::make_unique<IKTwoBoneData>());
		IKTwoBoneData* d = ikJobs.back().get();
		d->targetWorld = a_initialTargetWorld;
		d->polePtModel = a_initialPolePtModel;
		for (size_t i = 0; i < a_nodeNames.size(); i++) {
			d->nodeNames[i] = a_nodeNames[i];
		}
		d->TransitionIn(a_transitionTime);
		return d;
	}

	bool Graph::RemoveIKJob(IKTwoBoneData* a_jobData, float a_transitionTime)
	{
		for (auto& d : ikJobs) {
			if (d.get() == a_jobData) {
				d->TransitionOut(a_transitionTime, true);
				return true;
			}
		}
		return false;
	}

	void Graph::MakeSyncOwner()
	{
		if (syncInst == nullptr) {
			syncInst = std::make_shared<SyncInstance>();
		}
		syncInst->SetOwner(this);
	}

	void Graph::SyncToGraph(Graph* a_grph)
	{
		syncInst = a_grph->syncInst;
	}

	void Graph::StopSyncing()
	{
		if (syncInst != nullptr && syncInst->GetOwner() == this) {
			syncInst->SetOwner(nullptr);
		}
		syncInst = nullptr;
	}

	void Graph::SetNoBlink(bool a_noBlink)
	{
		if (!loadedData)
			return;

		Face::Manager::GetSingleton()->SetNoBlink(loadedData->faceAnimData, a_noBlink);
	}

	void Graph::SetFaceMorphsControlled(bool a_controlled, float a_transitionTime)
	{
		auto& l = loadedData;
		if (!l)
			return;

		if (a_controlled && l->faceAnimData) {
			if (l->faceMorphData) {
				l->faceMorphData->lock()->BeginTween(a_transitionTime, l->faceAnimData);
			} else {
				l->faceMorphData = std::make_shared<Face::MorphData>();
				Face::Manager::GetSingleton()->AttachMorphData(l->faceAnimData, l->faceMorphData, a_transitionTime);
			}
		} else if (!a_controlled && l->faceMorphData) {
			Face::Manager::GetSingleton()->DetachMorphData(l->faceAnimData, a_transitionTime);
			l->faceMorphData = nullptr;
		}
	}

	void Graph::DisableEyeTracking()
	{
		auto& l = loadedData;
		if (!l)
			return;

		if (!l->eyeTrackData || !l->eyeTrackData->lEye || !l->eyeTrackData->rEye)
			return;

		auto eyeNodes = RE::EyeTracking::GetEyeNodes();
		for (auto& n : eyeNodes.data) {
			if (n.leftEye.get() == l->eyeTrackData->lEye && n.rightEye.get() == l->eyeTrackData->rEye) {
				n.leftEye.reset(l->eyeTrackData->eyeTarget);
				n.rightEye.reset(l->eyeTrackData->eyeTarget);
				break;
			}
		}
	}

	void Graph::EnableEyeTracking()
	{
		auto& l = loadedData;
		if (!l)
			return;

		if (!l->eyeTrackData || !l->eyeTrackData->eyeTarget)
			return;

		auto eyeNodes = RE::EyeTracking::GetEyeNodes();
		for (auto& n : eyeNodes.data) {
			if (n.leftEye.get() == l->eyeTrackData->eyeTarget && n.rightEye.get() == l->eyeTrackData->eyeTarget) {
				n.leftEye.reset(l->eyeTrackData->lEye);
				n.rightEye.reset(l->eyeTrackData->rEye);
				break;
			}
		}
	}

	void Graph::DetachSequencer(bool a_transitionOut)
	{
		if (sequencer) {
			sequencer.reset();
			flags.reset(FLAGS::kLoadingSequencerAnimation);
			if (a_transitionOut) {
				StartTransition(nullptr, 1.0f);
			}
			SFSE::GetTaskInterface()->AddTask([target = target]() {
				GraphManager::GetSingleton()->SendEvent(SequencePhaseChangeEvent{
					.exiting = true,
					.target = target
				});
			});
		}
	}

	void Graph::SetLoaded(bool a_loaded)
	{
		if (a_loaded && !loadedData)
		{
			flags.reset(FLAGS::kUnloaded3D);
			loadedData = std::make_unique<LOADED_DATA>();
			auto& l = loadedData;
			l->blendLayers[0].weight = .0f;
			l->blendLayers[1].weight = .0f;

			if (skeleton->lEyeIdx != UINT64_MAX || skeleton->rEyeIdx != UINT64_MAX) {
				l->eyeTrackData = std::make_unique<EyeTrackingData>();
			}

			l->context.Resize(skeleton->data->num_joints());
			l->poseCache.set_pose_size(skeleton->data->num_soa_joints());
			l->poseCache.reserve(4);
			l->restPose = l->poseCache.acquire_handle();
			l->snapshotPose = l->poseCache.acquire_handle();
			l->generatedPose = l->poseCache.acquire_handle();
			l->blendedPose = l->poseCache.acquire_handle();

			if (unloadedData && !unloadedData->restoreFile.QPath().empty()) {
				loadedData->transition.queuedDuration = 0.2f;
				FileManager::GetSingleton()->RequestAnimation(unloadedData->restoreFile, skeleton->name, weak_from_this());
			}
			unloadedData.reset();
		}
		else if (!a_loaded && loadedData)
		{
			Face::Manager::GetSingleton()->OnAnimDataChange(loadedData->faceAnimData, nullptr);
			flags.set(FLAGS::kUnloaded3D);
			loadedData.reset();
			unloadedData = std::make_unique<UNLOADED_DATA>();
			auto& u = unloadedData;

			if (generator) {
				u->restoreFile = FileID(generator->GetSourceFile(), "");
				generator.reset();
				flags.reset(FLAGS::kHasGenerator);
			}

#ifdef ENABLE_PERFORMANCE_MONITORING
			lastUpdateMs = 0.0f;
#endif
		}
	}

	void Graph::UpdateFaceAnimData()
	{
		auto l = loadedData.get();
		if (!l) {
			return;
		}

		auto m = l->rootNode->bgsModelNode;
		if (!m) {
			l->faceAnimData = nullptr;
			return;
		}

		if (m->facegenNodes.size < 1) {
			l->faceAnimData = nullptr;
			return;
		}

		auto fn = m->facegenNodes.data[0];
		if (!fn) {
			l->faceAnimData = nullptr;
			return;
		}

		l->faceAnimData = fn->faceGenAnimData;
	}

	void Graph::AdvanceTransitionTime(float a_deltaTime)
	{
		auto& transition = loadedData->transition;

		transition.localTime += a_deltaTime;
		if (transition.localTime >= transition.duration) {
			if (transition.onEnd != nullptr) {
				transition.onEnd();
			}

			flags.reset(FLAGS::kTransitioning);

			if (transition.duration < 0.01f) {
				transition.duration = 0.01f;
			}

			transition.localTime = transition.duration;
		}
	}

	void Graph::UpdateTransition(const ozz::span<ozz::math::SoaTransform>& a_output)
	{
		auto& transition = loadedData->transition;
		auto& blendLayers = loadedData->blendLayers;
		blendLayers[0].transform = loadedData->generatedPose.get_ozz();
		blendLayers[1].transform = loadedData->snapshotPose.get_ozz();

		ozz::animation::BlendingJob blendJob;
		UpdateRestPose();
		blendJob.rest_pose = loadedData->restPose.get_ozz();
		blendJob.layers = ozz::make_span(blendLayers);
		blendJob.output = a_output;
		blendJob.threshold = 1.0f;

		float normalizedTime = transition.ease(transition.localTime / transition.duration);
		if (transition.startLayer >= 0) {
			blendLayers[transition.startLayer].weight = 1.0f - normalizedTime;
		}
		if (transition.endLayer >= 0) {
			blendLayers[transition.endLayer].weight = normalizedTime;
		}

		blendJob.Run();
	}

	void Graph::StartTransition(std::unique_ptr<Generator> a_dest, float a_transitionTime)
	{
		if (!loadedData)
			return;

		auto& transition = loadedData->transition;
		auto& blendLayers = loadedData->blendLayers;

		const auto SetData = [&](TRANSITION_TYPE t) {
			switch (t) {
			case kGameToGraph:
				transition.startLayer = -1;
				transition.endLayer = 0;
				transition.onEnd = nullptr;
				break;
			case kGraphToGame:
				transition.startLayer = 0;
				transition.endLayer = -1;
				transition.onEnd = [&]() {
					flags.reset(FLAGS::kHasGenerator);
					generator.reset();
				};
				break;
			case kGeneratorToGenerator:
				blendLayers[0].weight = 0.0f;
				blendLayers[1].weight = 1.0f;
				transition.startLayer = 1;
				transition.endLayer = 0;
				transition.onEnd = nullptr;
				break;
			case kGraphSnapshotToGame:
				blendLayers[0].weight = 0.0f;
				blendLayers[1].weight = 1.0f;
				transition.startLayer = 1;
				transition.endLayer = -1;
				transition.onEnd = [&]() {
					flags.reset(FLAGS::kHasGenerator);
					generator.reset();
				};
				break;
			}
		};

		transition.localTime = 0.0f;
		transition.duration = a_transitionTime;

		if (flags.all(FLAGS::kTransitioning)) {
			SnapshotPose();
			if (a_dest != nullptr) {
				SetData(TRANSITION_TYPE::kGeneratorToGenerator);
			} else {
				SetData(TRANSITION_TYPE::kGraphSnapshotToGame);
			}
		} else if (flags.all(FLAGS::kHasGenerator)) {
			if (a_dest != nullptr) {
				SnapshotPose();
				SetData(TRANSITION_TYPE::kGeneratorToGenerator);
			} else {
				SetData(TRANSITION_TYPE::kGraphToGame);
			}
		} else {
			if (a_dest != nullptr) {
				ResetRootTransform();
				SetData(TRANSITION_TYPE::kGameToGraph);
			} else {
				return;
			}
		}

		if (generator != nullptr) {
			generator->OnDetaching();
		}

		flags.set(FLAGS::kTransitioning);
		if (a_dest != nullptr) {
			generator = std::move(a_dest);
			generator->SetContext(&loadedData->context);
			generator->SetOutput(&loadedData->generatedPose);

			if (generator->HasFaceAnimation()) {
				loadedData->transition.queuedDuration = a_transitionTime;
				SetFaceMorphsControlled(true, a_transitionTime);
				generator->SetFaceMorphData(loadedData->faceMorphData.get());
			} else {
				SetFaceMorphsControlled(false, a_transitionTime);
			}

			flags.set(FLAGS::kHasGenerator);
		} else {
			SetFaceMorphsControlled(false, a_transitionTime);
		}
	}

	void Graph::PushAnimationOutput(const std::span<ozz::math::SoaTransform>& a_output)
	{
		if (generator && generator->rootResetRequired) {
			ResetRootOrientation();
			generator->localRootTransform.MakeIdentity();
			generator->rootResetRequired = false;
		}

		/*
		if (updateCount > 0) {
			const auto& rootRelative = a_output[0];
			rootTransform.rotate = rootRelative.rotate.InvertVector() * rootTransform.rotate;
			rootTransform.translate += rootOrientation * rootRelative.translate;
		}
		*/

		Transform::ExtractSoaTransformsReal(a_output, [&](size_t i, const RE::NiMatrix3& rot, const RE::NiPoint3& pos) {
			if (i > 0 && i < nodes.size()) {
				nodes[i]->SetLocalReal(rot, pos);
			}
		});

		auto r = loadedData->rootNode;
		if (!r)
			return;
		auto m = r->bgsModelNode;
		if (!m)
			return;
		auto u = m->unk10;
		if (!u)
			return;
		u->needsUpdate = true;
	}

	void Graph::PushRootOutput(bool a_visible)
	{
		static RE::TransformsManager* transformManager = RE::TransformsManager::GetSingleton();
		auto rootXYZ = GetRootXYZ();
		target->data.angle = rootXYZ.rotate;
		if (a_visible) {
			transformManager->RequestPositionUpdate(target.get(), rootXYZ.translate);
		} else {
			//This doesn't keep the actor perfectly in place, but it's good enough for when the camera is looking away.
			SFSE::GetTaskInterface()->AddTask([rootXYZ = rootXYZ, target = target] {
				auto actor = target->As<RE::Actor>();
				if (!actor)
					return;

				auto process = actor->currentProcess;
				if (!process)
					return;

				auto middleHigh = process->middleHigh;
				if (!middleHigh)
					return;

				auto charProxy = middleHigh->charProxy;
				if (!charProxy)
					return;

				charProxy->SetPosition(RE::NiPoint3A(rootXYZ.translate));
			});
		}
	}

	void Graph::UpdateRestPose()
	{
		Transform::StoreSoaTransforms(loadedData->restPose.get(), std::bind(&Graph::GetCurrentTransform, this, std::placeholders::_1));
	}

	void Graph::SnapshotPose()
	{
		if (!loadedData)
			return;

		std::span<ozz::math::SoaTransform> sourcePose;
		switch (loadedData->lastPose) {
		case kNoPose:
			UpdateRestPose();
			sourcePose = loadedData->restPose.get();
			break;
		case kGeneratedPose:
			sourcePose = loadedData->generatedPose.get();
			break;
		case kBlendedPose:
			sourcePose = loadedData->blendedPose.get();
			break;
		}

		std::span<ozz::math::SoaTransform> snapshotPose = loadedData->snapshotPose.get();
		std::copy(sourcePose.begin(), sourcePose.end(), snapshotPose.begin());
	}

	void Graph::ResetRootTransform()
	{
		rootTransform.rotate.FromEulerAnglesZXY(target->data.angle);
		rootTransform.translate = target->data.location;
		ResetRootOrientation();
	}

	void Graph::ResetRootOrientation()
	{
		rootOrientation = rootTransform.rotate.InvertVector();
	}

	XYZTransform Graph::GetRootXYZ()
	{
		XYZTransform result;
		result.rotate = rootTransform.rotate.ToEulerAnglesZXY();
		result.translate = rootTransform.translate;
		return result;
	}
}