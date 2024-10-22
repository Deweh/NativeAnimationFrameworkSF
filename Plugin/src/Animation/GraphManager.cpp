#include "GraphManager.h"
#include "Settings/Settings.h"
#include "Serialization/GLTFImport.h"
#include "Util/String.h"
#include "Graph.h"
#include "Generator.h"
#include "FileManager.h"
#include "Util/Trampoline.h"
#include "Util/Timing.h"

namespace Animation
{
	GraphManager* GraphManager::GetSingleton()
	{
		static GraphManager* instance{ new GraphManager() };
		return instance;
	}

	bool GraphManager::LoadAndStartAnimation(RE::Actor* a_actor, const std::string_view a_filePath, const std::string_view a_animId, float a_transitionTime)
	{
		return VisitGraph(a_actor, [&](Graph* g) {
			g->DetachSequencer(false);
			if (g->flags.none(Graph::FLAGS::kUnloaded3D)) {
				g->loadedData->transition.queuedDuration = a_transitionTime;
				FileManager::GetSingleton()->RequestAnimation(FileID(a_filePath, a_animId), a_actor->race->formEditorID.c_str(), g->weak_from_this());
			} else {
				g->unloadedData->restoreFile = FileID(a_filePath, a_animId);
			}
			
			return true;
		}, true);
	}

	void GraphManager::StartSequence(RE::Actor* a_actor, std::vector<Sequencer::PhaseData>&& a_phaseData)
	{
		auto seq = std::make_unique<Sequencer>(std::move(a_phaseData));
		VisitGraph(a_actor, [&](Graph* g) {
			g->sequencer = std::move(seq);
			g->sequencer->OnAttachedToGraph(g);
			return true;
		}, true);
	}

	bool GraphManager::AdvanceSequence(RE::Actor* a_actor, bool a_smooth)
	{
		return VisitGraph(a_actor, [&](Graph* g) {
			if (!g->sequencer)
				return false;

			g->sequencer->flags.set(Sequencer::FLAG::kForceAdvance);
			if (a_smooth)
				g->sequencer->flags.set(Sequencer::FLAG::kSmoothAdvance);
			return true;
		});
	}

	bool GraphManager::SetSequencePhase(RE::Actor* a_actor, size_t a_idx)
	{
		return VisitGraph(a_actor, [&](Graph* g) {
			if (!g->sequencer)
				return false;

			g->sequencer->SetPhase(a_idx);
			return true;
		});
	}

	size_t GraphManager::GetSequencePhase(RE::Actor* a_actor)
	{
		size_t result = UINT64_MAX;
		VisitGraph(a_actor, [&](Graph* g) {
			if (!g->sequencer)
				return false;

			result = g->sequencer->GetPhase();
			return true;
		});
		return result;
	}

	bool GraphManager::SetAnimationSpeed(RE::Actor* a_actor, float a_speed)
	{
		return VisitGraph(a_actor, [&](Graph* g) {
			if (!g->generator)
				return false;

			g->generator->speed = a_speed;
			return true;
		});
	}

	float GraphManager::GetAnimationSpeed(RE::Actor* a_actor)
	{
		float result = 0.0f;
		VisitGraph(a_actor, [&](Graph* g) {
			if (!g->generator)
				return false;

			result = g->generator->speed;
			return true;
		});
		return result;
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
		auto rootTransform = owner->rootTransform;
		ol.unlock();
		for (size_t i = 1; i < a_actors.size(); i++) {
			auto g = GetGraphLockless(a_actors[i], true);
			std::unique_lock l{ g->lock };
			g->SyncToGraph(owner.get());
			g->rootTransform = rootTransform;
		}
	}

	void GraphManager::StopSyncing(RE::Actor* a_actor)
	{
		VisitGraph(a_actor, [&](Graph* g) {
			g->StopSyncing();
			return true;
		});
	}

	bool GraphManager::SetProceduralVariable(RE::Actor* a_actor, const std::string_view a_name, float a_value)
	{
		return VisitGraph(a_actor, [&](Graph* g) {
			auto gen = g->generator.get();
			if (!gen)
				return false;

			if (gen->GetType() != GenType::kProcedural)
				return false;

			return static_cast<ProceduralGenerator*>(gen)->SetVariable(a_name, a_value);
		});
	}

	float GraphManager::GetProceduralVariable(RE::Actor* a_actor, const std::string_view a_name)
	{
		float result = 0.0f;

		VisitGraph(a_actor, [&](Graph* g) {
			auto gen = g->generator.get();
			if (!gen)
				return false;

			if (gen->GetType() != GenType::kProcedural)
				return false;

			result = static_cast<ProceduralGenerator*>(gen)->GetVariable(a_name);
			return true;
		});

		return result;
	}

	void GraphManager::SetGraphControlsPosition(RE::Actor* a_actor, bool a_controls)
	{
		VisitGraph(a_actor, [&](Graph* g) {
			g->SetLockPosition(a_controls);
			return true;
		});
	}

	bool GraphManager::AttachGenerator(RE::Actor* a_actor, std::unique_ptr<Generator> a_gen, float a_transitionTime)
	{
		return VisitGraph(a_actor, [&](Graph* g) {
			g->DetachSequencer(false);
			g->StartTransition(std::move(a_gen), a_transitionTime);
			return true;
		}, true);
	}

	bool GraphManager::DetachGenerator(RE::Actor* a_actor, float a_transitionTime)
	{
		bool detachRequired = false;
		bool result = VisitGraph(a_actor, [&](Graph* g) {
			g->flags.reset(Graph::FLAGS::kLoadingAnimation);
			g->DetachSequencer(false);

			if (g->flags.none(Graph::FLAGS::kUnloaded3D)) {
				g->StartTransition(nullptr, a_transitionTime);
			} else {
				detachRequired = true;
			}
			return true;
		});

		if (detachRequired) {
			DetachGraph(a_actor);
		}

		return result;
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

	void GraphManager::SetGraphLoaded(RE::IAnimationGraphManagerHolder* a_graph, bool a_loaded)
	{
		std::unique_lock l{ stateLock };
		if (a_loaded) {
			auto iter = state->unloadedGraphs.find(a_graph);
			if (iter == state->unloadedGraphs.end())
				return;

			auto ele = state->unloadedGraphs.extract(iter);
			state->loadedGraphs.insert(std::move(ele));
		} else {
			auto iter = state->loadedGraphs.find(a_graph);
			if (iter == state->loadedGraphs.end())
				return;

			auto ele = state->loadedGraphs.extract(iter);
			state->unloadedGraphs.insert(std::move(ele));
		}
	}

	std::shared_ptr<Graph> CreateGraph(RE::Actor* a_actor)
	{
		std::shared_ptr<Graph> g = std::make_shared<Graph>();
		g->target.reset(a_actor);
		g->SetSkeleton(Settings::GetSkeleton(a_actor));
		auto loadedData = a_actor->loadedData.lock_read();
		if (*loadedData != nullptr) {
			g->GetSkeletonNodes(static_cast<RE::BGSFadeNode*>(loadedData->data3D.get()));
		} else {
			g->GetSkeletonNodes(nullptr);
		}
		g->SetLockPosition(true);
		return g;
	}

	std::shared_ptr<Graph> GraphManager::GetGraphLockless(RE::Actor* a_actor, bool create)
	{
		if (auto iter = state->loadedGraphs.find(a_actor); iter != state->loadedGraphs.end())
			return iter->second;

		if (auto iter = state->unloadedGraphs.find(a_actor); iter != state->unloadedGraphs.end())
			return iter->second;

		if (!create)
			return nullptr;

		std::shared_ptr<Graph> g = CreateGraph(a_actor);
		if (g->flags.none(Graph::FLAGS::kUnloaded3D)) {
			state->loadedGraphs[a_actor] = g;
		} else {
			state->unloadedGraphs[a_actor] = g;
		}
		return g;
	}

	bool GraphManager::DetachGraph(RE::IAnimationGraphManagerHolder* a_graphHolder)
	{
		std::unique_lock l{ stateLock };
		if (auto iter = state->loadedGraphs.find(a_graphHolder); iter != state->loadedGraphs.end()) {
			iter->second->OnDetach();
			state->loadedGraphs.erase(iter);
			return true;
		}
		if (auto iter = state->unloadedGraphs.find(a_graphHolder); iter != state->unloadedGraphs.end()) {
			iter->second->OnDetach();
			state->unloadedGraphs.erase(iter);
			return true;
		}
		return false;
	}

	bool GraphManager::VisitGraph(RE::Actor* a_actor, const std::function<bool(Graph*)> visitFunc, bool create)
	{
		if (!a_actor)
			return false;

		auto g = GetGraph(a_actor, create);
		if (!g) {
			return false;
		}

		std::unique_lock l{ g->lock };
		return visitFunc(g.get());
	}

	void GraphManager::GetAllGraphs(std::vector<std::pair<RE::TESObjectREFR*, std::weak_ptr<Graph>>>& a_refsOut)
	{
		std::shared_lock ls{ stateLock };
		a_refsOut.clear();
		for (auto& iter : state->loadedGraphs) {
			a_refsOut.emplace_back(static_cast<RE::TESObjectREFR*>(iter.first), iter.second);
		}
		for (auto& iter : state->unloadedGraphs) {
			a_refsOut.emplace_back(static_cast<RE::TESObjectREFR*>(iter.first), iter.second);
		}
	}

	GraphManager& gm = *GraphManager::GetSingleton();

	static Util::Call5Hook<void(RE::IAnimationGraphManagerHolder*, RE::BSAnimationUpdateData*, void*)> GraphUpdateHook(118488, 0x61, "IAnimationGraphManagerHolder::UpdateAnimationGraphManager",
		[](RE::IAnimationGraphManagerHolder* a_graphHolder, RE::BSAnimationUpdateData* a_updateData, void* a_graph) {
			std::shared_lock l{ gm.stateLock };
			auto& m = gm.state->loadedGraphs;
			if (auto iter = m.find(a_graphHolder); iter != m.end()) {
				auto& g = iter->second;

				bool modelCulled = a_updateData->modelCulled;
				a_updateData->modelCulled = modelCulled || !g->GetRequiresBaseTransforms();
				PERF_TIMER_START(start);
				GraphUpdateHook(a_graphHolder, a_updateData, a_graph);
				PERF_TIMER_END(start, baseUpdateTime);

				std::unique_lock gl{ g->lock };
				PERF_TIMER_COPY_VALUE(baseUpdateTime, g->baseUpdateMS);
				g->Update(a_updateData->timeDelta, !modelCulled);

				if (g->GetRequiresDetach()) {
					l.unlock();
					gl.unlock();
					gm.DetachGraph(a_graphHolder);
				}
			} else {
				l.unlock();
				GraphUpdateHook(a_graphHolder, a_updateData, a_graph);
			}
		});

	static Util::VFuncHook<void*(void*,void*)> PerspectiveSwitchHook(422984, 0x1, "::HandlePlayerPerspectiveSwitchForEyeTracking",
		[](void* a1, void* a2) -> void* {
			void* res = PerspectiveSwitchHook(a1, a2);

			if (auto g = gm.GetGraph(RE::PlayerCharacter::GetSingleton(), false); g) {
				std::unique_lock l{ g->lock };
				g->flags.set(Graph::FLAGS::kRequiresEyeTrackUpdate);
			}

			return res;
		});

	static Util::VFuncHook<void*(RE::Actor*, RE::NiAVObject*, bool, bool)> ActorSet3DHook(422688, 0xA8, "Actor::Set3D",
		[](RE::Actor* a_this, RE::NiAVObject* a_3d, bool a_flag1, bool a_flag2) -> void* {
			void* result = ActorSet3DHook(a_this, a_3d, a_flag1, a_flag2);

			if (auto g = gm.GetGraph(a_this, false); g) {
				std::unique_lock l{ g->lock };
				g->GetSkeletonNodes(static_cast<RE::BGSFadeNode*>(a_3d));
				bool isLoaded = g->flags.none(Graph::FLAGS::kUnloaded3D);
				l.unlock();
				gm.SetGraphLoaded(a_this, isLoaded);
			}

			return result;
		});
}