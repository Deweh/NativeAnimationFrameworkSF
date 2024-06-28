#pragma once
#include "Ozz.h"
#include "Sequencer.h"

namespace Animation
{
	class Graph;
	class Generator;

	class GraphManager
	{
	public:
		struct PersistentState
		{
			std::map<RE::IAnimationGraphManagerHolder*, std::shared_ptr<Graph>> graphMap;
		};

		std::shared_mutex stateLock;
		std::unique_ptr<PersistentState> state = std::make_unique<PersistentState>();

		static GraphManager* GetSingleton();
		static std::unique_ptr<Generator> CreateAnimationGenerator(std::shared_ptr<OzzAnimation> anim);
		bool LoadAndStartAnimation(RE::Actor* a_actor, const std::string_view a_filePath, const std::string_view a_animId = "", float a_transitionTime = 1.0f);
		bool StartSequence(RE::Actor* a_actor, std::vector<Sequencer::PhaseData>&& a_phaseData);
		bool AdvanceSequence(RE::Actor* a_actor, bool a_smooth = true);
		void SyncGraphs(const std::vector<RE::Actor*>& a_actors);
		void StopSyncing(RE::Actor* a_actor);
		bool AttachGenerator(RE::Actor* a_actor, std::unique_ptr<Generator> a_gen, float a_transitionTime);
		bool DetachGenerator(RE::Actor* a_actor, float a_transitionTime);
		bool DetachGraph(RE::IAnimationGraphManagerHolder* a_graphHolder);
		void VisitGraph(RE::Actor* a_actor, const std::function<void(Graph*)> visitFunc);
		void InstallHooks();
		void Reset();
		std::shared_ptr<Graph> GetGraph(RE::Actor* a_actor, bool create);

	private:
		std::shared_ptr<Graph> GetGraphLockless(RE::Actor* a_actor, bool create);
	};
}