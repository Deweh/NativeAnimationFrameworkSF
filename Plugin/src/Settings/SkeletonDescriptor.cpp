#include "SkeletonDescriptor.h"

namespace Settings
{
	ozz::unique_ptr<ozz::animation::Skeleton> SkeletonDescriptor::ToOzz()
	{
		ozz::animation::offline::RawSkeleton raw;
		for (auto& n : nodeNames) {
			ozz::animation::offline::RawSkeleton::Joint j;
			j.transform = ozz::math::Transform::identity();
			j.name = n;
			raw.roots.push_back(j);
		}
		ozz::animation::offline::SkeletonBuilder builder;
		return builder(raw);
	}

	void SkeletonDescriptor::MakeNodeNamesUnique()
	{
		std::unordered_set<std::string> s;
		for (auto iter = nodeNames.begin(); iter != nodeNames.end();) {
			if (!s.contains(*iter)) {
				s.insert(*iter);
				iter++;
			} else {
				iter = nodeNames.erase(iter);
			}
		}
	}

	std::optional<size_t> SkeletonDescriptor::GetNodeIndex(const std::string& name) const
	{
		for (size_t i = 0; i < nodeNames.size(); i++) {
			if (nodeNames[i] == name)
				return i;
		}
		return std::nullopt;
	}

	std::map<std::string, size_t> SkeletonDescriptor::GetNodeIndexMap() const
	{
		std::map<std::string, size_t> result;
		for (size_t i = 0; i < nodeNames.size(); i++) {
			result[nodeNames[i]] = i;
		}
		return result;
	}
}