#pragma once
#include "Animation/Ozz.h"

namespace Serialization
{
	class GLTFExport
	{
	public:
		static std::vector<std::byte> CreateOptimizedAsset(Animation::RawOzzAnimation* anim, const ozz::animation::Skeleton* skeleton, uint8_t level = 0);
	};
}