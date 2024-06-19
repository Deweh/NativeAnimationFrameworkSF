#pragma once
#include "Util/General.h"

namespace Animation::Face
{
	class Manager
	{
	public:
		struct InternalData
		{
			std::map<uint32_t, RE::BSFaceGenAnimationData*> refMap;
			std::unordered_set<RE::BSFaceGenAnimationData*> noBlink;
		};

		static Manager* GetSingleton();
		void SetNoBlink(uint32_t a_refId, RE::BSFaceGenAnimationData* a_data, bool a_noBlink);
		void InstallHooks();
		void Reset();

		Util::Guarded<InternalData, std::shared_mutex> data;
	};
}