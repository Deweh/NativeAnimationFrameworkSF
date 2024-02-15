#pragma once
#include "Graph.h"
#include "Ozz.h"

namespace Animation
{
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
		bool AttachGeneratorsSynced(const std::vector<RE::Actor*>& a_actors, std::vector<std::unique_ptr<Generator>>& a_gens, float a_transitionTime, bool alignRoots);
		bool AttachGenerator(RE::Actor* a_actor, std::unique_ptr<Generator> a_gen, float a_transitionTime);
		bool DetachGenerator(RE::Actor* a_actor, float a_transitionTime);
		bool DetachGraph(RE::IAnimationGraphManagerHolder* a_graphHolder);
		void InstallHooks();
		void Reset();

	private:
		std::shared_ptr<Graph> GetGraph(RE::Actor* a_actor, bool create);
		std::shared_ptr<Graph> GetGraphLockless(RE::Actor* a_actor, bool create);
	};
}