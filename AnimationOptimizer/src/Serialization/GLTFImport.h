#pragma once
#include "Animation/Ozz.h"

namespace Serialization
{
	class GLTFImport
	{
	public:
		struct AssetData
		{
			fastgltf::Asset asset;
			std::map<size_t, std::vector<std::string>> morphTargets;
			std::map<size_t, std::string> originalNames;
		};
		
		static ozz::unique_ptr<ozz::animation::Skeleton> BuildSkeleton(const AssetData* assetData);
		static std::unique_ptr<Animation::RawOzzAnimation> CreateRawAnimation(const AssetData* assetData, const fastgltf::Animation* anim, const ozz::animation::Skeleton* skeleton);
		static std::unique_ptr<AssetData> LoadGLTF(const std::filesystem::path& fileName);
	};
}