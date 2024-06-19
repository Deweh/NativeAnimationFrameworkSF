#include "Trampoline.h"

namespace Util::Trampoline
{
	struct HookData
	{
		size_t totalAlloc = 0;
		std::vector<std::function<void(SFSE::Trampoline&)>> hookFuncs;
	};

	std::unique_ptr<HookData> tempData = std::make_unique<HookData>();

	void AddHook(size_t bytesAlloc, const std::function<void(SFSE::Trampoline&)>& func)
	{
		tempData->totalAlloc += bytesAlloc;
		tempData->hookFuncs.push_back(func);
	}

	void ProcessHooks()
	{
		SFSE::AllocTrampoline(tempData->totalAlloc);
		auto& t = SFSE::GetTrampoline();
		for (auto& f : tempData->hookFuncs) {
			f(t);
		}
		tempData.reset();
	}
}