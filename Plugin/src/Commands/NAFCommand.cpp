#include "NAFCommand.h"
#include "Serialization/GLTFImport.h"
#include "Settings/Settings.h"
#include "Animation/GraphManager.h"
#include "Animation/Ozz.h"

namespace Commands::NAFCommand
{
	void ShowHelp(RE::ConsoleLog* log)
	{
		log->Print("Usage:");
		log->Print("naf play [file-path]");
		log->Print("naf stop");
		log->Print("naf studio");
	}

	void ShowNoActor(RE::ConsoleLog* log)
	{
		log->Print("No actor selected.");
	}

	void ProcessPlayCommand(const std::vector<std::string_view>& args, RE::ConsoleLog* log, RE::TESObjectREFR* refr)
	{
		if (args.size() < 3) {
			ShowHelp(log);
			return;
		}

		if (!refr) {
			ShowNoActor(log);
			return;
		}

		auto actor = starfield_cast<RE::Actor*>(refr);
		if (!actor) {
			ShowNoActor(log);
			return;
		}

		using clock = std::chrono::high_resolution_clock;
		auto start = clock::now();
		Serialization::GLTFImport::AnimationInfo animInfo{ .targetActor = actor, .fileName = std::string(args[2]) };
		Serialization::GLTFImport::LoadAnimation(animInfo);

		if (animInfo.result.error) {
			using enum Serialization::GLTFImport::ErrorCode;
			switch (animInfo.result.error) {
			case kFailedToLoad:
				log->Print("Failed to load GLTF/GLB.");
				break;
			case kFailedToMakeClip:
				log->Print("Failed to create ClipGenerator. Malformed GLTF/GLB?");
				break;
			case kInvalidAnimationIdentifier:
				log->Print("GLTF/GLB contains no animations.");
				break;
			case kNoSkeleton:
				log->Print("No configured skeleton for selected actor's race.");
				break;
			}
			return;
		}

		auto sharedAnim = std::make_shared<Animation::OzzAnimation>();
		sharedAnim->data = std::move(animInfo.result.anim);

		Animation::GraphManager::GetSingleton()->AttachGenerator(
			actor,
			Animation::GraphManager::CreateAnimationGenerator(sharedAnim),
			1.0f
		);
		log->Print(std::format("Loaded animation in {:.3f}ms", std::chrono::duration<double>(clock::now() - start).count() * 1000).c_str());
	}

	void ProcessStopCommand(RE::ConsoleLog* log, RE::TESObjectREFR* refr)
	{
		if (!refr) {
			ShowNoActor(log);
			return;
		}

		auto actor = starfield_cast<RE::Actor*>(refr);
		if (!actor) {
			ShowNoActor(log);
			return;
		}

		Animation::GraphManager::GetSingleton()->DetachGenerator(actor, 1.0f);
		log->Print("Stopping animation...");
	}

	void ProcessStudio(RE::ConsoleLog* log)
	{
		const auto hndl = GetModuleHandleA("NAFStudio.dll");
		if (hndl != NULL) {
			const auto addr = GetProcAddress(hndl, "OpenStudio");
			if (addr != NULL) {
				(reinterpret_cast<void(*)()>(addr))();
			} else {
				log->Print("Failed to open NAF Studio.");
			}
		} else {
			log->Print("NAF Studio is not installed.");
		}
	}

	void Run(const std::vector<std::string_view>& args, const std::string_view& fullStr, RE::TESObjectREFR* refr)
	{
		auto log = RE::ConsoleLog::GetSingleton();
		log->Print(fullStr.data());
		if (args.size() < 2) {
			ShowHelp(log);
			return;
		}

		if (args[1] == "play") {
			ProcessPlayCommand(args, log, refr);
		} else if (args[1] == "stop") {
			ProcessStopCommand(log, refr);
		} else if (args[1] == "studio") {
			ProcessStudio(log);
		} else {
			ShowHelp(log);
		}
	}
}