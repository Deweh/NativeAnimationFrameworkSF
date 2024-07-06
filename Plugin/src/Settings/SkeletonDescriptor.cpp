#include "SkeletonDescriptor.h"

namespace Settings
{
	RE::BGSFadeNode* GetSkeletonModel(const std::string_view raceName)
	{
		auto race = RE::TESForm::LookupByEditorID<RE::TESRace>(raceName);
		if (!race)
			return nullptr;

		auto entry = RE::ModelDB::GetEntry(race->unk5E8[0].model.c_str());
		if (!entry || !entry->node)
			return nullptr;

		return entry->node;
	}

	bool SkeletonDescriptor::FillInGameData(const std::string_view raceName)
	{
		parents.resize(nodeNames.size(), UINT64_MAX);
		restPose.resize(nodeNames.size(), ozz::math::Transform::identity());

		auto m = GetSkeletonModel(raceName);
		if (!m)
			return false;

		auto idxMap = GetNodeIndexMap();
		for (auto iter = nodeNames.begin(); iter != nodeNames.end(); iter++) {
			auto gameNode = m->GetObjectByName(*iter);
			if (!gameNode)
				continue;

			auto curIdx = std::distance(nodeNames.begin(), iter);
			if (gameNode->parent) {
				if (auto iter = idxMap.find(gameNode->parent->name.c_str()); iter != idxMap.end()) {
					parents[curIdx] = iter->second;
				}
			}

			RE::NiQuaternion rotQuat(gameNode->local.rotate);
			restPose[curIdx].rotation = { rotQuat.x, rotQuat.y, rotQuat.z, rotQuat.w };
			restPose[curIdx].translation = { gameNode->local.translate.x, gameNode->local.translate.y, gameNode->local.translate.z };
		}

		return true;
	}

	ozz::unique_ptr<ozz::animation::Skeleton> SkeletonDescriptor::ToOzz()
	{
		using namespace ozz::animation::offline;

		struct PointerJoint
		{
			RawSkeleton::Joint joint;
			std::vector<PointerJoint*> children;
		};

		std::vector<std::unique_ptr<PointerJoint>> flatJoints;
		flatJoints.reserve(nodeNames.size());

		//Pass 1: Put all joints into flat vector.
		for (size_t i = 0; i < nodeNames.size(); i++) {
			auto& j = flatJoints.emplace_back(std::make_unique<PointerJoint>());
			j->joint.transform = restPose[i];
			j->joint.name = nodeNames[i];
		}

		//Pass 2: Assign children to parents.
		for (size_t i = 0; i < nodeNames.size(); i++) {
			auto p = parents[i];
			if (p == UINT64_MAX)
				continue;

			flatJoints[p]->children.push_back(flatJoints[i].get());
		}

		//Pass 3: Fill in real skeleton structure.
		std::function<void(RawSkeleton::Joint & j, PointerJoint * pj)> FillTree = [&FillTree](RawSkeleton::Joint& j, PointerJoint* pj) -> void {
			for (auto& c : pj->children) {
				auto& newJ = j.children.emplace_back(c->joint);
				FillTree(newJ, c);
			}
		};

		RawSkeleton raw;
		for (size_t i = 0; i < nodeNames.size(); i++) {
			//We want to start with roots, nodes that have no parent.
			if (parents[i] != UINT64_MAX)
				continue;

			FillTree(raw.roots.emplace_back(flatJoints[i]->joint), flatJoints[i].get());
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