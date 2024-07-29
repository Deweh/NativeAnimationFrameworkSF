#include "SaveLoadListener.h"
#include "Animation/GraphManager.h"
#include "Animation/Face/Manager.h"
#include "Papyrus/EventManager.h"
#include "Util/Trampoline.h"

namespace Tasks::SaveLoadListener
{
	using revert_t = REL::Relocation<decltype(&RE::BSScript::Internal::VirtualMachine::DropAllRunningData)>;
	revert_t OriginalRevert;

	void* OnRevert(RE::BSScript::Internal::VirtualMachine* a_this)
	{
		Animation::GraphManager::GetSingleton()->Reset();
		Animation::Face::Manager::GetSingleton()->Reset();
		Papyrus::EventManager::GetSingleton()->Reset();
		return OriginalRevert(a_this);
	}

	void InstallHooks()
	{
		REL::Relocation vmSaveIntfcVtbl{ REL::ID(447253) };
		OriginalRevert = vmSaveIntfcVtbl.write_vfunc(0x7, &OnRevert);
	}
}