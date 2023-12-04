#include "Commands.h"
#include "Util/String.h"
#include "polyhook2/Detour/x64Detour.hpp"

namespace Commands
{
	RE::NiPointer<RE::TESObjectREFR> GetRefrFromHandle(uint32_t handle)
	{
		RE::NiPointer<RE::TESObjectREFR> result;
		REL::Relocation<void(RE::NiPointer<RE::TESObjectREFR>&, uint32_t*)> func(REL::ID(72399));
		func(result, &handle);
		return result;
	}

	RE::NiPointer<RE::TESObjectREFR> GetConsoleRefr()
	{
		REL::Relocation<uint64_t**> consoleReferencesManager(REL::ID(879512));
		REL::Relocation<uint32_t* (uint64_t*, uint32_t*)> GetConsoleHandle(REL::ID(166314));
		uint32_t outId = 0;
		GetConsoleHandle(*consoleReferencesManager, &outId);
		return GetRefrFromHandle(outId);
	}

	typedef int64_t (*ExecuteCommandFunc)(void*, const char*);

	std::unique_ptr<PLH::x64Detour> ExecuteCommandDetour = nullptr;
	ExecuteCommandFunc OriginalExecuteCommand;
	std::unordered_map<std::string, CommandFunction> registrations;

	void RegisterCommand(const std::string& name, const CommandFunction& func)
	{
		registrations[name] = func;
	}

	int64_t ExecuteCommand(void* a1, const char* a_cmd, ...)
	{
		if (!a_cmd) {
			return OriginalExecuteCommand(a1, a_cmd);
		}

		std::string_view cmdView(a_cmd);
		auto args = Util::String::Split(cmdView, " ", '\"');
		if (auto iter = registrations.find(std::string(args[0])); iter != registrations.end()) {
			auto refr = GetConsoleRefr();
			iter->second(args, cmdView, refr.get());
			return 0;
		} else {
			return OriginalExecuteCommand(a1, a_cmd);
		}
	}

	void InstallHooks()
	{
		REL::Relocation<uintptr_t> hookLoc{ REL::ID(166307) };
		ExecuteCommandDetour = std::make_unique<PLH::x64Detour>(
			static_cast<uint64_t>(hookLoc.address()),
			reinterpret_cast<uint64_t>(&ExecuteCommand),
			reinterpret_cast<uint64_t*>(&OriginalExecuteCommand));
		ExecuteCommandDetour->hook();
		
		INFO("Installed command hook.");
	}
}