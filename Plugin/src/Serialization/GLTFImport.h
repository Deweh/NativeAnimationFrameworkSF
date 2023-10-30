#pragma once
#include "Animation/Generator.h"
#include "Settings/SkeletonDescriptor.h"

namespace Serialization
{
	class GLTFImport
	{
	public:
		static std::unique_ptr<Animation::LinearClipGenerator> CreateClipGenerator(const fastgltf::Asset* asset, const fastgltf::Animation* anim, const Settings::SkeletonDescriptor* skeleton);
		static std::unique_ptr<fastgltf::Asset> LoadGLTF(const std::filesystem::path& fileName);
	};
}