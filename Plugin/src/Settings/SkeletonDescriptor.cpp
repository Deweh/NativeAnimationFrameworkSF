#include "SkeletonDescriptor.h"

namespace Settings
{
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