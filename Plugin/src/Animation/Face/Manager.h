#pragma once
#include "Util/General.h"
#include "Animation/Easing.h"

namespace Animation::Face
{
	using MorphsArray = std::array<float, RE::BSFaceGenAnimationData::morphSize>;

	struct GraphStub
	{
		enum TransitionState
		{
			kNone,
			kTween,
			kOut
		};

		struct TransitionData
		{
			TransitionState state = kNone;
			float duration = 1.0f;
			float time = 0.0f;
			CubicInOutEase<float> ease;
		};

		void BeginTween(float a_duration, RE::BSFaceGenAnimationData* a_data);
		void BeginOutTransition(float a_duration);
		bool Update(float a_deltaTime, RE::BSFaceGenAnimationData* a_data);

		TransitionData transition;
		MorphsArray snapshot;
		MorphsArray current;
	};

	using MorphData = Util::Guarded<GraphStub>;

	class Manager
	{
	public:
		struct InternalData
		{
			std::unordered_set<RE::BSFaceGenAnimationData*> noBlink;
			std::map<RE::BSFaceGenAnimationData*, std::shared_ptr<MorphData>> controlledDatas;
		};

		static Manager* GetSingleton();
		void OnAnimDataChange(RE::BSFaceGenAnimationData* a_old, RE::BSFaceGenAnimationData* a_new);
		void SetNoBlink(RE::BSFaceGenAnimationData* a_data, bool a_noBlink);
		void AttachMorphData(RE::BSFaceGenAnimationData* a_data, std::shared_ptr<MorphData> a_morphs, float a_transitionTime);
		void DetachMorphData(RE::BSFaceGenAnimationData* a_data, float a_transitionTime);
		void DoDetach(RE::BSFaceGenAnimationData* a_data);
		std::shared_ptr<MorphData> GetMorphData(RE::BSFaceGenAnimationData* a_data);
		void InstallHooks();
		void Reset();

		Util::Guarded<InternalData, std::shared_mutex> data;
	};
}