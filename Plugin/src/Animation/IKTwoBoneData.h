#pragma once

namespace Animation
{
	struct IKTwoBoneData
	{
		enum FLAG : uint8_t
		{
			kNoFlags = 0,
			kDeleteOnTransitionFinish = 1 << 0,
			kPendingDelete = 1 << 1
		};

		enum TransitionMode : uint8_t
		{
			kNone = 0,
			kIn = 1,
			kOut = 2
		};

		struct TransitionData
		{
			TransitionMode mode = TransitionMode::kNone;
			float duration = 0.0f;
			float currentTime = 0.0f;
		};

		std::array<RE::NiAVObject*, 3> nodes;
		std::array<std::string, 3> nodeNames;
		RE::NiTransform targetWorld;
		RE::NiPoint3 polePtModel;
		ozz::animation::IKTwoBoneJob job;
		TransitionData transition;
		SFSE::stl::enumeration<FLAG, uint8_t> flags = FLAG::kNoFlags;

		IKTwoBoneData();

		void TransitionIn(float a_duration);
		void TransitionOut(float a_duration, bool a_delete);
		bool Validate();
		bool Update(float a_deltaTime, RE::NiAVObject* a_rootNode);
	};
}