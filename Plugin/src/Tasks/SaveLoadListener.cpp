#include "SaveLoadListener.h"
#include "Animation/GraphManager.h"
#include "Animation/Face/Manager.h"
#include "Papyrus/EventManager.h"
#include "Util/Trampoline.h"

namespace Tasks::SaveLoadListener
{
	static Util::VFuncHook<void*(RE::BSScript::Internal::VirtualMachine*)> RevertHook(481117, 0x7, "BSScript::Internal::VirtualMachine::DropAllRunningData",
		[](RE::BSScript::Internal::VirtualMachine* a_this) -> void* {
			Animation::GraphManager::GetSingleton()->Reset();
			Animation::Face::Manager::GetSingleton()->Reset();
			Papyrus::EventManager::GetSingleton()->Reset();
			return RevertHook(a_this);
		});
}