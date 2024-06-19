#include "GraphManager.h"
#include "Settings/Settings.h"
#include "Serialization/GLTFImport.h"
#include "Util/String.h"
#include "Graph.h"
#include "FileManager.h"
#include "Util/Trampoline.h"

namespace Animation
{
	typedef void (*GraphUpdateFunc)(RE::IAnimationGraphManagerHolder*, RE::BSAnimationUpdateData*, void*);

	GraphManager* GraphManager::GetSingleton()
	{
		static GraphManager singleton;
		return &singleton;
	}

	std::unique_ptr<Generator> GraphManager::CreateAnimationGenerator(std::shared_ptr<OzzAnimation> anim)
	{
		auto gen = std::make_unique<LinearClipGenerator>();
		gen->anim = anim;
		gen->duration = anim->data->duration();
		return gen;
	}

	bool GraphManager::LoadAndStartAnimation(RE::Actor* a_actor, const std::string_view a_filePath, const std::string_view a_animId, float a_transitionTime)
	{
		if (!a_actor)
			return false;
		
		auto g = GetGraph(a_actor, true);
		std::unique_lock l{ g->lock };
		g->transition.queuedDuration = a_transitionTime;
		g->flags.set(Graph::FLAGS::kLoadingAnimation);
		FileManager::GetSingleton()->RequestAnimation(FileID(a_filePath, a_animId), a_actor->race->formEditorID.c_str(), g);
		return true;
	}

	void GraphManager::SyncGraphs(const std::vector<RE::Actor*>& a_actors)
	{
		if (a_actors.size() < 2)
			return;

		std::unique_lock sl{ stateLock };
		auto owner = GetGraphLockless(*a_actors.begin(), true);
		std::unique_lock ol{ owner->lock };
		owner->MakeSyncOwner();
		owner->ResetRootTransform();
		owner->syncInst->data.lock()->rootTransform = owner->rootTransform;
		ol.unlock();
		for (size_t i = 1; i < a_actors.size(); i++) {
			auto g = GetGraphLockless(a_actors[i], true);
			std::unique_lock l{ g->lock };
			g->SyncToGraph(owner.get());
		}
	}

	void GraphManager::StopSyncing(RE::Actor* a_actor)
	{
		if (!a_actor)
			return;
		
		auto g = GetGraph(a_actor, false);
		if (!g)
			return;

		std::unique_lock l{ g->lock };
		g->StopSyncing();
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
		g->flags.reset(Graph::FLAGS::kLoadingAnimation);
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

		return GetGraphLockless(a_actor, create);
	}

	std::shared_ptr<Graph> CreateGraph(RE::Actor* a_actor)
	{
		std::shared_ptr<Graph> g = std::make_shared<Graph>();
		g->target.reset(a_actor);
		g->SetSkeleton(Settings::GetSkeleton(a_actor));
		g->requesterHandle = g;
		return g;
	}

	std::shared_ptr<Graph> GraphManager::GetGraphLockless(RE::Actor* a_actor, bool create)
	{
		if (auto iter = state->graphMap.find(a_actor); iter != state->graphMap.end())
			return iter->second;

		if (!create)
			return nullptr;

		std::shared_ptr<Graph> g = CreateGraph(a_actor);
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

	void GraphManager::VisitGraph(RE::Actor* a_actor, const std::function<void(Graph*)> visitFunc)
	{
		std::shared_lock ls{ stateLock };
		
		if (auto iter = state->graphMap.find(a_actor); iter != state->graphMap.end())
		{
			std::unique_lock gl{ iter->second->lock };
			visitFunc(iter->second.get());
		} else {
			ls.unlock();
			std::unique_lock lu{ stateLock };
			std::shared_ptr<Graph> g = CreateGraph(a_actor);
			state->graphMap[a_actor] = g;

			std::unique_lock gl{ g->lock };
			g->Update(0.0f);
			visitFunc(g.get());
		}
	}

	GraphManager& graphManager = *GraphManager::GetSingleton();
	GraphUpdateFunc OriginalGraphUpdate;

	void UpdateGraph(RE::IAnimationGraphManagerHolder* a_graphHolder, RE::BSAnimationUpdateData* a_updateData, void* a_graph)
	{
		OriginalGraphUpdate(a_graphHolder, a_updateData, a_graph);

		std::shared_lock l{ graphManager.stateLock };
		auto& m = graphManager.state->graphMap;
		if (auto iter = m.find(a_graphHolder); iter != m.end()) {
			auto& g = iter->second;
			std::unique_lock gl{ g->lock };
			g->Update(a_updateData->timeDelta);

			if (g->flags.none(Graph::FLAGS::kHasGenerator, Graph::FLAGS::kTransitioning, Graph::FLAGS::kLoadingAnimation) &&
				g->flags.all(Graph::FLAGS::kTemporary, Graph::FLAGS::kNoActiveIKChains)) {
				l.unlock();
				graphManager.DetachGraph(a_graphHolder);
			}
		}
	}

	void GraphManager::InstallHooks()
	{
		Util::Trampoline::AddHook(14, [](SFSE::Trampoline& t) {
			//IAnimationGraphManagerHolder::UpdateAnimationGraphManager(IAnimationGraphManagerHolder*, BSAnimationUpdateData*, Graph*)
			REL::Relocation<uintptr_t> hookLoc{ REL::ID(118488), 0x61 };
			OriginalGraphUpdate = reinterpret_cast<GraphUpdateFunc>(SFSE::GetTrampoline().write_call<5>(hookLoc.address(), &UpdateGraph));
			INFO("Installed graph update hook.");
		});
	}
}