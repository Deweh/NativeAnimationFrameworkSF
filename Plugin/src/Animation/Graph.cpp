#include "Graph.h"
#include "Util/Math.h"
#include "Tasks/MainLoop.h"
#include "Node.h"

namespace Animation
{
	Graph::Graph()
	{
		flags.set(FLAGS::kTemporary, FLAGS::kNoActiveIKChains, FLAGS::kUnloaded3D);
	}

	void Graph::SetSkeleton(std::shared_ptr<const Settings::SkeletonDescriptor> a_descriptor)
	{
		skeleton = a_descriptor;
		transitionOutput.resize(skeleton->nodeNames.size());
		transitionSnapshot.resize(skeleton->nodeNames.size());
	}

	void Graph::GetSkeletonNodes(RE::BGSFadeNode* a_rootNode) {
		nodes.clear();
		rootNode = a_rootNode;

		if (a_rootNode != nullptr) {
			for (auto& name : skeleton->nodeNames) {
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
			switch (state) {
			case STATE::kGenerator:
				generator->Generate(a_deltaTime);
				PushOutput(generator->output);
				break;
			case STATE::kTransition:
				switch (transitionType) {
				case kGameToGraph:
					UpdateTransition(a_deltaTime, STATE::kGenerator, [&](size_t i) {
						return std::make_pair(GetCurrentTransform(i), generator->output[i]);
					});
					break;
				case kGraphToGame:
					UpdateTransition(a_deltaTime, STATE::kIdle, [&](size_t i) {
						return std::make_pair(generator->output[i], GetCurrentTransform(i));
					});
					break;
				case kGeneratorToGenerator:
					UpdateTransition(a_deltaTime, STATE::kGenerator, [&](size_t i) {
						return std::make_pair(transitionSnapshot[i], generator->output[i]);
					});
					break;
				case kGraphSnapshotToGame:
					UpdateTransition(a_deltaTime, STATE::kIdle, [&](size_t i) {
						return std::make_pair(transitionSnapshot[i], GetCurrentTransform(i));
					});
					break;
				}
				break;
			case STATE::kIdle:
				break;
			}
		}
	}

	void Graph::StartTransition(std::unique_ptr<Generator> a_dest, float a_transitionTime)
	{
		transitionLocalTime = 0.0f;
		transitionDuration = a_transitionTime;

		switch (state) {
		case kIdle:
			if (a_dest != nullptr) {
				transitionType = TRANSITION_TYPE::kGameToGraph;
			} else {
				return;
			}
			break;
		case kGenerator:
			if (a_dest != nullptr) {
				SnapshotTransformsForTransition();
				transitionType = TRANSITION_TYPE::kGeneratorToGenerator;
			} else {
				transitionType = TRANSITION_TYPE::kGraphToGame;
			}
			break;
		case kTransition:
			SnapshotTransformsForTransition();
			if (a_dest != nullptr) {
				transitionType = TRANSITION_TYPE::kGeneratorToGenerator;
			} else {
				transitionType = TRANSITION_TYPE::kGraphSnapshotToGame;
			}
			break;
		}

		state = kTransition;
		if (a_dest != nullptr) {
			generator = std::move(a_dest);
		}
	}

	void Graph::UpdateTransition(float a_deltaTime, STATE a_endState, const std::function<std::pair<Transform, Transform>(size_t)>& a_transformsFunc)
	{
		generator->Generate(a_deltaTime);
		transitionLocalTime += a_deltaTime;

		if (transitionLocalTime >= transitionDuration) {
			state = a_endState;
			transitionType = TRANSITION_TYPE::kNoTransition;

			if (transitionDuration < 0.01f) {
				transitionDuration = 0.01f;
			}

			transitionLocalTime = transitionDuration;
		}

		auto normalizedTime = transitionEase(Util::NormalizeSpan(0.0f, transitionDuration, transitionLocalTime));
		size_t count = nodes.size() > generator->output.size() ? generator->output.size() : nodes.size();
		for (size_t i = 0; i < count; i++) {
			auto trans = a_transformsFunc(i);
			if (trans.first.IsIdentity()) {
				transitionOutput[i] = trans.second;
			} else if (trans.second.IsIdentity()) {
				transitionOutput[i] = trans.first;
			} else {
				auto& out = transitionOutput[i];
				transitionPosInterp(trans.first.translate, trans.second.translate, normalizedTime, out.translate);
				transitionRotInterp(trans.first.rotate, trans.second.rotate, normalizedTime, out.rotate);
			}
		}

		if (state == kIdle) {
			generator.reset();
		}

		PushOutput(transitionOutput);
	}

	void Graph::PushOutput(const std::vector<Transform>& a_output)
	{
		size_t updateCount = nodes.size() > a_output.size() ? a_output.size() : nodes.size();

		for (size_t i = 0; i < updateCount; i++) {
			const auto& cur = a_output[i];
			if (!cur.IsIdentity()) {
				nodes[i]->SetLocal(cur);
			}
		}

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

	void Graph::SnapshotTransformsForTransition() {
		for (size_t i = 0; i < nodes.size(); i++) {
			transitionSnapshot[i] = GetCurrentTransform(i);
		}
	}
}