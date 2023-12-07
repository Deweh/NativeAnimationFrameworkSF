#include "GraphManager.h"
#include "Settings/Settings.h"
#include "Serialization/GLTFImport.h"
#include "Util/String.h"

namespace Animation
{
	typedef void (*GraphUpdateFunc)(RE::IAnimationGraphManagerHolder*, RE::BSAnimationUpdateData*, void*);

	GraphManager* GraphManager::GetSingleton()
	{
		static GraphManager singleton;
		return &singleton;
	}

	GLTFErrorCode GraphManager::PlayAnimationFromGLTF(RE::Actor* a_actor, float a_transitionTime, const std::string& a_fileName, const AnimationIdentifer& a_id)
	{
		if (!a_actor) {
			return kNoSkeleton;
		}

		auto skeleton = Settings::GetSkeleton(a_actor);
		if (Settings::IsDefaultSkeleton(skeleton)) {
			return kNoSkeleton;
		}

		auto asset = Serialization::GLTFImport::LoadGLTF(Util::String::GetDataPath() / a_fileName);
		if (!asset) {
			return kFailedToLoad;
		}

		fastgltf::Animation* anim = nullptr;
		if (a_id.type == AnimationIdentifer::Type::kIndex && a_id.index < asset->animations.size()) {
			anim = &asset->animations[a_id.index];
		} else if (a_id.type == AnimationIdentifer::Type::kName) {
			for (auto& a : asset->animations) {
				if (a.name == a_id.name) {
					anim = &a;
					break;
				}
			}
		}

		if (!anim) {
			return kInvalidAnimationIdentifier;
		}
		
		auto gen = Serialization::GLTFImport::CreateClipGenerator(asset.get(), anim, skeleton.get());
		if (!gen) {
			return kFailedToMakeClip;
		}

		gen->InitTimelines();
		AttachGenerator(a_actor, std::move(gen), a_transitionTime);
		return kSuccess;
	}

	bool GraphManager::AttachGenerator(RE::Actor* a_actor, std::unique_ptr<Generator> a_gen, float a_transitionTime)
	{
		if (!a_actor)
			return false;

		auto g = GetGraph(a_actor, true);
		std::unique_lock l{ g->lock };

		g->StartTransition(std::move(a_gen), a_transitionTime);
		return true;
	}

	bool GraphManager::DetachGenerator(RE::Actor* a_actor, float a_transitionTime)
	{
		if (!a_actor)
			return false;

		auto g = GetGraph(a_actor, false);
		if (!g)
			return false;

		std::unique_lock l{ g->lock };
		g->StartTransition(nullptr, a_transitionTime);
		return true;
	}

	void GraphManager::Reset()
	{
		std::unique_lock l{ stateLock };
		state = std::make_unique<PersistentState>();
	}

	std::shared_ptr<Graph> GraphManager::GetGraph(RE::Actor* a_actor, bool create)
	{
		std::shared_lock ls{ stateLock, std::defer_lock };
		std::unique_lock lu{ stateLock, std::defer_lock };
		if (create) {
			lu.lock();
		} else {
			ls.lock();
		}

		if (auto iter = state->graphMap.find(a_actor); iter != state->graphMap.end())
			return iter->second;

		if (!create)
			return nullptr;

		std::shared_ptr<Graph> g = std::make_shared<Graph>();
		g->target.reset(a_actor);
		g->SetSkeleton(Settings::GetSkeleton(a_actor));

		state->graphMap[a_actor] = g;
		return g;
	}

	bool GraphManager::DetachGraph(RE::IAnimationGraphManagerHolder* a_graphHolder)
	{
		std::unique_lock l{ stateLock };
		if (auto iter = state->graphMap.find(a_graphHolder); iter != state->graphMap.end()) {
			state->graphMap.erase(iter);
			return true;
		}
		return false;
	}

	GraphManager& graphManager = *GraphManager::GetSingleton();
	GraphUpdateFunc OriginalGraphUpdate;

	void UpdateGraph(RE::IAnimationGraphManagerHolder* a_graphHolder, RE::BSAnimationUpdateData* a_updateData, void* a_graph)
	{
		std::shared_lock l{ graphManager.stateLock };
		auto& m = graphManager.state->graphMap;
		if (auto iter = m.find(a_graphHolder); iter != m.end()) {
			auto& g = iter->second;
			std::unique_lock gl{ g->lock };

			if (g->state == Graph::STATE::kTransition || g->state == Graph::STATE::kIdle) {
				OriginalGraphUpdate(a_graphHolder, a_updateData, a_graph);
			}

			g->Update(a_updateData->timeDelta);

			if (g->state == Graph::STATE::kIdle && g->flags.all(Graph::FLAGS::kTemporary, Graph::FLAGS::kNoActiveIKChains)) {
				l.unlock();
				graphManager.DetachGraph(a_graphHolder);
			}
		} else {
			OriginalGraphUpdate(a_graphHolder, a_updateData, a_graph);
		}
	}

	void GraphManager::InstallHooks()
	{
		//IAnimationGraphManagerHolder::UpdateAnimationGraphManager(IAnimationGraphManagerHolder*, BSAnimationUpdateData*, Graph*)
		REL::Relocation<uintptr_t> hookLoc{ REL::ID(118488), 0x61 };
		OriginalGraphUpdate = reinterpret_cast<GraphUpdateFunc>(SFSE::GetTrampoline().write_call<5>(hookLoc.address(), &UpdateGraph));
		INFO("Installed graph update hook.");
	}
}