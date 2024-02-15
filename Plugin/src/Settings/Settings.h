#pragma once
#include "SkeletonDescriptor.h"
#include "Animation/Ozz.h"

namespace Settings
{
	void Load();
	std::shared_ptr<const Animation::OzzSkeleton> GetSkeleton(const std::string& a_raceId);
	std::shared_ptr<const Animation::OzzSkeleton> GetSkeleton(RE::Actor* a_actor);
	bool IsDefaultSkeleton(std::shared_ptr<const Animation::OzzSkeleton> a_skeleton);
}