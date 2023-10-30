#pragma once
#include "SkeletonDescriptor.h"

namespace Settings
{
	void Load();
	std::shared_ptr<const SkeletonDescriptor> GetSkeleton(const std::string& a_raceId);
	std::shared_ptr<const SkeletonDescriptor> GetSkeleton(RE::Actor* a_actor);
	bool IsDefaultSkeleton(std::shared_ptr<const SkeletonDescriptor> a_skeleton);
}