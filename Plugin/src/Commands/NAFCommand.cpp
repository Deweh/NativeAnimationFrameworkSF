#include "NAFCommand.h"
#include "Serialization/GLTFImport.h"
#include "Serialization/GLTFExport.h"
#include "Settings/Settings.h"
#include "Animation/GraphManager.h"
#include "Animation/Ozz.h"
#include "Util/String.h"
#include "zstr.hpp"
#include "Tasks/Input.h"

namespace Commands::NAFCommand
{
	CCF::ConsoleInterface* itfc;
	CCF::simple_array<CCF::simple_string_view> args;
	const char* fullStr = nullptr;
	RE::Actor* lastActor = nullptr;
	std::filesystem::path lastFile;

	void SetLastAnimInfo(std::string_view file, RE::Actor* actor)
	{
		lastActor = actor;
		lastFile = Util::String::GetDataPath() / file;
	}

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

	RE::Actor* ActorStrOrSelection(size_t argPos, bool verbose = false)
	{
		if (args.size() > argPos) {
			return StrToActor(args[argPos], verbose);
		} else {
			auto refr = itfc->GetSelectedReference();
			if (refr.get() == nullptr) {
				if (verbose)
					ShowNoActor();
				return nullptr;
			}
			RE::Actor* result = starfield_cast<RE::Actor*>(refr.get());
			if (!result && verbose) {
				ShowNoActor();
			}
			return result;
		}
	}

	void ProcessPlayCommand(uint64_t idxStart = 1, bool verbose = true)
	{
		if (args.size() < (idxStart + 1)) {
			ShowHelp();
			return;
		}

		RE::Actor* actor = ActorStrOrSelection(idxStart + 1, verbose);
		if (!actor) {
			return;
		}

		Animation::GraphManager::GetSingleton()->LoadAndStartAnimation(actor, args[idxStart]);
		SetLastAnimInfo(args[idxStart], actor);
		if (verbose)
			itfc->PrintLn("Starting animation...");
	}

	void ProcessStopCommand(uint64_t idxStart = 1, bool verbose = true)
	{
		RE::Actor* actor = ActorStrOrSelection(idxStart, verbose);
		if (!actor) {
			return;
		}

		Animation::GraphManager::GetSingleton()->DetachGenerator(actor, 1.0f);
		if (verbose)
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

	void ProcessSyncCommand(uint64_t idxStart = 1)
	{
		if (args.size() < (idxStart + 2)) {
			return;
		}

		std::vector<RE::Actor*> vec;
		for (size_t i = idxStart; i < args.size(); i++) {
			auto frm = itfc->HexStrToForm(args[i]);
			if (!frm)
				continue;

			auto actor = starfield_cast<RE::Actor*>(frm);
			if (!actor)
				continue;

			vec.push_back(actor);
		}

		Animation::GraphManager::GetSingleton()->SyncGraphs(vec);
	}

	void ProcessStopSyncCommand(uint64_t idxStart = 1, bool verbose = true)
	{
		auto actor = ActorStrOrSelection(idxStart, verbose);
		if (!actor)
			return;

		if (actor == lastActor) {
			lastActor = nullptr;
		}

		Animation::GraphManager::GetSingleton()->StopSyncing(actor);
	}

	void ProcessOptimizeCommand(uint64_t idxStart = 1, bool verbose = true)
	{
		if (args.size() < idxStart + 1) {
			return;
		}

		auto actor = ActorStrOrSelection(idxStart + 1, verbose);
		if (!actor)
			return;

		auto skele = Settings::GetSkeleton(actor);
		if (Settings::IsDefaultSkeleton(skele)) {
			if (verbose)
				itfc->PrintLn("No skeleton for provided actor.");
			return;
		}

		std::string filePath = (Util::String::GetDataPath() / args[idxStart].get()).generic_string();
		auto baseFile = Serialization::GLTFImport::LoadGLTF(filePath);
		if (!baseFile || baseFile->asset.animations.empty()) {
			if (verbose)
				itfc->PrintLn("Failed to load file.");
			return;
		}

		auto rawAnim = Serialization::GLTFImport::CreateRawAnimation(baseFile.get(), &baseFile->asset.animations[0], skele->data.get());
		if (!rawAnim) {
			if (verbose)
				itfc->PrintLn("Failed to load anim.");
			return;
		}

		baseFile.reset();
		auto optimizedAsset = Serialization::GLTFExport::CreateOptimizedAsset(rawAnim.get(), skele->data.get());

		try {
			zstr::ofstream file(filePath, std::ios::binary);
			file.write(reinterpret_cast<char*>(optimizedAsset.data()), optimizedAsset.size());
		}
		catch (const std::exception&) {
			if (verbose)
				itfc->PrintLn("Failed to save file.");
			return;
		}

		if (verbose)
			itfc->PrintLn("Done.");
	}

	void ProcessSilentCommand()
	{
		if (args.size() < 2) {
			return;
		}

		auto type = args[1].get();
		if (type == "play") {
			ProcessPlayCommand(2, false);
		} else if (type == "stop") {
			ProcessStopCommand(2, false);
		} else if (type == "sync") {
			ProcessSyncCommand(2);
		} else if (type == "stopsync") {
			ProcessStopSyncCommand(2, false);
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
			ProcessSilentCommand();
		} else if (type == "play") {
			ProcessPlayCommand();
		} else if (type == "stop") {
			ProcessStopCommand();
		} else if (type == "studio") {
			ProcessStudioCommand();
		} else if (type == "sync") {
			ProcessSyncCommand();
		} else if (type == "stopsync") {
			ProcessStopSyncCommand();
		} else if (type == "optimize") {
			ProcessOptimizeCommand();	
		} else {
			ShowHelp();
		}
	}

	void ScrollAnims(Tasks::Input::BS_BUTTON_CODE a_key, bool a_down)
	{
		if (!a_down || lastFile.empty() || lastActor == nullptr)
			return;

		std::vector<std::filesystem::path> files;
		try {
			std::string ext;

			for (auto& f : std::filesystem::directory_iterator(std::filesystem::path(lastFile).parent_path())) {
				ext = Util::String::ToLower(f.path().extension().generic_string());

				if (ext != ".gltf" && ext != ".glb") {
					continue;
				}

				files.push_back(f.path());
			}
		}
		catch (const std::exception&) {
		}

		if (files.size() < 2)
			return;

		uint64_t currentIdx = UINT64_MAX;
		for (size_t i = 0; i < files.size(); i++) {
			if (std::filesystem::equivalent(files[i], lastFile)) {
				currentIdx = i;
				break;
			}
		}

		auto gm = Animation::GraphManager::GetSingleton();

		const auto StartFile = [](const std::filesystem::path& a_file) {
			Animation::GraphManager::GetSingleton()->LoadAndStartAnimation(lastActor, a_file.lexically_relative(Util::String::GetDataPath()).generic_string());
			lastFile = a_file;
		};
		
		if (currentIdx == UINT64_MAX) {
			StartFile(files[0]);
			return;
		}

		if (a_key == Tasks::Input::BS_BUTTON_CODE::kDown) {
			if (currentIdx >= (files.size() - 1)) {
				StartFile(files[0]);
			} else {
				StartFile(files[currentIdx + 1]);
			}
		} else {
			if (currentIdx == 0) {
				StartFile(files.back());
			} else {
				StartFile(files[currentIdx - 1]);
			}
		}
	}

	void RegisterKeybinds()
	{
		auto im = Tasks::Input::GetSingleton();
		im->RegisterForKey(Tasks::Input::BS_BUTTON_CODE::kDown, &ScrollAnims);
		im->RegisterForKey(Tasks::Input::BS_BUTTON_CODE::kUp, &ScrollAnims);
	}
}