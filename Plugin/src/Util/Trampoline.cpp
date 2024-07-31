#include "Trampoline.h"

namespace Util::Trampoline
{
	struct HookData
	{
		size_t totalAlloc = 0;
		std::vector<std::function<void(SFSE::Trampoline&)>> hookFuncs;
	};

	 std::unique_ptr<HookData>& GetTempData()
	{
		static std::unique_ptr<HookData> tempData{ std::make_unique<HookData>() };
		return tempData;
	}

	void AddHook(size_t bytesAlloc, const std::function<void(SFSE::Trampoline&)>& func)
	{
		auto& tempData = GetTempData();
		tempData->totalAlloc += bytesAlloc;
		tempData->hookFuncs.push_back(func);
	}

	void ProcessHooks()
	{
		auto& tempData = GetTempData();
		SFSE::AllocTrampoline(tempData->totalAlloc);
		auto& t = SFSE::GetTrampoline();
		for (auto& f : tempData->hookFuncs) {
			f(t);
		}
		tempData.reset();
	}
}