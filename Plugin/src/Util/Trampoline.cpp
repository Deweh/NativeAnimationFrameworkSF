#include "Trampoline.h"

namespace Util::Trampoline
{
	struct HookData
	{
		size_t totalAlloc = 0;
		std::vector<std::function<void(SFSE::Trampoline&, uintptr_t)>> hookFuncs;
	};

	 std::unique_ptr<HookData>& GetTempData()
	{
		static std::unique_ptr<HookData> tempData{ std::make_unique<HookData>() };
		return tempData;
	}

	void AddHook(size_t bytesAlloc, const std::function<void(SFSE::Trampoline&, uintptr_t)>& func)
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
		uintptr_t baseAddr = REL::Module::get().base();
		for (auto& f : tempData->hookFuncs) {
			f(t, baseAddr);
		}
		tempData.reset();
	}
}