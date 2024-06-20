#pragma once
#include "Util/General.h"

namespace Animation::Face
{
	using MorphsArray = std::array<float, RE::BSFaceGenAnimationData::morphSize>;
	using MorphData = Util::Guarded<MorphsArray>;

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
		void AttachMorphData(RE::BSFaceGenAnimationData* a_data, std::shared_ptr<MorphData> a_morphs);
		void DetachMorphData(RE::BSFaceGenAnimationData* a_data);
		std::shared_ptr<MorphData> GetMorphData(RE::BSFaceGenAnimationData* a_data);
		void InstallHooks();
		void Reset();

		Util::Guarded<InternalData, std::shared_mutex> data;
	};
}