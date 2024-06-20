#pragma once

namespace Animation
{
	struct RawOzzFaceAnimation
	{
		std::array<ozz::animation::offline::RawFloatTrack, RE::BSFaceGenAnimationData::morphSize> tracks;
		float duration;
	};

	struct RawOzzAnimation
	{
		ozz::unique_ptr<ozz::animation::offline::RawAnimation> data = nullptr;
		std::unique_ptr<RawOzzFaceAnimation> faceData = nullptr;
	};

	struct OzzFaceAnimation
	{
		std::array<ozz::animation::FloatTrack, RE::BSFaceGenAnimationData::morphSize> tracks;
		float duration;
	};

	struct OzzAnimation
	{
		ozz::unique_ptr<ozz::animation::Animation> data = nullptr;
		std::unique_ptr<OzzFaceAnimation> faceData = nullptr;

		~OzzAnimation() noexcept;
	};

	struct OzzSkeleton
	{
		ozz::unique_ptr<ozz::animation::Skeleton> data = nullptr;
	};
}