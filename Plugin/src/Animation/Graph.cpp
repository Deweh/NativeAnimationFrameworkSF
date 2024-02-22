#include "Graph.h"
#include "Util/Math.h"
#include "Tasks/MainLoop.h"
#include "Node.h"

namespace Animation
{
	Graph::Graph()
	{
		flags.set(FLAGS::kTemporary, FLAGS::kNoActiveIKChains, FLAGS::kUnloaded3D);
		blendLayers[0].weight = .0f;
		blendLayers[1].weight = .0f;
	}

	void Graph::SetSkeleton(std::shared_ptr<const OzzSkeleton> a_descriptor)
	{
		skeleton = a_descriptor;
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
			generator->output = generatedPose;
		}
	}

	void Graph::GetSkeletonNodes(RE::BGSFadeNode* a_rootNode) {
		nodes.clear();
		rootNode = a_rootNode;

		if (a_rootNode != nullptr) {
			ResetRootTransform();
			for (auto& name : skeleton->data->joint_names()) {
				RE::NiAVObject* n = a_rootNode->GetObjectByName(name);
				if (!n) {
					nodes.push_back(std::make_unique<NullNode>());
				} else {
					nodes.push_back(std::make_unique<GameNode>(n));
				}
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
		if (loadedRefData == nullptr || loadedRefData->data3D.get() == nullptr) {
			if (rootNode != nullptr) {
				GetSkeletonNodes(nullptr);
			}
		} else if (loadedRefData->data3D.get() != rootNode) {
			GetSkeletonNodes(static_cast<RE::BGSFadeNode*>(loadedRefData->data3D.get()));
		}
		
		if (rootNode != nullptr && generator != nullptr) {
			generator->Generate(a_deltaTime);

			if (flags.all(FLAGS::kTransitioning)) {
				UpdateTransition(a_deltaTime);
				PushOutput(blendedPose);
			} else {
				PushOutput(generatedPose);
			}
		}
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
				SnapshotCurrentPose();
				blendLayers[0].weight = 0.0f;
				blendLayers[1].weight = 1.0f;
				transition.startLayer = 1;
				transition.endLayer = 0;
				transition.onEnd = nullptr;
				break;
			case kGraphSnapshotToGame:
				SnapshotCurrentPose();
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
			if (a_dest != nullptr) {
				SetData(TRANSITION_TYPE::kGeneratorToGenerator);
			} else {
				SetData(TRANSITION_TYPE::kGraphSnapshotToGame);
			}
		} else if (flags.all(FLAGS::kHasGenerator)) {
			if (a_dest != nullptr) {
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

		flags.set(FLAGS::kTransitioning);
		if (a_dest != nullptr) {
			generator = std::move(a_dest);
			generator->context = &context;
			generator->output = generatedPose;
			flags.set(FLAGS::kHasGenerator);
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

	void Graph::SnapshotCurrentPose()
	{
		Transform::StoreSoaTransforms(snapshotPose, std::bind(&Graph::GetCurrentTransform, this, std::placeholders::_1));
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