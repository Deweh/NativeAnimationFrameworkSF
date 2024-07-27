#include "SaveLoadListener.h"
#include "Animation/GraphManager.h"
#include "Animation/Face/Manager.h"
#include "Papyrus/EventManager.h"
#include "Util/Trampoline.h"

namespace Tasks::SaveLoadListener
{
	using savegame_t = REL::Relocation<decltype(&RE::BGSSaveLoadGame::SaveGame)>;
	using loadgame_t = REL::Relocation<decltype(&RE::BGSSaveLoadGame::LoadGame)>;
	using revert_t = REL::Relocation<decltype(&RE::BSScript::Internal::VirtualMachine::DropAllRunningData)>;

	savegame_t OriginalSaveGame;
	loadgame_t OriginalLoadGame;
	revert_t OriginalRevert;

	std::string MakeSavePath(std::string a_saveName, bool a_hasExtension)
	{
		if (!a_hasExtension) {
			a_saveName += ".sfs";
		}

		std::string_view localPath = RE::INISettingCollection::GetSingleton()->GetSetting<std::string_view>("sLocalSavePath:General", "Saves");

		char documents_path[MAX_PATH];
		SHGetFolderPathA(NULL, CSIDL_MYDOCUMENTS, NULL, SHGFP_TYPE_CURRENT, documents_path);

		std::filesystem::path result = std::string_view(documents_path);
		return	(result / "My Games" / "Starfield" / localPath / a_saveName).generic_string();
	}

	void OnSaveGame(RE::BGSSaveLoadGame* a_this, void* a_unk1, void* a_unk2, const char* a_name)
	{
		INFO("Save game: {}", MakeSavePath(a_name, true));
		OriginalSaveGame(a_this, a_unk1, a_unk2, a_name);
	}

	void OnLoadGame(RE::BGSSaveLoadGame* a_this, const char* a_name, void* a_unk1, void* a_unk2)
	{
		INFO("Load game: {}", MakeSavePath(a_name, false));
		OriginalLoadGame(a_this, a_name, a_unk1, a_unk2);
	}

	void OnRevert(RE::BSScript::Internal::VirtualMachine* a_this)
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

		/*
		Util::Trampoline::AddHook(28, [](SFSE::Trampoline& t) {
			// Call from <UnknownFunc> to BGSSaveLoadGame::SaveGame
			REL::Relocation hookLoc{ REL::ID(147478), 0x12B };
			OriginalSaveGame = t.write_call<5>(hookLoc.address(), &OnSaveGame);

			// Call from <UnknownFunc> to BGSSaveLoadGame::LoadGame
			REL::Relocation hookLoc2{ REL::ID(147850), 0x572 };
			OriginalLoadGame = t.write_call<5>(hookLoc2.address(), &OnLoadGame);
		});
		*/
	}
}