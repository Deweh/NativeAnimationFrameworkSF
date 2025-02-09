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
		itfc->PrintLn("naf play <file_path> <optional: actor_form_id>");
		itfc->PrintLn("naf playk <file_path> <optional: actor_form_id>");
		itfc->PrintLn("naf stop <optional: actor_form_id>");
		itfc->PrintLn("naf sync <actor_form_id> <actor_form_id> <any # more...>");
		itfc->PrintLn("naf stopsync <optional: actor_form_id>");
		itfc->PrintLn("naf optimize <file_path> <optional: actor_form_id>");
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

	void ProcessPlayCommand(uint64_t idxStart = 1, bool verbose = true, bool initKeys = false)
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
		if (initKeys)
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

		if (actor == lastActor) {
			lastActor = nullptr;
			lastFile.clear();
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

		Animation::GraphManager::GetSingleton()->StopSyncing(actor);
	}

	bool DoOptimize(const std::string_view filePath, const std::string& savePath, uint8_t compressLevel, const Animation::OzzSkeleton* skele, bool verbose = true) {
		auto baseFile = Serialization::GLTFImport::LoadGLTF(filePath);
		if (!baseFile || baseFile->asset.animations.empty()) {
			if (verbose)
				itfc->PrintLn("Failed to load file.");
			return false;
		}

		auto rawAnim = Serialization::GLTFImport::CreateRawAnimation(baseFile.get(), &baseFile->asset.animations[0], skele);
		if (!rawAnim) {
			if (verbose)
				itfc->PrintLn("Failed to load anim.");
			return false;
		}

		baseFile.reset();
		auto optimizedAsset = Serialization::GLTFExport::CreateOptimizedAsset(rawAnim.get(), skele->data.get(), compressLevel);

		try {
			zstr::ofstream file(savePath, std::ios::binary);
			file.write(reinterpret_cast<char*>(optimizedAsset.data()), optimizedAsset.size());
		} catch (const std::exception&) {
			if (verbose)
				itfc->PrintLn("Failed to save file.");
			return false;
		}

		return true;
	}

	void ProcessOptimizeCommand(uint64_t idxStart = 1, bool verbose = true)
	{
		if (args.size() < idxStart + 2) {
			return;
		}

		auto actor = ActorStrOrSelection(idxStart + 2, verbose);
		if (!actor)
			return;

		auto skele = Settings::GetSkeleton(actor);
		if (Settings::IsDefaultSkeleton(skele)) {
			if (verbose)
				itfc->PrintLn("No skeleton for provided actor.");
			return;
		}

		std::string filePath = (Util::String::GetDataPath() / args[idxStart].get()).generic_string();
		
		if (args[idxStart + 1].get() == "test") {
			std::string savePath = std::filesystem::path(filePath).replace_extension().generic_string();

			for (uint8_t i = 0; i < 5; i++) {
				if (!DoOptimize(filePath, std::format("{}_{}.glb", savePath, i), i, skele.get(), true)) {
					break;
				}
			}
		} else {
			auto arg2Int = Util::String::StrToInt(std::string(args[idxStart + 1]));
			int compressLevel = arg2Int.has_value() ? std::clamp(arg2Int.value(), 0, 255) : 0;
			DoOptimize(filePath, filePath, static_cast<uint8_t>(compressLevel), skele.get(), true);
		}
		

		if (verbose)
			itfc->PrintLn("Done.");
	}

	void ProcessStartSeqCommand(uint64_t idxStart = 1, bool verbose = true)
	{
		if (args.size() < idxStart + 4) {
			return;
		}

		auto actor = StrToActor(args[idxStart], verbose);
		if (!actor) {
			return;
		}

		bool loopSeq = Util::String::ToLower(args[idxStart + 1]) == "true";

		std::vector<Animation::Sequencer::PhaseData> phases;
		for (size_t i = (idxStart + 2); (i + 2) < args.size(); i += 3) {
			auto& p = phases.emplace_back();
			p.file = Animation::FileID(args[i], "");

			auto loopCount = Util::String::StrToInt(std::string(args[i + 1].get()));
			p.loopCount = loopCount.has_value() ? loopCount.value() : 0;

			auto transitionTime = Util::String::StrToFloat(std::string(args[i + 2].get()));
			p.transitionTime = transitionTime.has_value() ? transitionTime.value() : 1.0f;
		}

		Animation::GraphManager::GetSingleton()->StartSequence(actor, std::move(phases));
	}

	void ProcessAdvanceSeqCommand(uint64_t idxStart = 1, bool verbose = true)
	{
		if (args.size() < idxStart + 1) {
			return;
		}

		auto actor = ActorStrOrSelection(idxStart + 1, verbose);
		if (!actor) {
			return;
		}

		bool smooth = Util::String::ToLower(args[idxStart]) == "true";
		Animation::GraphManager::GetSingleton()->AdvanceSequence(actor, smooth);
	}

	void ProcessLockPosCommand(uint64_t idxStart = 1, bool verbose = true)
	{
		if (args.size() < idxStart + 1) {
			return;
		}

		auto actor = ActorStrOrSelection(idxStart + 1, verbose);
		if (!actor) {
			return;
		}

		bool lock = Util::String::ToLower(args[idxStart]) == "true";
		Animation::GraphManager::GetSingleton()->SetGraphControlsPosition(actor, lock);
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
		} else if (type == "startseq") {
			ProcessStartSeqCommand(2, false);
		} else if (type == "advseq") {
			ProcessAdvanceSeqCommand(2, false);
		}
	}

	void ProcessTest()
	{
		// put test routines here.
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
		} else if (type == "playk") {
			ProcessPlayCommand(1, true, true);
		} else if (type == "startseq") {
			ProcessStartSeqCommand();
		} else if (type == "advseq") {
			ProcessAdvanceSeqCommand();
		} else if (type == "lockpos") {
			ProcessLockPosCommand();
		} else if (type == "test") {
			ProcessTest();
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

		const auto StartFile = [](const std::filesystem::path& a_file) {
			const std::string relativePath = a_file.lexically_relative(Util::String::GetDataPath()).generic_string();
			RE::ConsoleLog::GetSingleton()->PrintLine(relativePath.c_str());
			Animation::GraphManager::GetSingleton()->LoadAndStartAnimation(lastActor, relativePath);
			lastFile = a_file;
		};
		
		if (currentIdx == UINT64_MAX) {
			StartFile(files[0]);
			return;
		}

		const auto ScrollFile = [&](uint64_t a_relative) {
			size_t targetIndex = (currentIdx + a_relative) % files.size();
			StartFile(files[targetIndex]);
		};

		if (a_key == Tasks::Input::BS_BUTTON_CODE::kDown) {
			ScrollFile(1);
		} else if (a_key == Tasks::Input::BS_BUTTON_CODE::kUp) {
			ScrollFile(-1);
		} else if (a_key == Tasks::Input::BS_BUTTON_CODE::kLeft) {
			ScrollFile(-10);
		} else if (a_key == Tasks::Input::BS_BUTTON_CODE::kRight) {
			ScrollFile(10);
		}
	}

	void RegisterKeybinds()
	{
		auto im = Tasks::Input::GetSingleton();
		im->RegisterForKey(Tasks::Input::BS_BUTTON_CODE::kDown, &ScrollAnims);
		im->RegisterForKey(Tasks::Input::BS_BUTTON_CODE::kUp, &ScrollAnims);
		im->RegisterForKey(Tasks::Input::BS_BUTTON_CODE::kLeft, &ScrollAnims);
		im->RegisterForKey(Tasks::Input::BS_BUTTON_CODE::kRight, &ScrollAnims);
	}
}