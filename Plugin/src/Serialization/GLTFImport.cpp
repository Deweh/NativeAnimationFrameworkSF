#include "GLTFImport.h"
#include "Animation/Transform.h"
#include "Settings/Settings.h"
#include "Util/String.h"
#include "zstr.hpp"
#include "simdjson.h"

template <>
struct fastgltf::ElementTraits<ozz::math::Quaternion> : fastgltf::ElementTraitsBase<ozz::math::Quaternion, AccessorType::Vec4, float>
{};

template <>
struct fastgltf::ElementTraits<ozz::math::Float3> : fastgltf::ElementTraitsBase<ozz::math::Float3, AccessorType::Vec3, float>
{};

namespace Serialization
{
	void GLTFImport::LoadAnimation(AnimationInfo& info)
	{
		if (!info.targetActor) {
			info.result.error = kNoSkeleton;
			return;
		}

		auto skeleton = Settings::GetSkeleton(info.targetActor);
		if (Settings::IsDefaultSkeleton(skeleton)) {
			info.result.error = kNoSkeleton;
			return;
		}

		auto assetData = LoadGLTF(Util::String::GetDataPath() / info.fileName);
		if (!assetData) {
			info.result.error = kFailedToLoad;
			return;
		}

		auto asset = &assetData->asset;

		fastgltf::Animation* anim = nullptr;
		if (info.id.type == AnimationIdentifer::Type::kIndex && info.id.index < asset->animations.size()) {
			anim = &asset->animations[info.id.index];
		} else if (info.id.type == AnimationIdentifer::Type::kName) {
			for (auto& a : asset->animations) {
				if (a.name == info.id.name.c_str()) {
					anim = &a;
					break;
				}
			}
		}

		if (!anim) {
			info.result.error = kInvalidAnimationIdentifier;
			return;
		}

		auto runtimeAnim = CreateRuntimeAnimation(assetData.get(), anim, skeleton->data.get());
		if (!runtimeAnim) {
			info.result.error = kFailedToMakeClip;
			return;
		}

		info.result.anim = std::move(runtimeAnim->data);
		info.result.error = kSuccess;
	}

	std::unique_ptr<std::vector<ozz::math::Transform>> GLTFImport::CreateRawPose(const fastgltf::Asset* asset, const ozz::animation::Skeleton* skeleton)
	{
		//Create a map of GLTF node indexes -> skeleton indexes
		std::map<std::string, size_t> skeletonMap;
		auto skeletonJointNames = skeleton->joint_names();
		for (size_t i = 0; i < skeletonJointNames.size(); i++) {
			skeletonMap[skeletonJointNames[i]] = i;
		}

		ozz::math::Transform identity;
		identity.rotation = { .0f, .0f, .0f, 1.0f };
		identity.translation = { .0f, .0f, .0f };
		auto bindPose = std::make_unique<std::vector<ozz::math::Transform>>(skeletonMap.size(), identity);

		for (const auto& n : asset->nodes) {
			if (auto iter = skeletonMap.find(n.name.c_str()); iter != skeletonMap.end()) {
				if (std::holds_alternative<fastgltf::TRS>(n.transform)) {
					auto& trs = std::get<fastgltf::TRS>(n.transform);
					auto& b = (*bindPose)[iter->second];
					b.rotation = {
						trs.rotation[0],
						trs.rotation[1],
						trs.rotation[2],
						trs.rotation[3]
					};
					b.translation = {
						trs.translation[0],
						trs.translation[1],
						trs.translation[2]
					};
					b.scale = ozz::math::Float3::one();
				}
			}
		}

		return bindPose;
	}

	void ParseMorphChannel(const GLTFImport::AssetData* assetData, const fastgltf::Animation* anim, const fastgltf::AnimationChannel& channel, Animation::RawOzzAnimation* rawAnim)
	{
		auto asset = &assetData->asset;
		auto& targetNode = asset->nodes[channel.nodeIndex.value()];

		if (!targetNode.meshIndex.has_value())
			return;

		auto iter = assetData->morphTargets.find(targetNode.meshIndex.value());
		if (iter == assetData->morphTargets.end())
			return;

		auto& morphTargets = iter->second;

		//Create a map of GLTF morph indexes -> game morph indexes
		std::vector<size_t> morphIdxs;
		auto gameIdxs = Settings::GetFaceMorphIndexMap();
		bool hasMorphs = false;

		for (auto mt : morphTargets) {
			if (auto iter = gameIdxs.find(mt); iter != gameIdxs.end()) {
				morphIdxs.push_back(iter->second);
				hasMorphs = true;
			} else {
				morphIdxs.push_back(UINT64_MAX);
			}
		}

		if (!hasMorphs)
			return;

		//Process GLTF data
		if (channel.samplerIndex > anim->samplers.size())
			return;

		auto& sampler = anim->samplers[channel.samplerIndex];
		if (sampler.inputAccessor > asset->accessors.size() || sampler.outputAccessor > asset->accessors.size())
			return;

		auto& timeAccessor = asset->accessors[sampler.inputAccessor];
		auto& dataAccessor = asset->accessors[sampler.outputAccessor];

		if (timeAccessor.count != (dataAccessor.count / morphTargets.size()))
			return;

		if (!std::holds_alternative<std::pmr::vector<double>>(timeAccessor.max))
			return;

		auto timeMax = std::get_if<std::pmr::vector<double>>(&timeAccessor.max);
		if (!timeMax || timeMax->empty())
			return;

		float duration = static_cast<float>((*timeMax)[0]);

		if (rawAnim->faceData == nullptr) {
			rawAnim->faceData = std::make_unique<Animation::RawOzzFaceAnimation>();
			rawAnim->faceData->duration = duration;
		}

		ozz::animation::offline::RawTrackKeyframe<float> kf{};
		kf.interpolation = ozz::animation::offline::RawTrackInterpolation::kLinear;
		for (size_t i = 0; i < timeAccessor.count; i++) {
			kf.ratio = (fastgltf::getAccessorElement<float>(*asset, timeAccessor, i) / duration);

			for (size_t j = 0; j < morphTargets.size(); j++) {
				auto idx = morphIdxs[j];
				if (idx == UINT64_MAX)
					continue;

				kf.value = fastgltf::getAccessorElement<float>(*asset, dataAccessor, (i * morphTargets.size()) + j);
				rawAnim->faceData->tracks[idx].keyframes.push_back(kf);
			}
		}
	}

	std::unique_ptr<Animation::RawOzzAnimation> GLTFImport::CreateRawAnimation(const AssetData* assetData, const fastgltf::Animation* anim, const ozz::animation::Skeleton* skeleton)
	{
		auto asset = &assetData->asset;

		//Create a map of GLTF node indexes -> skeleton indexes
		std::vector<size_t> skeletonIdxs;
		skeletonIdxs.reserve(asset->nodes.size());

		std::map<std::string, size_t> skeletonMap;
		auto skeletonJointNames = skeleton->joint_names();
		for (size_t i = 0; i < skeletonJointNames.size(); i++) {
			skeletonMap[skeletonJointNames[i]] = i;
		}

		ozz::math::Transform identity;
		identity.rotation = { .0f, .0f, .0f, 1.0f };
		identity.translation = { .0f, .0f, .0f };
		std::vector<ozz::math::Transform> bindPose(skeletonMap.size(), identity);

		//Save skeleton bind pose.
		for (const auto& n : asset->nodes) {
			if (auto iter = skeletonMap.find(n.name.c_str()); iter != skeletonMap.end()) {
				skeletonIdxs.push_back(iter->second);
				if (std::holds_alternative<fastgltf::TRS>(n.transform)) {
					auto& trs = std::get<fastgltf::TRS>(n.transform);
					auto& b = bindPose[iter->second];
					b.rotation = {
						trs.rotation[0],
						trs.rotation[1],
						trs.rotation[2],
						trs.rotation[3]
					};
					b.translation = {
						trs.translation[0],
						trs.translation[1],
						trs.translation[2]
					};
					b.scale = ozz::math::Float3::one();
				}
			} else {
				skeletonIdxs.push_back(UINT64_MAX);
			}
		}

		//Create the raw animation
		auto rawAnimResult = std::make_unique<Animation::RawOzzAnimation>();
		rawAnimResult->data = ozz::make_unique<ozz::animation::offline::RawAnimation>();
		auto& animResult = rawAnimResult->data;
		animResult->duration = 0.001f;
		animResult->tracks.resize(skeletonMap.size());

		//Process GLTF data
		std::vector<float> times;
		for (auto& c : anim->channels) {
			times.clear();
			if (!c.nodeIndex.has_value() || c.nodeIndex > asset->nodes.size())
				continue;

			if (c.path == fastgltf::AnimationPath::Weights) {
				ParseMorphChannel(assetData, anim, c, rawAnimResult.get());
				continue;
			}

			auto idx = skeletonIdxs[c.nodeIndex.value()];
			if (idx == UINT64_MAX)
				continue;

			auto& rTl = animResult->tracks[idx].rotations;
			auto& pTl = animResult->tracks[idx].translations;

			if (c.samplerIndex > anim->samplers.size())
				continue;

			auto& sampler = anim->samplers[c.samplerIndex];
			if (sampler.inputAccessor > asset->accessors.size() || sampler.outputAccessor > asset->accessors.size())
				continue;

			auto& timeAccessor = asset->accessors[sampler.inputAccessor];
			auto& dataAccessor = asset->accessors[sampler.outputAccessor];

			if (timeAccessor.count != dataAccessor.count)
				continue;

			ozz::animation::offline::RawAnimation::RotationKey r;
			ozz::animation::offline::RawAnimation::TranslationKey p;
			for (size_t i = 0; i < timeAccessor.count; i++) {
				float t = fastgltf::getAccessorElement<float>(*asset, timeAccessor, i);
				if (t > animResult->duration)
					animResult->duration = t;

				switch (c.path) {
				case fastgltf::AnimationPath::Rotation:
					r.value = fastgltf::getAccessorElement<ozz::math::Quaternion>(*asset, dataAccessor, i);
					r.time = t;
					rTl.push_back(r);
					break;
				case fastgltf::AnimationPath::Translation:
					p.value = fastgltf::getAccessorElement<ozz::math::Float3>(*asset, dataAccessor, i);
					p.time = t;
					pTl.push_back(p);
					break;
				}
			}
		}

		for (size_t i = 0; i < skeletonMap.size(); i++) {
			auto& rTl = animResult->tracks[i].rotations;
			auto& pTl = animResult->tracks[i].translations;
			auto& b = bindPose[i];
			if (rTl.empty()) {
				ozz::animation::offline::RawAnimation::RotationKey r;
				r.time = 0.0001f;
				r.value = b.rotation;
				rTl.push_back(r); 
			}
			if (pTl.empty()) {
				ozz::animation::offline::RawAnimation::TranslationKey p;
				p.time = 0.0001f;
				p.value = b.translation;
				pTl.push_back(p);
			}
		}

		if (rawAnimResult->faceData != nullptr) {
			auto faceResult = rawAnimResult->faceData.get();
			ozz::animation::offline::RawTrackKeyframe<float> kf{};
			kf.interpolation = ozz::animation::offline::RawTrackInterpolation::kLinear;
			kf.ratio = 0.0f;
			kf.value = 0.0f;
			for (auto& t : faceResult->tracks) {
				if (t.keyframes.empty()) {
					t.keyframes.push_back(kf);
				}
			}
		}

		return rawAnimResult;
	}

	std::unique_ptr<Animation::OzzAnimation> GLTFImport::CreateRuntimeAnimation(const AssetData* assetData, const fastgltf::Animation* anim, const ozz::animation::Skeleton* skeleton)
	{
		auto result = std::make_unique<Animation::OzzAnimation>();

		auto animResult = CreateRawAnimation(assetData, anim, skeleton);
		ozz::animation::offline::AnimationBuilder builder;
		result->data = builder(*animResult->data);

		if (animResult->faceData.get() != nullptr) {
			ozz::animation::offline::TrackBuilder trackBuilder;
			result->faceData = std::make_unique<Animation::OzzFaceAnimation>();
			result->faceData->duration = animResult->faceData->duration;
			for (size_t i = 0; i < animResult->faceData->tracks.size(); i++) {
				result->faceData->tracks[i] = std::move(*trackBuilder(animResult->faceData->tracks[i]));
			}
		}

		return result;
	}

	std::unique_ptr<GLTFImport::AssetData> GLTFImport::LoadGLTF(const std::filesystem::path& fileName)
	{
		try {
			zstr::ifstream file(fileName.generic_string(), std::ios::binary);

			if (file.bad())
				return nullptr;

			std::vector<uint8_t> buffer;
			std::istreambuf_iterator<char> fileEnd;

			for (std::istreambuf_iterator<char> iter(file); iter != fileEnd; iter++) {
				buffer.push_back(*iter);
			}

			size_t baseBufferSize = buffer.size();
			size_t paddedBufferSize = baseBufferSize + fastgltf::getGltfBufferPadding();
			buffer.resize(paddedBufferSize);

			fastgltf::GltfDataBuffer data;
			data.fromByteView(buffer.data(), baseBufferSize, paddedBufferSize);

			fastgltf::Parser parser;
			auto gltfOptions =
				fastgltf::Options::LoadGLBBuffers |
				fastgltf::Options::DecomposeNodeMatrices;

			auto gltfCategories =
				fastgltf::Category::Animations |
				fastgltf::Category::Nodes |
				fastgltf::Category::Meshes |
				fastgltf::Category::Buffers |
				fastgltf::Category::BufferViews |
				fastgltf::Category::Samplers |
				fastgltf::Category::Accessors;

			auto assetData = std::make_unique<AssetData>();
			parser.setUserPointer(assetData.get());
			parser.setExtrasParseCallback([](simdjson::dom::object* extras, std::size_t objectIndex, fastgltf::Category objectType, void* userPointer) {
				auto assetData = static_cast<AssetData*>(userPointer);
				if (objectType != fastgltf::Category::Meshes) {
					return;
				}

				auto arr = (*extras)["targetNames"].get_array();
				if (arr.error() != simdjson::error_code::SUCCESS) {
					return;
				}

				std::string_view name;
				auto& target = assetData->morphTargets[objectIndex];
				for (auto ele : arr) {
					if (ele.get_string().get(name) == simdjson::error_code::SUCCESS) {
						target.emplace_back(name);
					} else {
						assetData->morphTargets.erase(objectIndex);
						return;
					}
				}
			});

			auto gltf = parser.loadGltf(&data, fileName.parent_path(), gltfOptions, gltfCategories);
			if (auto err = gltf.error(); err != fastgltf::Error::None) {
				return nullptr;
			}

			assetData->asset = std::move(gltf.get());
			return assetData;
		} catch (const std::exception&) {
			return nullptr;
		}
	}
}