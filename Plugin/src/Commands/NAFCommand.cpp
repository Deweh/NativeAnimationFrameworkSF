#include "NAFCommand.h"
#include "Serialization/GLTFImport.h"
#include "Settings/Settings.h"
#include "Animation/GraphManager.h"
#include "Animation/Ozz.h"
#include "Util/String.h"

namespace Commands::NAFCommand
{
	CCF::ConsoleInterface* itfc;
	CCF::simple_array<CCF::simple_string_view> args;
	const char* fullStr = nullptr;

	void ShowHelp()
	{
		itfc->PrintLn("Usage:");
		itfc->PrintLn("naf play [anim-file]");
		itfc->PrintLn("naf stop");
		itfc->PrintLn("naf studio");
	}

	void ShowNoActor()
	{
		itfc->PrintLn("No actor selected.");
	}

	RE::Actor* StrToActor(const std::string_view s, bool verbose = false)
	{
		auto frm = itfc->HexStrToForm(s.data());
		if (!frm) {
			if (verbose)
				itfc->PrintLn("Provided form ID is invalid.");
			return nullptr;
		}
		RE::Actor* actor = starfield_cast<RE::Actor*>(frm);
		if (!actor) {
			if (verbose)
				itfc->PrintLn("Provided form ID does not point to an actor.");
			return nullptr;
		}
		return actor;
	}

	RE::Actor* ActorStrOrSelection(size_t argPos)
	{
		if (args.size() > argPos) {
			return StrToActor(args[argPos], true);
		} else {
			auto refr = itfc->GetSelectedReference();
			if (refr.get() == nullptr) {
				ShowNoActor();
				return nullptr;
			}
			RE::Actor* result = starfield_cast<RE::Actor*>(refr.get());
			if (!result) {
				ShowNoActor();
			}
			return result;
		}
	}

	void ProcessPlayCommand()
	{
		if (args.size() < 2) {
			ShowHelp();
			return;
		}

		RE::Actor* actor = ActorStrOrSelection(2);
		if (!actor) {
			return;
		}

		Animation::GraphManager::GetSingleton()->LoadAndStartAnimation(actor, args[1]);
		itfc->PrintLn("Starting animation...");
	}

	void ProcessStopCommand()
	{
		RE::Actor* actor = ActorStrOrSelection(1);
		if (!actor) {
			return;
		}

		Animation::GraphManager::GetSingleton()->DetachGenerator(actor, 1.0f);
		itfc->PrintLn("Stopping animation...");
	}

	void ProcessStudioCommand()
	{
		const auto hndl = GetModuleHandleA("NAFStudio.dll");
		if (hndl != NULL) {
			const auto addr = GetProcAddress(hndl, "OpenStudio");
			if (addr != NULL) {
				(reinterpret_cast<void(*)()>(addr))();
			} else {
				itfc->PrintLn("Failed to open NAF Studio.");
			}
		} else {
			itfc->PrintLn("NAF Studio is not installed.");
		}
	}

	void ProcessPapyrusCommand()
	{
		if (args.size() < 2) {
			return;
		}

		auto type = args[1].get();
		if (type == "p" && args.size() > 3) {
			auto a = StrToActor(args[3]);
			if (!a) {
				return;
			}

			Animation::GraphManager::GetSingleton()->LoadAndStartAnimation(a, args[2], "", 1.0f);
		} else if (type == "s" && args.size() > 2) {
			auto a = StrToActor(args[2]);
			if (!a) {
				return;
			}

			Animation::GraphManager::GetSingleton()->DetachGenerator(a, 1.0f);
		}
	}

	void Run(const CCF::simple_array<CCF::simple_string_view>& a_args, const char* a_fullString, CCF::ConsoleInterface* a_intfc)
	{
		args = a_args;
		fullStr = a_fullString;
		itfc = a_intfc;

		if (a_args.size() < 1) {
			ShowHelp();
			return;
		}

		auto type = args[0].get();
		if (type == "s") {
			itfc->PreventDefaultPrint();
			ProcessPapyrusCommand();
		} else if (type == "play") {
			ProcessPlayCommand();
		} else if (type == "stop") {
			ProcessStopCommand();
		} else if (type == "studio") {
			ProcessStudioCommand();
		} else {
			ShowHelp();
		}
	}
}