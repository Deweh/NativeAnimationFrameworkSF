#include "GLTFImport.h"
#include "Animation/Transform.h"
#include "Settings/Settings.h"
#include "Util/String.h"
#include "zstr.hpp"

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

		auto asset = LoadGLTF(Util::String::GetDataPath() / info.fileName);
		if (!asset) {
			info.result.error = kFailedToLoad;
			return;
		}

		fastgltf::Animation* anim = nullptr;
		if (info.id.type == AnimationIdentifer::Type::kIndex && info.id.index < asset->animations.size()) {
			anim = &asset->animations[info.id.index];
		} else if (info.id.type == AnimationIdentifer::Type::kName) {
			for (auto& a : asset->animations) {
				if (a.name == info.id.name) {
					anim = &a;
					break;
				}
			}
		}

		if (!anim) {
			info.result.error = kInvalidAnimationIdentifier;
			return;
		}

		auto runtimeAnim = CreateRuntimeAnimation(asset.get(), anim, skeleton->data.get());
		if (!runtimeAnim) {
			info.result.error = kFailedToMakeClip;
			return;
		}

		info.result.anim = std::move(runtimeAnim);
		info.result.error = kSuccess;
	}

	ozz::unique_ptr<ozz::animation::Animation> GLTFImport::CreateRuntimeAnimation(const fastgltf::Asset* asset, const fastgltf::Animation* anim, const ozz::animation::Skeleton* skeleton)
	{
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

		for (const auto& n : asset->nodes) {
			if (auto iter = skeletonMap.find(n.name); iter != skeletonMap.end()) {
				skeletonIdxs.push_back(iter->second);
				if (std::holds_alternative<fastgltf::Node::TRS>(n.transform)) {
					auto& trs = std::get<fastgltf::Node::TRS>(n.transform);
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
		ozz::animation::offline::RawAnimation animResult;
		animResult.duration = 0.001f;
		animResult.tracks.resize(skeletonMap.size());

		//Process GLTF data
		std::vector<float> times;
		for (auto& c : anim->channels) {
			times.clear();
			if (c.nodeIndex > asset->nodes.size())
				continue;

			auto idx = skeletonIdxs[c.nodeIndex];
			if (idx == UINT64_MAX)
				continue;

			auto& rTl = animResult.tracks[idx].rotations;
			auto& pTl = animResult.tracks[idx].translations;

			if (c.samplerIndex > anim->samplers.size())
				continue;

			auto& sampler = anim->samplers[c.samplerIndex];
			if (sampler.inputAccessor > asset->accessors.size() || sampler.outputAccessor > asset->accessors.size())
				continue;

			auto& timeAccessor = asset->accessors[sampler.inputAccessor];
			auto& dataAccessor = asset->accessors[sampler.outputAccessor];

			size_t elementSize = 0;
			switch (c.path) {
			case fastgltf::AnimationPath::Rotation:
				if (dataAccessor.type != fastgltf::AccessorType::Vec4)
					continue;
				elementSize = 16;
				break;
			case fastgltf::AnimationPath::Translation:
				if (dataAccessor.type != fastgltf::AccessorType::Vec3)
					continue;
				elementSize = 12;
				break;
			default:
				continue;
			}

			if (timeAccessor.count != dataAccessor.count ||
				dataAccessor.componentType != fastgltf::ComponentType::Float ||
				timeAccessor.componentType != fastgltf::ComponentType::Float ||
				timeAccessor.type != fastgltf::AccessorType::Scalar ||
				!timeAccessor.bufferViewIndex.has_value() ||
				!dataAccessor.bufferViewIndex.has_value())
				continue;

			auto& timeBV = asset->bufferViews[timeAccessor.bufferViewIndex.value()];
			auto& dataBV = asset->bufferViews[dataAccessor.bufferViewIndex.value()];

			size_t timeByteOffset = timeAccessor.byteOffset + timeBV.byteOffset;
			size_t dataByteOffset = dataAccessor.byteOffset + dataBV.byteOffset;
			size_t timeByteCount =  timeAccessor.count * 4;
			size_t dataByteCount = dataAccessor.count * elementSize;

			auto& timeBuffer = asset->buffers[timeBV.bufferIndex];
			auto& dataBuffer = asset->buffers[dataBV.bufferIndex];
				
			if (!std::holds_alternative<fastgltf::sources::Vector>(timeBuffer.data) ||
				!std::holds_alternative<fastgltf::sources::Vector>(dataBuffer.data))
				continue;

			auto& tBufData = std::get<fastgltf::sources::Vector>(timeBuffer.data);
			auto& dBufData = std::get<fastgltf::sources::Vector>(dataBuffer.data);

			if (timeByteOffset + (timeByteCount) > tBufData.bytes.size() ||
				dataByteOffset + (dataByteCount) > dBufData.bytes.size())
				continue;

			ozz::animation::offline::RawAnimation::RotationKey r;
			ozz::animation::offline::RawAnimation::TranslationKey p;
			for (size_t i = 0; i < timeAccessor.count; i++) {
				float t;
				std::memcpy(&t, &tBufData.bytes[timeByteOffset + (i * 4)], 4);
				if (t > animResult.duration)
					animResult.duration = t;

				size_t off = dataByteOffset + (i * elementSize);
				switch (c.path) {
				case fastgltf::AnimationPath::Rotation:
					std::memcpy(&r.value.x, &dBufData.bytes[off], elementSize);
					r.time = t;
					rTl.push_back(r);
					break;
				case fastgltf::AnimationPath::Translation:
					std::memcpy(&p.value.x, &dBufData.bytes[off], elementSize);
					p.time = t;
					pTl.push_back(p);
					break;
				}
			}
		}

		for (size_t i = 0; i < skeletonMap.size(); i++) {
			auto& rTl = animResult.tracks[i].rotations;
			auto& pTl = animResult.tracks[i].translations;
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

		ozz::animation::offline::AnimationBuilder builder;
		return builder(animResult);
	}

	std::unique_ptr<fastgltf::Asset> GLTFImport::LoadGLTF(const std::filesystem::path& fileName)
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

			auto type = fastgltf::determineGltfFileType(&data);
			std::unique_ptr<fastgltf::glTF> gltf;
			if (type == fastgltf::GltfType::glTF) {
				gltf = parser.loadGLTF(&data, fileName.parent_path(), gltfOptions);
			} else if (type == fastgltf::GltfType::GLB) {
				gltf = parser.loadBinaryGLTF(&data, fileName.parent_path(), gltfOptions);
			} else {
				return nullptr;
			}

			if (!gltf)
				return nullptr;

			if (gltf->parse(fastgltf::Category::OnlyAnimations | fastgltf::Category::Nodes) != fastgltf::Error::None || gltf->validate() != fastgltf::Error::None)
				return nullptr;

			return gltf->getParsedAsset();
		} catch (const std::exception&) {
			return nullptr;
		}
	}
}