#include "SaveLoadListener.h"
#include "Animation/GraphManager.h"

namespace Tasks::SaveLoadListener
{
	typedef int64_t(*PlayerLoadGameFunc)(RE::PlayerCharacter*, RE::BGSLoadFormBuffer*);

	PlayerLoadGameFunc OriginalLoadGame;

	int64_t PlayerLoadGame(RE::PlayerCharacter* a1, RE::BGSLoadFormBuffer* a2) {
		Animation::GraphManager::GetSingleton()->Reset();
		return OriginalLoadGame(a1, a2);
	}

	void InstallHooks()
	{
		REL::Relocation<uintptr_t> playerVtbl{ REL::ID(423292) };
		OriginalLoadGame = reinterpret_cast<PlayerLoadGameFunc>(playerVtbl.write_vfunc(27, &PlayerLoadGame));
	}
}