#pragma once
#include "Animation/Generator.h"
#include "Settings/SkeletonDescriptor.h"
#include "Animation/Ozz.h"

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
			ozz::unique_ptr<ozz::animation::Animation> anim = nullptr;
			ErrorCode error = kUnspecifiedError;
		};

		struct AnimationInfo
		{
			RE::Actor* targetActor = nullptr;
			std::string fileName = "";
			AnimationIdentifer id = {};
			AnimationResult result;
		};

		struct AssetData
		{
			fastgltf::Asset asset;
			std::map<size_t, std::vector<std::string>> morphTargets;
			std::map<size_t, std::string> originalNames;
		};

		static void LoadAnimation(AnimationInfo& info);
		static std::unique_ptr<std::vector<ozz::math::Transform>> CreateRawPose(const fastgltf::Asset* asset, const ozz::animation::Skeleton* skeleton);
		static bool RetargetAnimation(ozz::animation::offline::RawAnimation* anim, const std::vector<ozz::math::Transform>& sourceRestPose, const std::vector<ozz::math::Transform>& targetRestPose);
		static std::unique_ptr<Animation::RawOzzAnimation> CreateRawAnimation(const AssetData* assetData, const fastgltf::Animation* anim, const ozz::animation::Skeleton* skeleton);
		static std::unique_ptr<Animation::OzzAnimation> CreateRuntimeAnimation(const AssetData* assetData, const fastgltf::Animation* anim, const ozz::animation::Skeleton* skeleton);
		static std::unique_ptr<AssetData> LoadGLTF(const std::filesystem::path& fileName);
	};
}