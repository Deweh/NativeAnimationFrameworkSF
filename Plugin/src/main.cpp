#include "Settings/Settings.h"
#include "Animation/GraphManager.h"
#include "Commands/NAFCommand.h"
#include "Tasks/SaveLoadListener.h"
#include "API/CCF_API.h"
#include "Util/Trampoline.h"
#include "Animation/Face/Manager.h"
#include "Tasks/Input.h"
#include "Papyrus/NAFScript.h"
#define NDEBUG

namespace
{
	void MessageCallback(SFSE::MessagingInterface::Message* a_msg) noexcept
	{
		switch (a_msg->type) {
		case SFSE::MessagingInterface::kPostDataLoad:
			{
				Settings::Init();
				break;
			}
		case SFSE::MessagingInterface::kPostLoad:
			{
				CCF::RegisterCommand("naf", Commands::NAFCommand::Run);
				break;
			}
		default:
			break;
		}
	}

	void BindPapyrusScripts(RE::BSScript::IVirtualMachine** a_vm)
	{
		Papyrus::NAFScript::RegisterFunctions(*a_vm);
	}
}

DLLEXPORT bool SFSEAPI SFSEPlugin_Load(const SFSE::LoadInterface* a_sfse)
{
	SFSE::Init(a_sfse, true);
	logger::info("Starfield Offset: {:X}", REL::Module::get().base());

	Util::Trampoline::ProcessHooks();
	SFSE::SetPapyrusCallback(&BindPapyrusScripts);
	SFSE::GetMessagingInterface()->RegisterListener(MessageCallback);
	Commands::NAFCommand::RegisterKeybinds();
	return true;
}