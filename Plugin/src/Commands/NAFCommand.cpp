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
		log->Print("naf play [anim-file]");
		log->Print("naf retarget [src-pose-file] [dest-pose-file] [anim-file]");
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
		/*
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
		*/
		Animation::GraphManager::GetSingleton()->LoadAndStartAnimation(actor, args[2]);
		log->Print("Starting animation...");
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

	void ProcessStudioCommand(RE::ConsoleLog* log)
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

	void ProcessRetargetCommand(const std::vector<std::string_view>& args, RE::ConsoleLog* log, RE::TESObjectREFR* refr)
	{
		if (args.size() < 5) {
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

		auto skeleton = Settings::GetSkeleton(actor);
		if (Settings::IsDefaultSkeleton(skeleton)) {
			log->Print("No configured skeleton for selected actor's race.");
			return;
		}

		static const auto LoadPose = [](const std::string_view& path, const ozz::animation::Skeleton* skeleton) {
			std::unique_ptr<std::vector<ozz::math::Transform>> result = nullptr;
			auto file = Serialization::GLTFImport::LoadGLTF(path);
			if (!file) {
				return result;
			}

			result = Serialization::GLTFImport::CreateRawPose(file.get(), skeleton);
			return result;
		};

		using clock = std::chrono::high_resolution_clock;
		auto start = clock::now();

		auto srcBindPose = LoadPose(args[2], skeleton->data.get());
		if (!srcBindPose)
		{
			log->Print("Failed to load source pose GLTF/GLB.");
			return;
		}

		auto destBindPose = LoadPose(args[3], skeleton->data.get());
		if (!srcBindPose) {
			log->Print("Failed to load destination pose GLTF/GLB.");
			return;
		}

		std::unique_ptr<ozz::animation::offline::RawAnimation> targetAnim = nullptr;
		{
			auto animFile = Serialization::GLTFImport::LoadGLTF(args[4]);
			if (!animFile || animFile->animations.empty()) {
				log->Print("Failed to load animation GLTF/GLB.");
				return;
			}

			targetAnim = Serialization::GLTFImport::CreateRawAnimation(animFile.get(), &animFile->animations[0], skeleton->data.get());
		}
		
		if (!targetAnim) {
			log->Print("Failed to process animation GLTF/GLB.");
			return;
		}
		
		ozz::animation::offline::RawAnimation rawResult;
		{
			ozz::animation::offline::AdditiveAnimationBuilder builder;
			builder(*targetAnim, ozz::make_span(*srcBindPose), &rawResult);
		}

		auto sharedAnim = std::make_shared<Animation::OzzAnimation>();
		{
			ozz::animation::offline::AnimationBuilder builder;
			sharedAnim->data = builder(rawResult);
		}

		std::vector<ozz::math::SoaTransform> finalRestPose;
		finalRestPose.resize(skeleton->data->num_soa_joints());
		Animation::Transform::StoreSoaTransforms(finalRestPose, [&destBindPose](size_t i) {
			if (i < destBindPose->size()) {
				return Animation::Transform{ destBindPose->at(i) };
			}
			return Animation::Transform();
		});

		auto gen = std::make_unique<Animation::AdditiveGenerator>();
		gen->SetRestPose(finalRestPose);
		gen->baseGen = Animation::GraphManager::CreateAnimationGenerator(sharedAnim);

		Animation::GraphManager::GetSingleton()->AttachGenerator(
			actor,
			std::move(gen),
			1.0f
		);
		log->Print(std::format("Retargeting finished in {:.3f}ms, playing animation...", std::chrono::duration<double>(clock::now() - start).count() * 1000).c_str());
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
			ProcessStudioCommand(log);
		} else if (args[1] == "retarget") {
			ProcessRetargetCommand(args, log, refr);
		} else {
			ShowHelp(log);
		}
	}
}