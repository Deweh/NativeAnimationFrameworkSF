#pragma once
#include "Graph.h"
#include "Tasks/MainLoop.h"

namespace Animation
{
	struct AnimationIdentifer
	{
		enum Type : uint8_t
		{
			kIndex = 0,
			kName = 1
		};

		Type type = kIndex;
		std::string name = "";
		size_t index = 0;
	};

	enum GLTFErrorCode : uint16_t
	{
		kSuccess = 0,
		kNoSkeleton = 1,
		kFailedToLoad = 2,
		kFailedToMakeClip = 3,
		kInvalidAnimationIdentifier = 4
	};

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
		GLTFErrorCode PlayAnimationFromGLTF(RE::Actor* a_actor, float a_transitionTime, const std::string& a_fileName, const AnimationIdentifer& a_id = {});
		bool AttachGenerator(RE::Actor* a_actor, std::unique_ptr<Generator> a_gen, float a_transitionTime);
		bool DetachGenerator(RE::Actor* a_actor, float a_transitionTime);
		bool DetachGraph(RE::IAnimationGraphManagerHolder* a_graphHolder);
		void InstallHooks();
		void Reset();

	private:
		std::shared_ptr<Graph> GetGraph(RE::Actor* a_actor, bool create);
	};
}