#pragma once
#include "Generator.h"
#include "FileManager.h"

namespace Animation
{
	class Graph;

	class Sequencer
	{
	public:
		enum FLAG : uint8_t
		{
			kNone,
			kForceAdvance,
			kSmoothAdvance,
			kLoadingNextAnim,
			kPausedForLoading,
			kLoop
		};

		struct PhaseData
		{
			FileID file;
			int32_t loopCount = 0;
			float transitionTime = 1.0f;
		};

		using phases_vector = std::vector<PhaseData>;
		using phases_iterator = phases_vector::iterator;

		Sequencer(std::vector<PhaseData>&& a_phases);

		SFSE::stl::enumeration<FLAG, uint8_t> flags = kNone;
		int32_t loopsRemaining = 0;
		phases_iterator currentPhase;
		phases_vector phases;
		Graph* owner;
		std::shared_ptr<OzzAnimation> loadedAnim = nullptr;
		FileID loadingFile;

		void Update();
		void OnAttachedToGraph(Graph* a_graph);
		bool OnAnimationRequested(const FileID& a_id);
		bool OnAnimationReady(const FileID& a_id, std::shared_ptr<OzzAnimation> a_anim);
		void LoadNextAnimation();
		void AdvancePhase(bool a_init = false);
		std::optional<phases_iterator> GetNextPhase();
		void SetPhase(size_t idx);
		void TransitionToLoadedAnimation();
		void SpotLoadCurrentAnimation();
		void Exit();
	};
}