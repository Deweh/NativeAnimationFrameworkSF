#include "GLTFImport.h"
#include "Animation/Transform.h"

namespace Serialization
{
	std::unique_ptr<Animation::LinearClipGenerator> GLTFImport::CreateClipGenerator(const fastgltf::Asset* asset, const fastgltf::Animation* anim, const Settings::SkeletonDescriptor* skeleton)
	{
		//Create a map of GLTF node indexes -> skeleton indexes
		std::vector<size_t> skeletonIdxs;
		skeletonIdxs.reserve(asset->nodes.size());
		auto skeletonMap = skeleton->GetNodeIndexMap();

		for (const auto& n : asset->nodes) {
			if (auto iter = skeletonMap.find(n.name); iter != skeletonMap.end()) {
				skeletonIdxs.push_back(iter->second);
			} else {
				skeletonIdxs.push_back(UINT64_MAX);
			}
		}

		//Create the clip generator
		std::unique_ptr<Animation::LinearClipGenerator> result = std::make_unique<Animation::LinearClipGenerator>();
		result->duration = 0.001f;
		result->SetSize(skeletonMap.size());

		//Process GLTF data
		std::vector<std::optional<Animation::Transform>> bindPose(skeletonMap.size(), std::optional<Animation::Transform>(std::nullopt));
		std::vector<float> times;
		for (auto& c : anim->channels) {
			times.clear();
			if (c.nodeIndex > asset->nodes.size())
				continue;

			auto idx = skeletonIdxs[c.nodeIndex];
			if (idx == UINT64_MAX)
				continue;

			auto& rTl = result->rotation[idx];
			auto& pTl = result->position[idx];

			if (auto& curNode = asset->nodes[c.nodeIndex]; std::holds_alternative<fastgltf::Node::TRS>(curNode.transform)) {
				auto& trs = std::get<fastgltf::Node::TRS>(curNode.transform);
				bindPose[idx] = {
					RE::NiQuaternion{
						trs.rotation[3],
						trs.rotation[0],
						trs.rotation[1],
						trs.rotation[2]
					},
					RE::NiPoint3{
						trs.translation[0],
						trs.translation[1],
						trs.translation[2]
					}
				};			
			}

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

			for (size_t i = 0; i < timeAccessor.count; i++) {
				float t;
				std::memcpy(&t, &tBufData.bytes[timeByteOffset + (i * 4)], 4);
				if (t > result->duration)
					result->duration = t;

				size_t off = dataByteOffset + (i * elementSize);
				RE::NiQuaternion q;
				RE::NiPoint3 p;
				switch (c.path) {
				case fastgltf::AnimationPath::Rotation:
					//NiQuaternions are stored as WXYZ, but GLTF stores rotations as XYZW, so we have to copy XYZ and W separately.
					std::memcpy(&q.x, &dBufData.bytes[off], 12);
					off += 12;
					std::memcpy(&q.w, &dBufData.bytes[off], 4);
					rTl.keys.emplace(t, q);
					break;
				case fastgltf::AnimationPath::Translation:
					std::memcpy(&p.x, &dBufData.bytes[off], elementSize);
					pTl.keys.emplace(t, p);
					break;
				}
			}
		}

		for (size_t i = 0; i < skeletonMap.size(); i++) {
			auto& rTl = result->rotation[i].keys;
			auto& pTl = result->position[i].keys;
			auto& b = bindPose[i];
			if (rTl.empty() && b.has_value()) {
				rTl[0.0001f] = b->rotate;
			}
			if (pTl.empty() && b.has_value()) {
				pTl[0.0001f] = b->translate;
			}
		}

		return result;
	}

	std::unique_ptr<fastgltf::Asset> GLTFImport::LoadGLTF(const std::filesystem::path& fileName)
	{
		fastgltf::GltfDataBuffer data;
		if (!data.loadFromFile(fileName))
			return nullptr;

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
	}
}