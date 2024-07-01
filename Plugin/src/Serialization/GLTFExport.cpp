#include "GLTFExport.h"
#include "Settings/Settings.h"

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
			Trans,
			Morphs
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
				
				//Prevent accessor duplication.
				if (auto iter = bufferAccMap.find(bufIdx); iter != bufferAccMap.end()) {
					return iter->second;
				}

				auto bvIdx = MakeBView(bufIdx, unitSize * length);
				auto accIdx = MakeAccessor(min, max, type, length, bvIdx);
				bufferAccMap[bufIdx] = accIdx;
				return accIdx;
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

				size_t bufSize = bVec.bytes.size();
				finalBuf.byteLength = bufSize;
				asset->buffers = std::move(finalVec);

				for (auto& acc : asset->accessors) {
					acc.byteOffset = idxPosMap[acc.bufferViewIndex.value()];
					acc.bufferViewIndex = 0;
				}

				asset->bufferViews = std::vector<fastgltf::BufferView>();
				auto& finalBV = asset->bufferViews.emplace_back();
				finalBV.bufferIndex = 0;
				finalBV.byteLength = bufSize;
				finalBV.byteOffset = 0;
			}

			std::map<size_t, size_t> bufferAccMap;
			std::map<BufferType, std::set<size_t>> bufferMap;
			std::unique_ptr<fastgltf::Asset> asset;
		};
	}

	std::vector<std::byte> GLTFExport::CreateOptimizedAsset(Animation::RawOzzAnimation* anim, const ozz::animation::Skeleton* skeleton, uint8_t level)
	{
		if (level > 0) {
			float toleranceLevel = 1e-5;

			switch (level) {
			case 1:
				toleranceLevel = 0.0f;
				break;
			case 2:
				toleranceLevel = 1e-7f;
				break;
			case 3:
				toleranceLevel = 1e-6f;
				break;
			}

			ozz::unique_ptr<ozz::animation::offline::RawAnimation> tempAnim = ozz::make_unique<ozz::animation::offline::RawAnimation>();
			ozz::animation::offline::AnimationOptimizer optimizer;
			optimizer.setting.distance = 1.0f;
			optimizer.setting.tolerance = toleranceLevel;
			optimizer(*anim->data, *skeleton, tempAnim.get());
			anim->data = std::move(tempAnim);
		}

		ExportUtil util;
		auto& asset = util.asset;
		util.Init(skeleton);

		auto& assetAnim = asset->animations.emplace_back();
		assetAnim.name = "Animation";

		const auto DedupeSampler = [&](const fastgltf::AnimationSampler& smplr) -> size_t {
			for (size_t i = 0; i < (assetAnim.samplers.size() - 1); i++) {
				auto& s = assetAnim.samplers[i];
				if (s.inputAccessor == smplr.inputAccessor && s.outputAccessor == smplr.outputAccessor) {
					assetAnim.samplers.pop_back();
					return i;
				}
			}
			return (assetAnim.samplers.size() - 1);
		};

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
			rotChnl.samplerIndex = DedupeSampler(rotSmplr);

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
			transChnl.samplerIndex = DedupeSampler(transSmplr);
		}

		std::vector<std::string> morphTargets = Settings::GetFaceMorphs();
		std::vector<ozz::animation::offline::RawFloatTrack*> tracksView;
		bool hasFaceAnim = false;

		if (anim->faceData != nullptr) {
			auto targetIter = morphTargets.begin();
			auto trackIter = anim->faceData->tracks.begin();
			while (targetIter != morphTargets.end() && trackIter != anim->faceData->tracks.end()) {
				if (trackIter->keyframes.size() == 1 && trackIter->keyframes[0].value == 0.0f) {
					targetIter = morphTargets.erase(targetIter);
				} else {
					targetIter++;
					tracksView.push_back(&(*trackIter));
				}
				trackIter++;
			}

			if (!tracksView.empty()) {
				hasFaceAnim = true;
			}
		}

		if (hasFaceAnim) {
			auto& n = asset->nodes.emplace_back();
			n.name = "_MorphTarget_";
			n.meshIndex = 0;

			auto& mesh = asset->meshes.emplace_back();
			mesh.name = "_MorphTarget_";
			auto& mPrim = mesh.primitives.emplace_back();
			auto& a = mPrim.attributes.emplace_back();
			a.first = "POSITION";
			a.second = 0;

			size_t timeSize = 1;
			ozz::animation::offline::RawFloatTrack* timeTrack = tracksView[0];
			for (auto& t : tracksView) {
				if (t->keyframes.size() > 1) {
					timeSize = t->keyframes.size();
					timeTrack = t;
					break;
				}
			}

			auto& smplr = assetAnim.samplers.emplace_back();
			smplr.interpolation = fastgltf::AnimationInterpolation::Linear;
			smplr.inputAccessor = util.WriteAccessor(
				0.0f,
				anim->faceData->duration,
				fastgltf::AccessorType::Scalar,
				timeSize,
				BufferType::Time,
				[&](size_t i) {
					auto& r = timeTrack->keyframes[i].ratio;
					r *= anim->faceData->duration;
					return &r;
				}
			);

			std::vector<float> combinedWeights;
			combinedWeights.reserve(tracksView.size() * timeSize);

			for (size_t i = 0; i < timeSize; i++) {
				for (size_t j = 0; j < tracksView.size(); j++) {
					auto& kfs = tracksView[j]->keyframes;
					combinedWeights.push_back(i >= kfs.size() ? kfs.back().value : kfs[i].value);
				}
			}

			smplr.outputAccessor = util.WriteAccessor(
				0.0f,
				0.0f,
				fastgltf::AccessorType::Scalar,
				combinedWeights.size(),
				BufferType::Morphs,
				[&](size_t i) { return &combinedWeights[i]; }
			);

			auto& chnl = assetAnim.channels.emplace_back();
			chnl.nodeIndex = (asset->nodes.size() - 1);
			chnl.path = fastgltf::AnimationPath::Weights;
			chnl.samplerIndex = (assetAnim.samplers.size() - 1);
		}

		util.CombineBuffers();
		fastgltf::Exporter exp;
		exp.setUserPointer(&morphTargets);
		exp.setExtrasWriteCallback([](std::size_t objectIndex, fastgltf::Category objectType, void* userPointer) -> std::optional<std::string> {
			if (objectType != fastgltf::Category::Meshes)
				return std::nullopt;

			auto& morphNames = *static_cast<std::vector<std::string>*>(userPointer);
			std::string result = R"({"targetNames":[)";
			for (size_t i = 0; i < morphNames.size(); i++) {
				result += "\"" + morphNames[i] + "\"";
				if ((i + 1) < morphNames.size()) {
					result += ",";
				}
			}
			result += "]}";
			return result;
		});

		auto result = exp.writeGltfBinary(*util.asset.get(), fastgltf::ExportOptions::None);
		if (result.error() != fastgltf::Error::None) {
			return {};
		}

		return std::move(result.get().output);
	}
}