#include "Settings.h"
#include "simdjson.h"
#include "Util/String.h"

namespace Settings
{
	const std::filesystem::path skeletonsDir = Util::String::GetDataPath() / "Skeletons";

	std::shared_ptr<SkeletonDescriptor> defaultSkeleton = std::make_shared<SkeletonDescriptor>();

	std::mutex lock;
	std::unordered_map<std::string, std::shared_ptr<SkeletonDescriptor>> skeletons;

	void Load()
	{
		simdjson::ondemand::parser parser;
		for (auto& f : std::filesystem::directory_iterator(skeletonsDir)) {
			if (auto p = f.path(); f.exists() && !f.is_directory() && p.has_extension() && p.extension() == ".json") {
				auto skele = std::make_shared<SkeletonDescriptor>();
				try {
					auto res = simdjson::padded_string::load(p.generic_string());
					if (res.error() != simdjson::error_code::SUCCESS) {
						continue;
					}
					simdjson::ondemand::document doc = parser.iterate(res.value());
					if (!doc.is_alive()) {
						continue;
					}
					auto nodes = doc["nodes"].get_array();
					for (auto n : nodes) {
						skele->nodeNames.emplace_back(n.get_string().value());
					}
					skele->MakeNodeNamesUnique();
					auto sName = p.stem().generic_string();
					skeletons[sName] = skele;
					INFO("Loaded {} skeleton.", sName);
				}
				catch (const std::exception&) {
					continue;
				}
			}
		}
	}

	std::shared_ptr<const SkeletonDescriptor> GetSkeleton(const std::string& a_raceId) {
		std::unique_lock l{ lock };
		if (auto iter = skeletons.find(a_raceId); iter != skeletons.end()) {
			return iter->second;
		} else {
			return defaultSkeleton;
		}
	}

	std::shared_ptr<const SkeletonDescriptor> GetSkeleton(RE::Actor* a_actor)
	{
		if (!a_actor)
			return defaultSkeleton;

		return GetSkeleton(a_actor->race->formEditorID.c_str());
	}

	bool IsDefaultSkeleton(std::shared_ptr<const SkeletonDescriptor> a_skeleton)
	{
		return a_skeleton.get() == defaultSkeleton.get();
	}
}