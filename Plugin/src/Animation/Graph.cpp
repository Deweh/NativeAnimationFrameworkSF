#include "Graph.h"
#include "Util/Math.h"
#include "Tasks/MainLoop.h"
#include "Node.h"
#include "Face/Manager.h"
#include "Util/Timing.h"
#include "Sequencer.h"

namespace Animation
{
	Graph::Graph()
	{
		flags.set(FLAGS::kUnloaded3D);
		blendLayers[0].weight = .0f;
		blendLayers[1].weight = .0f;
	}

	Graph::~Graph() noexcept
	{
		if (syncInst != nullptr && syncInst->GetOwner() == this) {
			syncInst->SetOwner(nullptr);
		}
		SetNoBlink(false);
		SetFaceMorphsControlled(false, 1.0f);
		EnableEyeTracking();
	}

	void Graph::OnAnimationReady(const FileID& a_id, std::shared_ptr<OzzAnimation> a_anim)
	{
		std::unique_lock l{ lock };
		if (sequencer && sequencer->OnAnimationReady(a_id, a_anim)) {
			return;
		}

		if (flags.all(FLAGS::kLoadingAnimation)) {
			if (a_id != activeFile) {
				return;
			}

			flags.reset(FLAGS::kLoadingAnimation);
			if (a_anim != nullptr) {
				StartTransition(std::make_unique<LinearClipGenerator>(a_anim), transition.queuedDuration);
			}
		}
	}

	void Graph::OnAnimationRequested(const FileID& a_id)
	{
		std::unique_lock l{ lock };
		if (sequencer && sequencer->OnAnimationRequested(a_id)) {
			return;
		}

		if (flags.all(FLAGS::kLoadingAnimation)) {
			FileManager::GetSingleton()->CancelAnimationRequest(activeFile, weak_from_this());
		} else {
			flags.set(FLAGS::kLoadingAnimation);
		}
		
		activeFile = a_id;
	}

	void Graph::SetSkeleton(std::shared_ptr<const OzzSkeleton> a_descriptor)
	{
		skeleton = a_descriptor;

		if (skeleton->lEyeIdx != UINT64_MAX || skeleton->rEyeIdx != UINT64_MAX) {
			eyeTrackData = std::make_unique<EyeTrackingData>();
		}

		int soaSize = skeleton->data->num_soa_joints();
		int jointSize = skeleton->data->num_joints();
		restPose.resize(soaSize);
		snapshotPose.resize(soaSize);
		generatedPose.resize(soaSize);
		blendedPose.resize(soaSize);
		context.Resize(jointSize);
		blendLayers[0].transform = ozz::make_span(generatedPose);
		blendLayers[1].transform = ozz::make_span(snapshotPose);
		if (generator != nullptr) {
			generator->SetOutput(ozz::make_span(generatedPose));
		}
	}

	void Graph::GetSkeletonNodes(RE::BGSFadeNode* a_rootNode) {
		nodes.clear();
		rootNode = a_rootNode;
		EnableEyeTracking();

		if (eyeTrackData) {
			eyeTrackData->lEye = nullptr;
			eyeTrackData->rEye = nullptr;
			eyeTrackData->eyeTarget = nullptr;
		}

		if (a_rootNode != nullptr) {
			RE::BSFaceGenAnimationData* oldFaceAnimData = faceAnimData;
			UpdateFaceAnimData();
			Face::Manager::GetSingleton()->OnAnimDataChange(oldFaceAnimData, faceAnimData);

			SetNoBlink(true);
			ResetRootTransform();
			size_t idx = 0;
			for (auto& name : skeleton->data->joint_names()) {
				RE::NiNode* n = a_rootNode->GetObjectByName(name);
				if (!n) {
					nodes.push_back(std::make_unique<NullNode>());
				} else {
					if (idx == skeleton->lEyeIdx) {
						eyeTrackData->lEye = n;
					} else if (idx == skeleton->rEyeIdx) {
						eyeTrackData->rEye = n;
					}
					nodes.push_back(std::make_unique<GameNode>(n));
				}
				idx++;
			}

			if (eyeTrackData) {
				eyeTrackData->eyeTarget = a_rootNode->GetObjectByName("Eye_Target");
				flags.set(FLAGS::kRequiresEyeTrackUpdate);
			}
			
			flags.reset(FLAGS::kUnloaded3D);
		} else {
			flags.set(FLAGS::kUnloaded3D);
		}
	}

	Transform Graph::GetCurrentTransform(size_t nodeIdx)
	{
		if (nodeIdx < nodes.size()) {
			return nodes[nodeIdx]->GetLocal();
		}
		return Transform();
	}

	void Graph::Update(float a_deltaTime) {
		auto dataLock = target->loadedData.lock_write();
		auto& loadedRefData = *dataLock;

		if (!loadedRefData || !loadedRefData->data3D.get()) {
			if (rootNode) {
				GetSkeletonNodes(nullptr);
			}
		} else if (loadedRefData->data3D.get() != rootNode) {
			GetSkeletonNodes(static_cast<RE::BGSFadeNode*>(loadedRefData->data3D.get()));
		} else if (flags.any(FLAGS::kRequiresEyeTrackUpdate)) {
			DisableEyeTracking();
			flags.reset(FLAGS::kRequiresEyeTrackUpdate);
		}

		if (!rootNode || !generator) {
			return;
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

			auto sOwner = syncInst->GetOwner();
			if (sOwner == this) {
				syncInst->NotifyOwnerUpdate(generator->localTime, rootTransform, syncEnabled, sequencePhase);
			} else if (!sOwner) {
				syncInst = nullptr;
			} else if (syncEnabled) {
				auto data = syncInst->NotifyGraphUpdate(this);
				if (sequencer && sequencePhase != data.sequencePhase) {
					sequencer->SetPhase(data.sequencePhase);
				}
				if (data.syncEnabled) {
					generator->localTime = data.time + (data.hasOwnerUpdated ? 0.0f : a_deltaTime);
					rootTransform = data.rootTransform;
				}
			}
		}

		generator->Generate(0.0f);

		if (flags.any(FLAGS::kTransitioning)) {
			UpdateTransition(a_deltaTime);
			PushOutput(blendedPose);
		} else {
			PushOutput(generatedPose);
		}
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
		Face::Manager::GetSingleton()->SetNoBlink(faceAnimData, a_noBlink);
	}

	void Graph::SetFaceMorphsControlled(bool a_controlled, float a_transitionTime)
	{
		if (a_controlled && faceAnimData) {
			if (faceMorphData) {
				faceMorphData->lock()->BeginTween(a_transitionTime, faceAnimData);
			} else {
				faceMorphData = std::make_shared<Face::MorphData>();
				Face::Manager::GetSingleton()->AttachMorphData(faceAnimData, faceMorphData, a_transitionTime);
			}
		} else if (!a_controlled && faceMorphData) {
			Face::Manager::GetSingleton()->DetachMorphData(faceAnimData, a_transitionTime);
			faceMorphData = nullptr;
		}
	}

	void Graph::DisableEyeTracking()
	{
		if (!eyeTrackData || !eyeTrackData->lEye || !eyeTrackData->rEye)
			return;

		auto eyeNodes = RE::EyeTracking::GetEyeNodes();
		for (auto& n : eyeNodes.data) {
			if (n.leftEye.get() == eyeTrackData->lEye && n.rightEye.get() == eyeTrackData->rEye) {
				n.leftEye.reset(eyeTrackData->eyeTarget);
				n.rightEye.reset(eyeTrackData->eyeTarget);
				break;
			}
		}
	}

	void Graph::EnableEyeTracking()
	{
		if (!eyeTrackData || !eyeTrackData->eyeTarget)
			return;

		auto eyeNodes = RE::EyeTracking::GetEyeNodes();
		for (auto& n : eyeNodes.data) {
			if (n.leftEye.get() == eyeTrackData->eyeTarget && n.rightEye.get() == eyeTrackData->eyeTarget) {
				n.leftEye.reset(eyeTrackData->lEye);
				n.rightEye.reset(eyeTrackData->rEye);
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
		}
	}

	void Graph::UpdateFaceAnimData()
	{
		if (!rootNode) {
			faceAnimData = nullptr;
			return;
		}

		auto m = rootNode->bgsModelNode;
		if (!m) {
			faceAnimData = nullptr;
			return;
		}

		if (m->facegenNodes.size < 1) {
			faceAnimData = nullptr;
			return;
		}

		auto fn = m->facegenNodes.data[0];
		if (!fn) {
			faceAnimData = nullptr;
			return;
		}

		faceAnimData = fn->faceGenAnimData;
	}

	void Graph::UpdateTransition(float a_deltaTime)
	{
		ozz::animation::BlendingJob blendJob;
		UpdateRestPose();
		blendJob.rest_pose = ozz::make_span(restPose);
		blendJob.layers = ozz::make_span(blendLayers);
		blendJob.output = ozz::make_span(blendedPose);
		blendJob.threshold = 1.0f;

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
			SnapshotBlend();
			if (a_dest != nullptr) {
				SetData(TRANSITION_TYPE::kGeneratorToGenerator);
			} else {
				SetData(TRANSITION_TYPE::kGraphSnapshotToGame);
			}
		} else if (flags.all(FLAGS::kHasGenerator)) {
			if (a_dest != nullptr) {
				SnapshotGenerator();
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
			generator->SetContext(&context);
			generator->SetOutput(ozz::make_span(generatedPose));

			if (generator->HasFaceAnimation()) {
				SetFaceMorphsControlled(true, a_transitionTime);
				generator->SetFaceMorphData(faceMorphData.get());
			} else {
				SetFaceMorphsControlled(false, a_transitionTime);
			}

			flags.set(FLAGS::kHasGenerator);
		} else {
			SetFaceMorphsControlled(false, a_transitionTime);
		}
	}

	void Graph::PushOutput(const std::vector<ozz::math::SoaTransform>& a_output)
	{
		static RE::TransformsManager* transformManager = RE::TransformsManager::GetSingleton();

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

		auto rootXYZ = GetRootXYZ();
		transformManager->RequestPosRotUpdate(target.get(), rootXYZ.translate, rootXYZ.rotate);

		auto r = rootNode;
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

	void Graph::UpdateRestPose()
	{
		Transform::StoreSoaTransforms(restPose, std::bind(&Graph::GetCurrentTransform, this, std::placeholders::_1));
	}

	void Graph::SnapshotBlend()
	{
		for (size_t i = 0; i < snapshotPose.size(); i++) {
			snapshotPose[i] = blendedPose[i];
		}
	}

	void Graph::SnapshotGenerator()
	{
		for (size_t i = 0; i < snapshotPose.size(); i++) {
			snapshotPose[i] = generatedPose[i];
		}
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