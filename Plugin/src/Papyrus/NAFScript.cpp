#include "NAFScript.h"
#include "Animation/GraphManager.h"
#include "Animation/Graph.h"

namespace Papyrus::NAFScript
{
	using namespace RE::BSScript;
	using ErrorLevel = ErrorLogger::Severity;
	using SequencePhase = structure_wrapper<"NAF", "SequencePhase">;

	auto agm = Animation::GraphManager::GetSingleton();

	namespace detail
	{
		bool UnpackSequencePhases(const std::span<SequencePhase>& a_phases, std::vector<Animation::Sequencer::PhaseData>& a_out)
		{
			a_out.reserve(a_phases.size());
			for (const auto& p : a_phases) {
				auto numLoops = p.find<int32_t>("numLoops", true);
				auto transitionTime = p.find<float>("transitionTime", true);
				auto filePath = p.find<std::string_view>("filePath", true);

				if (!numLoops || !transitionTime || !filePath) {
					return false;
				}

				a_out.emplace_back(Animation::FileID(*filePath, ""), *numLoops, *transitionTime);
			}
			return true;
		}
	}

	void PlayAnimation(IVirtualMachine& a_vm, uint32_t a_stackID, std::monostate, RE::Actor* a_actor, std::string a_anim, float a_transitionTime)
	{
		if (!a_actor) {
			a_vm.PostError("Cannot play an animation on a none actor.", a_stackID, ErrorLevel::kInfo);
			return;
		}

		agm->LoadAndStartAnimation(a_actor, a_anim, "", a_transitionTime);
	}

	bool StopAnimation(IVirtualMachine& a_vm, uint32_t a_stackID, std::monostate, RE::Actor* a_actor, float a_transitionTime)
	{
		return agm->DetachGenerator(a_actor, a_transitionTime);
	}

	void SyncAnimations(IVirtualMachine& a_vm, uint32_t a_stackID, std::monostate, std::vector<RE::Actor*> a_actors)
	{
		for (const auto& a : a_actors) {
			if (!a) {
				a_vm.PostError("Cannot sync a none actor.", a_stackID, ErrorLevel::kInfo);
				return;
			}
		}

		agm->SyncGraphs(a_actors);
	}

	void StopSyncing(IVirtualMachine& a_vm, uint32_t a_stackID, std::monostate, RE::Actor* a_actor)
	{
		if (!a_actor) {
			a_vm.PostError("Cannot stop syncing a none actor.", a_stackID, ErrorLevel::kInfo);
			return;
		}

		agm->StopSyncing(a_actor);
	}

	void StartSequence(IVirtualMachine& a_vm, uint32_t a_stackID, std::monostate, RE::Actor* a_actor, std::vector<SequencePhase> a_phases, bool a_loop)
	{
		if (!a_actor) {
			a_vm.PostError("Cannot start sequence on a none actor.", a_stackID, ErrorLevel::kInfo);
			return;
		}

		if (a_phases.empty()) {
			a_vm.PostError("Cannot start an empty sequence.", a_stackID, ErrorLevel::kInfo);
			return;
		}

		std::vector<Animation::Sequencer::PhaseData> phases;
		if (!detail::UnpackSequencePhases(a_phases, phases)) {
			a_vm.PostError("One or more phases are missing struct data, cannot start sequence.", a_stackID, ErrorLevel::kInfo);
			return;
		}

		agm->StartSequence(a_actor, std::move(phases));
	}

	bool AdvanceSequence(IVirtualMachine& a_vm, uint32_t a_stackID, std::monostate, RE::Actor* a_actor, bool a_smooth)
	{
		if (!a_actor) {
			a_vm.PostError("Cannot advance sequence on a none actor.", a_stackID, ErrorLevel::kInfo);
			return false;
		}

		return agm->AdvanceSequence(a_actor, a_smooth);
	}

	bool SetSequencePhase(IVirtualMachine& a_vm, uint32_t a_stackID, std::monostate, RE::Actor* a_actor, int32_t a_idx)
	{
		if (!a_actor) {
			a_vm.PostError("Cannot set sequence phase on a none actor.", a_stackID, ErrorLevel::kInfo);
			return false;
		}

		return agm->SetSequencePhase(a_actor, a_idx);
	}

	int32_t GetSequencePhase(IVirtualMachine& a_vm, uint32_t a_stackID, std::monostate, RE::Actor* a_actor)
	{
		if (!a_actor) {
			a_vm.PostError("Cannot get sequence phase for a none actor.", a_stackID, ErrorLevel::kInfo);
			return -1;
		}

		size_t result = agm->GetSequencePhase(a_actor);
		if (result == UINT64_MAX) {
			return -1;
		}
		return static_cast<int32_t>(result);
	}

	bool SetAnimationSpeed(IVirtualMachine& a_vm, uint32_t a_stackID, std::monostate, RE::Actor* a_actor, float a_speed)
	{
		return agm->SetAnimationSpeed(a_actor, a_speed * 0.01f);
	}

	float GetAnimationSpeed(IVirtualMachine& a_vm, uint32_t a_stackID, std::monostate, RE::Actor* a_actor)
	{
		return agm->GetAnimationSpeed(a_actor) * 100.0f;
	}

	std::string GetCurrentAnimation(std::monostate, RE::Actor* a_actor)
	{
		if (!a_actor) {
			return "";
		}
		
		auto g = agm->GetGraph(a_actor, false);
		if (!g) {
			return "";
		}

		std::unique_lock l{ g->lock };
		if (!g->generator) {
			return "";
		}

		return std::string{ g->generator->GetSourceFile() };
	}

	void RegisterFunctions(IVirtualMachine* a_vm)
	{
		a_vm->BindNativeMethod(SCRIPT_NAME, "PlayAnimation", &PlayAnimation, true, false);
		a_vm->BindNativeMethod(SCRIPT_NAME, "StopAnimation", &StopAnimation, true, false);
		a_vm->BindNativeMethod(SCRIPT_NAME, "SyncAnimations", &SyncAnimations, true, false);
		a_vm->BindNativeMethod(SCRIPT_NAME, "StopSyncing", &StopSyncing, true, false);
		a_vm->BindNativeMethod(SCRIPT_NAME, "StartSequence", &StartSequence, true, false);
		a_vm->BindNativeMethod(SCRIPT_NAME, "AdvanceSequence", &AdvanceSequence, true, false);
		a_vm->BindNativeMethod(SCRIPT_NAME, "SetSequencePhase", &SetSequencePhase, true, false);
		a_vm->BindNativeMethod(SCRIPT_NAME, "GetSequencePhase", &GetSequencePhase, true, false);
		a_vm->BindNativeMethod(SCRIPT_NAME, "SetAnimationSpeed", &SetAnimationSpeed, true, false);
		a_vm->BindNativeMethod(SCRIPT_NAME, "GetAnimationSpeed", &GetAnimationSpeed, true, false);
		a_vm->BindNativeMethod(SCRIPT_NAME, "GetCurrentAnimation", &GetCurrentAnimation, true, false);
	}
}