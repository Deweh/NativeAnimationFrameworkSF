#include "Commands.h"
#include "Util/String.h"

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

	typedef void (*ExecuteCommandFunc)(void*, const char*);

	ExecuteCommandFunc OriginalExecuteCommand;
	std::unordered_map<std::string, CommandFunction> registrations;

	void RegisterCommand(const std::string& name, const CommandFunction& func)
	{
		registrations[name] = func;
	}

	void ExecuteCommand(void* a1, const char* a_cmd)
	{
		if (!a_cmd) {
			OriginalExecuteCommand(a1, a_cmd);
			return;
		}

		std::string_view cmdView(a_cmd);
		auto args = Util::String::Split(cmdView, " ", '\"');
		if (auto iter = registrations.find(std::string(args[0])); iter != registrations.end()) {
			auto refr = GetConsoleRefr();
			iter->second(args, cmdView, refr.get());
		} else {
			OriginalExecuteCommand(a1, a_cmd);
		}
	}

	void InstallHooks()
	{
		auto& t = SFSE::GetTrampoline();

		REL::Relocation<uintptr_t> hookLoc1{ REL::ID(148098), 0x184 };
		OriginalExecuteCommand = reinterpret_cast<ExecuteCommandFunc>(t.write_call<5>(hookLoc1.address(), &ExecuteCommand));
		
		REL::Relocation<uintptr_t> hookLoc2{ REL::ID(149017), 0x35 };
		t.write_call<5>(hookLoc2.address(), &ExecuteCommand);

		REL::Relocation<uintptr_t> hookLoc3{ REL::ID(166251), 0x142 };
		t.write_call<5>(hookLoc3.address(), &ExecuteCommand);

		REL::Relocation<uintptr_t> hookLoc4{ REL::ID(166292), 0x15 };
		t.write_call<5>(hookLoc4.address(), &ExecuteCommand);

		REL::Relocation<uintptr_t> hookLoc5{ REL::ID(166311), 0x1E };
		t.write_call<5>(hookLoc5.address(), &ExecuteCommand);

		REL::Relocation<uintptr_t> hookLoc6{ REL::ID(166324), 0x18A };
		t.write_call<5>(hookLoc6.address(), &ExecuteCommand);

		REL::Relocation<uintptr_t> hookLoc7{ REL::ID(170824), 0x53 };
		t.write_call<5>(hookLoc7.address(), &ExecuteCommand);

		REL::Relocation<uintptr_t> hookLoc8{ REL::ID(170824), 0x40 };
		t.write_call<5>(hookLoc8.address(), &ExecuteCommand);

		REL::Relocation<uintptr_t> hookLoc9{ REL::ID(170824), 0x29 };
		t.write_call<5>(hookLoc9.address(), &ExecuteCommand);
		
		INFO("Installed command hook.");
	}
}