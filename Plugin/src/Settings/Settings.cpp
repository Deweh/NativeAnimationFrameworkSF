#include "Settings.h"
#include "simdjson.h"
#include "Util/String.h"

namespace Settings
{
	const std::filesystem::path skeletonsDir = Util::String::GetDataPath() / "Skeletons";
	std::mutex lock;

	std::shared_ptr<Animation::OzzSkeleton> GetDefaultSkeleton()
	{
		static std::shared_ptr<Animation::OzzSkeleton> defaultSkeleton = std::make_shared<Animation::OzzSkeleton>();
		return defaultSkeleton;
	}

	std::unordered_map<std::string, std::shared_ptr<Animation::OzzSkeleton>>& GetSkeletonMap()
	{
		static std::unordered_map<std::string, std::shared_ptr<Animation::OzzSkeleton>> skeletons;
		return skeletons;
	}

	void InitDefaultSkeleton()
	{
		SkeletonDescriptor skele;
		skele.nodeNames.push_back("COM");
		auto d = skele.ToOzz();
		GetDefaultSkeleton()->data = std::move(d);
	}

	void Load()
	{
		InitDefaultSkeleton();

		ozz::animation::offline::SkeletonBuilder skeletonBuilder;
		simdjson::ondemand::parser parser;
		auto& skeletons = GetSkeletonMap();
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
		auto& skeletons = GetSkeletonMap();
		if (auto iter = skeletons.find(a_raceId); iter != skeletons.end()) {
			return iter->second;
		} else {
			return GetDefaultSkeleton();
		}
	}

	std::shared_ptr<const Animation::OzzSkeleton> GetSkeleton(RE::Actor* a_actor)
	{
		if (!a_actor)
			return GetDefaultSkeleton();

		return GetSkeleton(a_actor->race->formEditorID.c_str());
	}

	bool IsDefaultSkeleton(std::shared_ptr<const Animation::OzzSkeleton> a_skeleton)
	{
		return a_skeleton.get() == GetDefaultSkeleton().get();
	}

	struct PerformanceMorph
	{
		RE::BSFixedString name;
		bool someFlag;
		uint8_t pad09;
		uint16_t pad0A;
		uint32_t pad0C;
	};

	PerformanceMorph* GetMorphNames()
	{
		REL::Relocation<PerformanceMorph*> ptr(REL::ID(872948));
		return ptr.get();
	}

	const std::map<std::string, size_t>& GetFaceMorphIndexMap()
	{
		static std::map<std::string, size_t> idxMap;
		if (idxMap.empty()) {
			auto names = GetMorphNames();
			for (size_t i = 0; i < RE::BSFaceGenAnimationData::morphSize; i++) {
				idxMap[names[i].name.c_str()] = i;
			}
		}
		return idxMap;
	}
}