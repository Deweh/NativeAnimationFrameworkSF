#include "GLTFExport.h"
#include "GLTFImport.h"

namespace Serialization
{
	namespace
	{
		constexpr size_t Vec4Size = sizeof(float) * 4;
		constexpr size_t Vec3Size = sizeof(float) * 3;
		constexpr size_t ScalarSize = sizeof(float);

		enum BufferType
		{
			Rot,
			Time,
			Trans
		};

		struct ExportUtil
		{
			void Init(const ozz::animation::Skeleton* skeleton)
			{
				asset = std::make_unique<fastgltf::Asset>();
				fastgltf::Node n;
				n.transform = fastgltf::TRS{
					.translation = { 0.0f, 0.0f, 0.0f },
					.rotation = { 0.0f, 0.0f, 0.0f, 1.0f },
					.scale = { 1.0f, 1.0f, 1.0f }
				};

				auto joints = skeleton->joint_names();
				for (auto& j : joints) {
					n.name = j;
					asset->nodes.push_back(n);
				}
			}

			size_t MakeBView(size_t bufferIdx, size_t length)
			{
				auto& bv = asset->bufferViews.emplace_back();
				bv.bufferIndex = bufferIdx;
				bv.byteLength = length;
				bv.byteOffset = 0;
				return (asset->bufferViews.size() - 1);
			}

			fastgltf::sources::Vector& MakeBuffer(size_t length)
			{
				auto& buf = asset->buffers.emplace_back();
				buf.data.emplace<fastgltf::sources::Vector>();
				auto& bVec = std::get<fastgltf::sources::Vector>(buf.data);
				bVec.mimeType = fastgltf::MimeType::GltfBuffer;
				bVec.bytes.resize(length);
				return bVec;
			}

			size_t WriteBuffer(size_t unitSize, size_t length, BufferType bufType, const std::function<void*(size_t)>& getFunc)
			{
				auto& buf = MakeBuffer(unitSize * length);
				for (size_t i = 0; i < length; i++) {
					memcpy(&buf.bytes[i * unitSize], getFunc(i), unitSize);
				}
				return DedupeLastBuffer(bufType, buf);
			}

			size_t MakeAccessor(double min, double max, fastgltf::AccessorType type, size_t count, size_t bufferView)
			{
				auto& acc = asset->accessors.emplace_back();
				acc.min.emplace<std::pmr::vector<double>>(1, min);
				acc.max.emplace<std::pmr::vector<double>>(1, max);
				acc.type = type;
				acc.count = count;
				acc.componentType = fastgltf::ComponentType::Float;
				acc.bufferViewIndex = bufferView;
				return (asset->accessors.size() - 1);
			}

			size_t WriteAccessor(double min, double max, fastgltf::AccessorType type, size_t length, BufferType bufType, const std::function<void*(size_t)>& getFunc)
			{
				size_t unitSize = 0;
				switch (type) {
				case fastgltf::AccessorType::Scalar:
					unitSize = ScalarSize;
					break;
				case fastgltf::AccessorType::Vec3:
					unitSize = Vec3Size;
					break;
				case fastgltf::AccessorType::Vec4:
					unitSize = Vec4Size;
					break;
				}

				auto bufIdx = WriteBuffer(unitSize, length, bufType, getFunc);
				bufferMap[bufType].insert(bufIdx);

				auto bvIdx = MakeBView(bufIdx, unitSize * length);
				return MakeAccessor(min, max, type, length, bvIdx);
			}

			size_t DedupeLastBuffer(BufferType bufType, fastgltf::sources::Vector& cmp)
			{
				auto iter = bufferMap.find(bufType);
				if (iter == bufferMap.end()) {
					return (asset->buffers.size() - 1);
				}

				for (auto& idx : iter->second) {
					auto& vec = std::get<fastgltf::sources::Vector>(asset->buffers[idx].data);
					if (vec.bytes.size() != cmp.bytes.size())
						continue;

					if (memcmp(vec.bytes.data(), cmp.bytes.data(), cmp.bytes.size()) == 0) {
						asset->buffers.pop_back();
						return idx;
					}
				}
				return (asset->buffers.size() - 1);
			}

			void CombineBuffers()
			{
				std::vector<size_t> idxPosMap;
				idxPosMap.reserve(asset->buffers.size());

				std::vector<fastgltf::Buffer> finalVec;
				auto& finalBuf = finalVec.emplace_back();
				finalBuf.data.emplace<fastgltf::sources::Vector>();
				auto& bVec = std::get<fastgltf::sources::Vector>(finalBuf.data);
				bVec.mimeType = fastgltf::MimeType::OctetStream;

				for (auto& b : asset->buffers) {
					idxPosMap.push_back(bVec.bytes.empty() ? 0 : bVec.bytes.size());
					auto& curVec = std::get<fastgltf::sources::Vector>(b.data);
					bVec.bytes.insert(bVec.bytes.end(), curVec.bytes.begin(), curVec.bytes.end());
				}

				finalBuf.byteLength = bVec.bytes.size();
				asset->buffers = std::move(finalVec);

				for (auto& bv : asset->bufferViews) {
					bv.byteOffset = idxPosMap[bv.bufferIndex];
					bv.bufferIndex = 0;
				}
			}

			std::map<BufferType, std::set<size_t>> bufferMap;
			std::unique_ptr<fastgltf::Asset> asset;
		};
	}

	std::unique_ptr<fastgltf::Asset> GLTFExport::CreateOptimizedAsset(Animation::RawOzzAnimation* anim, const ozz::animation::Skeleton* skeleton)
	{
		ExportUtil util;
		auto& asset = util.asset;
		util.Init(skeleton);

		auto& assetAnim = asset->animations.emplace_back();
		assetAnim.name = "Animation";

		for (size_t i = 0; i < anim->data->tracks.size(); i++) {
			auto& trck = anim->data->tracks[i];

			auto& rotSmplr = assetAnim.samplers.emplace_back();
			rotSmplr.interpolation = fastgltf::AnimationInterpolation::Linear;
			rotSmplr.inputAccessor = util.WriteAccessor(
				trck.rotations.front().time,
				trck.rotations.back().time,
				fastgltf::AccessorType::Scalar,
				trck.rotations.size(),
				BufferType::Time,
				[&](size_t i) { return &trck.rotations[i].time; }
			);
			rotSmplr.outputAccessor = util.WriteAccessor(
				0.0f,
				0.0f,
				fastgltf::AccessorType::Vec4,
				trck.rotations.size(),
				BufferType::Rot,
				[&](size_t i) { return &trck.rotations[i].value; }
			);

			auto& rotChnl = assetAnim.channels.emplace_back();
			rotChnl.nodeIndex = i;
			rotChnl.path = fastgltf::AnimationPath::Rotation;
			rotChnl.samplerIndex = (assetAnim.samplers.size() - 1);

			auto& transSmplr = assetAnim.samplers.emplace_back();
			transSmplr.interpolation = fastgltf::AnimationInterpolation::Linear;
			transSmplr.inputAccessor = util.WriteAccessor(
				trck.translations.front().time,
				trck.translations.back().time,
				fastgltf::AccessorType::Scalar,
				trck.translations.size(),
				BufferType::Time,
				[&](size_t i) { return &trck.translations[i].time; }
			);
			transSmplr.outputAccessor = util.WriteAccessor(
				0.0f,
				0.0f,
				fastgltf::AccessorType::Vec3,
				trck.translations.size(),
				BufferType::Trans,
				[&](size_t i) { return &trck.translations[i].value; }
			);

			auto& transChnl = assetAnim.channels.emplace_back();
			transChnl.nodeIndex = i;
			transChnl.path = fastgltf::AnimationPath::Translation;
			transChnl.samplerIndex = (assetAnim.samplers.size() - 1);
		}

		util.CombineBuffers();
		return std::move(util.asset);
	}
}