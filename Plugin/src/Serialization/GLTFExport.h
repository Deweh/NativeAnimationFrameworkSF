#pragma once
#include "Animation/Ozz.h"

namespace Serialization
{
	class GLTFExport
	{
	public:
		static std::unique_ptr<fastgltf::Asset> CreateOptimizedAsset(Animation::RawOzzAnimation* anim, const ozz::animation::Skeleton* skeleton);
	};
}