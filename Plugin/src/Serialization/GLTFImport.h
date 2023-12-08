#pragma once
#include "Animation/Generator.h"
#include "Settings/SkeletonDescriptor.h"

namespace Serialization
{
	class GLTFImport
	{
	public:
		enum ErrorCode : uint16_t
		{
			kSuccess = 0,
			kNoSkeleton = 1,
			kFailedToLoad = 2,
			kFailedToMakeClip = 3,
			kInvalidAnimationIdentifier = 4,
			kUnspecifiedError = 5
		};

		struct AnimationIdentifer
		{
			enum Type : uint8_t
			{
				kIndex = 0,
				kName = 1
			};

			Type type = kIndex;
			std::string name = "";
			size_t index = 0;
		};

		struct AnimationResult
		{
			std::unique_ptr<Animation::Generator> generator = nullptr;
			ErrorCode error = kUnspecifiedError;
		};

		struct AnimationInfo
		{
			RE::Actor* targetActor = nullptr;
			std::string fileName = "";
			AnimationIdentifer id = {};
			AnimationResult result;
		};

		static void LoadAnimation(AnimationInfo& info);
		static std::unique_ptr<Animation::LinearClipGenerator> CreateClipGenerator(const fastgltf::Asset* asset, const fastgltf::Animation* anim, const Settings::SkeletonDescriptor* skeleton);
		static std::unique_ptr<fastgltf::Asset> LoadGLTF(const std::filesystem::path& fileName);
	};
}