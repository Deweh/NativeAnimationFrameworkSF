#pragma once
#include "FileID.h"
#include "IAnimationFile.h"

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

	struct OzzAnimation : public IAnimationFile
	{
		ozz::unique_ptr<ozz::animation::Animation> data = nullptr;
		std::unique_ptr<OzzFaceAnimation> faceData = nullptr;

		size_t GetSize();
		virtual std::unique_ptr<Generator> CreateGenerator() override;
	};

	struct OzzSkeleton
	{
		ozz::unique_ptr<ozz::animation::Skeleton> data = nullptr;
		size_t lEyeIdx = UINT64_MAX;
		size_t rEyeIdx = UINT64_MAX;
		std::string name;

		std::unique_ptr<std::vector<ozz::math::Transform>> GetRestPose() const;
	};
}