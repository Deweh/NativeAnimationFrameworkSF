#pragma once

namespace Animation
{
	struct OzzAnimation
	{
		ozz::unique_ptr<ozz::animation::Animation> data = nullptr;

		~OzzAnimation() noexcept;
	};

	struct OzzSkeleton
	{
		ozz::unique_ptr<ozz::animation::Skeleton> data = nullptr;
	};
}