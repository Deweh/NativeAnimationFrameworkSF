#pragma once

namespace Util::Trampoline
{
	void AddHook(size_t bytesAlloc, const std::function<void(SFSE::Trampoline&, uintptr_t)>& func);
	void ProcessHooks();
}