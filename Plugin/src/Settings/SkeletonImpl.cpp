#include "SkeletonImpl.h"
#include "Settings/Settings.h"
#include "simdjson.h"
#include "Util/String.h"

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

	bool FillInSkeletonNIFData(SkeletonDescriptor& a_desc, const std::string_view raceName)
	{
		auto m = GetSkeletonModel(raceName);
		if (!m)
			return false;

		for (auto iter = a_desc.bones.begin(); iter != a_desc.bones.end(); iter++) {
			auto gameNode = m->GetObjectByName(iter->name);
			if (!gameNode)
				continue;

			if (gameNode->parent) {
				iter->parent = gameNode->parent->name;
			}

			RE::NiQuaternion rotQuat(gameNode->local.rotate);
			iter->restPose.rotation = { rotQuat.x, rotQuat.y, rotQuat.z, rotQuat.w };
			iter->restPose.translation = { gameNode->local.translate.x, gameNode->local.translate.y, gameNode->local.translate.z };
		}

		return true;
	}

	void LoadBaseSkeletons()
	{
		ozz::animation::offline::SkeletonBuilder skeletonBuilder;
		simdjson::ondemand::parser parser;
		auto& skeletons = Settings::GetSkeletonMap();
		for (auto& f : std::filesystem::directory_iterator(Settings::GetSkeletonsPath())) {
			if (auto p = f.path(); f.exists() && !f.is_directory() && p.has_extension() && p.extension() == ".json") {
				Settings::SkeletonDescriptor skele;
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
						skele.AddBone(n.get_string().value(), "", ozz::math::Transform::identity());
					}
					auto sName = p.stem().generic_string();
					FillInSkeletonNIFData(skele, sName);

					std::unique_ptr<Animation::OzzSkeleton> runtimeSkele = skele.BuildRuntime(sName);
					if (runtimeSkele.get() != nullptr) {
						skeletons[sName] = std::move(runtimeSkele);
						logger::info("Loaded {} skeleton.", sName);
					} else {
						logger::info("Failed to load {} skeleton.", sName);
					}
				} catch (const std::exception&) {
					continue;
				}
			}
		}
	}
}