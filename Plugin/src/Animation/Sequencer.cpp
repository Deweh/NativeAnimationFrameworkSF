#include "Sequencer.h"
#include "Graph.h"

namespace Animation
{
	Sequencer::Sequencer(std::vector<PhaseData>&& a_phases)
	{
		phases = std::move(a_phases);
		currentPhase = phases.begin();
	}

	bool Sequencer::Update()
	{
		if (owner->generator->rootResetRequired || (flags.all(FLAG::kForceAdvance) && flags.none(FLAG::kSmoothAdvance))) {
			if (loopsRemaining == 0 || flags.all(FLAG::kForceAdvance)) {
				owner->generator->rootResetRequired = false;
				owner->generator->localTime = (owner->generator->duration - 0.0001f);
				AdvancePhase();
				return true;
			} else if (loopsRemaining > 0) {
				loopsRemaining--;
			}
		}
		return false;
	}

	void Sequencer::OnAttachedToGraph(Graph* a_graph)
	{
		owner = a_graph;
		AdvancePhase(true);
	}

	bool Sequencer::OnAnimationRequested(const FileID& a_id)
	{
		return flags.any(FLAG::kPausedForLoading, FLAG::kLoadingNextAnim) && a_id == loadingFile;
	}

	bool Sequencer::OnAnimationReady(const FileID& a_id, std::shared_ptr<OzzAnimation> a_anim)
	{
		if (!flags.any(FLAG::kPausedForLoading, FLAG::kLoadingNextAnim) || a_id != loadingFile) {
			return false;
		}

		if (!a_anim) {
			DetachFromGraph();
			return;
		}

		nextAnim = a_anim;
		bool wasPaused = flags.all(FLAG::kPausedForLoading);
		flags.reset(FLAG::kPausedForLoading, FLAG::kLoadingNextAnim);

		if (wasPaused) {
			AdvancePhase(true);
		}

		return true;
	}

	void Sequencer::LoadNextAnimation()
	{
		nextAnim = nullptr;

		auto next = std::next(currentPhase);
		if (next == phases.end()) {
			if (flags.all(FLAG::kLoop)) {
				next = phases.begin();
			} else {
				return;
			}
		}
		
		flags.set(FLAG::kLoadingNextAnim);
		loadingFile = next->file;
		FileManager::GetSingleton()->RequestAnimation(next->file, owner->skeleton, owner->weak_from_this());
	}

	void Sequencer::AdvancePhase(bool a_init)
	{
		flags.reset(FLAG::kForceAdvance, FLAG::kSmoothAdvance);

		if (!a_init) {
			auto next = std::next(currentPhase);
			if (next == phases.end()) {
				if (flags.all(FLAG::kLoop)) {
					currentPhase = phases.begin();
				} else {
					DetachFromGraph();
					return;
				}
			} else {
				currentPhase = next;
			}
		}

		loopsRemaining = currentPhase->loopCount;

		if (nextAnim) {
			auto gen = std::make_unique<LinearClipGenerator>();
			gen->anim = nextAnim;
			gen->duration = nextAnim->data->duration();
			owner->StartTransition(std::move(gen), currentPhase->transitionTime);
			LoadNextAnimation();
		} else {
			flags.set(FLAG::kPausedForLoading);
			if (owner->generator) {
				owner->generator->paused = true;
			}
			loadingFile = currentPhase->file;
			FileManager::GetSingleton()->RequestAnimation(currentPhase->file, owner->skeleton, owner->weak_from_this());
		}
	}

	void Sequencer::DetachFromGraph()
	{
		owner->sequencer.reset();
		owner->StartTransition(nullptr, 1.0f);
	}
}