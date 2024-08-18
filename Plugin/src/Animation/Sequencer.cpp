#include "Sequencer.h"
#include "Graph.h"
#include "GraphManager.h"

namespace Animation
{
	Sequencer::Sequencer(std::vector<PhaseData>&& a_phases)
	{
		phases = std::move(a_phases);
		currentPhase = phases.begin();
	}

	void Sequencer::Update()
	{
		if (owner->generator->rootResetRequired || (flags.any(FLAG::kForceAdvance) && flags.none(FLAG::kSmoothAdvance))) {
			if (loopsRemaining == 0 || flags.any(FLAG::kForceAdvance)) {
				owner->generator->rootResetRequired = false;
				owner->generator->localTime = (owner->generator->duration - 0.0001f);
				AdvancePhase();
			} else if (loopsRemaining > 0) {
				loopsRemaining--;
			}
		}
	}

	bool Sequencer::Synchronize(Sequencer* a_owner)
	{
		if (flags.any(FLAG::kPausedForLoading) || a_owner->flags.any(FLAG::kPausedForLoading))
			return false;

		if (auto idx = a_owner->GetPhase(); GetPhase() != idx) {
			SetPhase(idx);
			return false;
		} else {
			return true;
		}
	}

	void Sequencer::OnAttachedToGraph(Graph* a_graph)
	{
		owner = a_graph;
		AdvancePhase(true);
	}

	bool Sequencer::OnAnimationRequested(const FileID& a_id)
	{
		bool belongsToThis = (flags.any(FLAG::kPausedForLoading, FLAG::kLoadingNextAnim) && a_id == loadingFile);
		if (belongsToThis) {
			owner->flags.set(Graph::FLAGS::kLoadingSequencerAnimation);
		}
		return belongsToThis;
	}

	bool Sequencer::OnAnimationReady(const FileID& a_id, std::shared_ptr<IAnimationFile> a_anim)
	{
		if (!flags.any(FLAG::kPausedForLoading, FLAG::kLoadingNextAnim) || a_id != loadingFile) {
			return false;
		}

		owner->flags.reset(Graph::FLAGS::kLoadingSequencerAnimation);

		if (!a_anim) {
			Exit();
			return true;
		}

		loadedAnim = a_anim;
		bool wasPaused = flags.all(FLAG::kPausedForLoading);
		flags.reset(FLAG::kPausedForLoading, FLAG::kLoadingNextAnim);

		if (wasPaused) {
			AdvancePhase(true);
		}

		return true;
	}

	void Sequencer::LoadNextAnimation()
	{
		loadedAnim = nullptr;

		auto next = GetNextPhase();
		if (!next.has_value())
			return;
		
		flags.set(FLAG::kLoadingNextAnim);
		loadingFile = next.value()->file;
		FileManager::GetSingleton()->RequestAnimation(next.value()->file, owner->skeleton->name, owner->weak_from_this());
	}

	void Sequencer::AdvancePhase(bool a_init)
	{
		flags.reset(FLAG::kForceAdvance, FLAG::kSmoothAdvance);

		if (!a_init) {
			auto next = GetNextPhase();
			if (next.has_value()) {
				currentPhase = next.value();
			} else {
				Exit();
				return;
			}
		}

		loopsRemaining = currentPhase->loopCount;

		if (loadedAnim) {
			TransitionToLoadedAnimation();
		} else {
			SpotLoadCurrentAnimation();
		}
	}

	std::optional<Sequencer::phases_iterator> Sequencer::GetNextPhase()
	{
		auto next = std::next(currentPhase);
		if (next == phases.end()) {
			if (flags.all(FLAG::kLoop)) {
				return phases.begin();
			} else {
				return std::nullopt;
			}
		} else {
			return next;
		}
	}

	void Sequencer::SetPhase(size_t idx)
	{
		if (idx >= phases.size() || std::distance(phases.begin(), currentPhase) == idx)
			return;

		flags.reset(FLAG::kForceAdvance, FLAG::kSmoothAdvance);
		auto next = GetNextPhase();
		currentPhase = phases.begin() + idx;

		loopsRemaining = currentPhase->loopCount;

		if (loadedAnim && next.has_value() && next.value() == currentPhase) {
			TransitionToLoadedAnimation();
		} else {
			SpotLoadCurrentAnimation();
		}
	}

	size_t Sequencer::GetPhase()
	{
		return std::distance(phases.begin(), currentPhase);
	}

	void Sequencer::TransitionToLoadedAnimation()
	{
		owner->StartTransition(loadedAnim->CreateGenerator(), currentPhase->transitionTime);
		LoadNextAnimation();
		SFSE::GetTaskInterface()->AddTask([idx = GetPhase(), target = owner->target]() {
			GraphManager::GetSingleton()->SendEvent(SequencePhaseChangeEvent{
				.exiting = false,
				.target = target,
				.index = idx
			});
		});
	}

	void Sequencer::SpotLoadCurrentAnimation()
	{
		flags.set(FLAG::kPausedForLoading);
		if (owner->generator) {
			owner->generator->paused = true;
		}
		loadingFile = currentPhase->file;
		FileManager::GetSingleton()->RequestAnimation(currentPhase->file, owner->skeleton->name, owner->weak_from_this());
	}

	void Sequencer::Exit()
	{
		if (owner->generator) {
			owner->generator->rootResetRequired = false;
			owner->generator->localTime = (owner->generator->duration - 0.0001f);
			owner->generator->paused = true;
		}
		owner->DetachSequencer();
	}
}