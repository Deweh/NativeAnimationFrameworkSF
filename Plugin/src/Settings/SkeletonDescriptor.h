#pragma once

namespace Settings
{
	struct IKChainDescriptor
	{
		std::vector<std::string> nodeNames;
		std::optional<RE::NiPoint3> poleStartPos;
		std::string poleParent;
		std::string poleNode;
		std::string effectorNode;
	};

	struct SkeletonDescriptor
	{
		std::vector<IKChainDescriptor> chains;
		std::vector<std::string> nodeNames;
		std::string raceId;

		ozz::unique_ptr<ozz::animation::Skeleton> ToOzz();
		void MakeNodeNamesUnique();
		std::optional<size_t> GetNodeIndex(const std::string& name) const;
		std::map<std::string, size_t> GetNodeIndexMap() const;
	};
}