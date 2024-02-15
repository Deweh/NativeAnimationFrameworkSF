#pragma once

namespace Animation
{
	struct OzzAnimation
	{
		ozz::unique_ptr<ozz::animation::Animation> data = nullptr;
	};

	struct OzzSkeleton
	{
		ozz::unique_ptr<ozz::animation::Skeleton> data = nullptr;
	};
}