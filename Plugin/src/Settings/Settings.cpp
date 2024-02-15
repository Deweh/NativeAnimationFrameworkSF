#include "Settings.h"
#include "simdjson.h"
#include "Util/String.h"

namespace Settings
{
	const std::filesystem::path skeletonsDir = Util::String::GetDataPath() / "Skeletons";

	std::shared_ptr<Animation::OzzSkeleton> defaultSkeleton;

	std::mutex lock;
	std::unordered_map<std::string, std::shared_ptr<Animation::OzzSkeleton>> skeletons;

	void InitDefaultSkeleton()
	{
		SkeletonDescriptor skele;
		skele.nodeNames.push_back("COM");
		defaultSkeleton = std::make_shared<Animation::OzzSkeleton>();
		defaultSkeleton->data = skele.ToOzz();
	}

	void Load()
	{
		InitDefaultSkeleton();

		ozz::animation::offline::SkeletonBuilder skeletonBuilder;
		simdjson::ondemand::parser parser;
		for (auto& f : std::filesystem::directory_iterator(skeletonsDir)) {
			if (auto p = f.path(); f.exists() && !f.is_directory() && p.has_extension() && p.extension() == ".json") {
				SkeletonDescriptor skele;
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
						skele.nodeNames.emplace_back(n.get_string().value());
					}
					skele.MakeNodeNamesUnique();
					auto sName = p.stem().generic_string();
					auto sharedSkele = std::make_shared<Animation::OzzSkeleton>();
					sharedSkele->data = skele.ToOzz();
					skeletons[sName] = sharedSkele;
					INFO("Loaded {} skeleton.", sName);
				}
				catch (const std::exception&) {
					continue;
				}
			}
		}
	}

	std::shared_ptr<const Animation::OzzSkeleton> GetSkeleton(const std::string& a_raceId)
	{
		std::unique_lock l{ lock };
		if (auto iter = skeletons.find(a_raceId); iter != skeletons.end()) {
			return iter->second;
		} else {
			return defaultSkeleton;
		}
	}

	std::shared_ptr<const Animation::OzzSkeleton> GetSkeleton(RE::Actor* a_actor)
	{
		if (!a_actor)
			return defaultSkeleton;

		return GetSkeleton(a_actor->race->formEditorID.c_str());
	}

	bool IsDefaultSkeleton(std::shared_ptr<const Animation::OzzSkeleton> a_skeleton)
	{
		return a_skeleton.get() == defaultSkeleton.get();
	}
}