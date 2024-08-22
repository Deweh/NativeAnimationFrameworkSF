#pragma once
#include "Ozz.h"
#include "Sequencer.h"
#include "Util/Event.h"

namespace Animation
{
	class Graph;
	class Generator;

	class GraphManager :
		public Util::Event::MultiDispatcher<SequencePhaseChangeEvent>
	{
	public:
		struct PersistentState
		{
			std::unordered_map<RE::IAnimationGraphManagerHolder*, std::shared_ptr<Graph>> loadedGraphs;
			std::unordered_map<RE::IAnimationGraphManagerHolder*, std::shared_ptr<Graph>> unloadedGraphs;
		};

		std::shared_mutex stateLock;
		std::unique_ptr<PersistentState> state = std::make_unique<PersistentState>();

		static GraphManager* GetSingleton();
		bool LoadAndStartAnimation(RE::Actor* a_actor, const std::string_view a_filePath, const std::string_view a_animId = "", float a_transitionTime = 1.0f);
		void StartSequence(RE::Actor* a_actor, std::vector<Sequencer::PhaseData>&& a_phaseData);
		bool AdvanceSequence(RE::Actor* a_actor, bool a_smooth = true);
		bool SetSequencePhase(RE::Actor* a_actor, size_t a_idx);
		size_t GetSequencePhase(RE::Actor* a_actor);
		bool SetAnimationSpeed(RE::Actor* a_actor, float a_speed);
		float GetAnimationSpeed(RE::Actor* a_actor);
		void SyncGraphs(const std::vector<RE::Actor*>& a_actors);
		void StopSyncing(RE::Actor* a_actor);
		bool SetProceduralVariable(RE::Actor* a_actor, const std::string_view a_name, float a_value);
		float GetProceduralVariable(RE::Actor* a_actor, const std::string_view a_name);
		bool AttachGenerator(RE::Actor* a_actor, std::unique_ptr<Generator> a_gen, float a_transitionTime);
		bool DetachGenerator(RE::Actor* a_actor, float a_transitionTime);
		bool DetachGraph(RE::IAnimationGraphManagerHolder* a_graphHolder);
		bool VisitGraph(RE::Actor* a_actor, const std::function<bool(Graph*)> visitFunc, bool create = false);
		void GetAllGraphs(std::vector<std::pair<RE::TESObjectREFR*, std::weak_ptr<Graph>>>& a_refsOut);
		void Reset();
		std::shared_ptr<Graph> GetGraph(RE::Actor* a_actor, bool create);
		void SetGraphLoaded(RE::IAnimationGraphManagerHolder* a_graph, bool a_loaded);

	private:
		std::shared_ptr<Graph> GetGraphLockless(RE::Actor* a_actor, bool create);
	};
}