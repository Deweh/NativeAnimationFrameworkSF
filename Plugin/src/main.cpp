#include "DKUtil/Hook.hpp"
#include "Settings/Settings.h"
#include "Animation/GraphManager.h"
#include "Commands/NAFCommand.h"
#include "Tasks/SaveLoadListener.h"
#include "API/CCF_API.h"
#include "Util/Trampoline.h"
#include "Animation/Face/Manager.h"
#define NDEBUG

namespace
{
	void MessageCallback(SFSE::MessagingInterface::Message* a_msg) noexcept
	{
		switch (a_msg->type) {
		case SFSE::MessagingInterface::kPostLoad:
			{
				CCF::RegisterCommand("naf", Commands::NAFCommand::Run);
			}
			break;
		default:
			break;
		}
	}

	void InstallHooks()
	{
		Animation::GraphManager::GetSingleton()->InstallHooks();
		Animation::Face::Manager::GetSingleton()->InstallHooks();
		Tasks::SaveLoadListener::InstallHooks();
		Util::Trampoline::ProcessHooks();
	}
}

DLLEXPORT bool SFSEAPI SFSEPlugin_Load(const SFSE::LoadInterface* a_sfse)
{
	SFSE::Init(a_sfse, false);
	DKUtil::Logger::Init(Plugin::NAME, std::to_string(Plugin::Version));
	INFO("{} v{} loaded", Plugin::NAME, Plugin::Version);
	INFO("Starfield Offset: {:X}", REL::Module::get().base());

	InstallHooks();
	Settings::Load();
	SFSE::GetMessagingInterface()->RegisterListener(MessageCallback);
	//SFSE::GetTaskInterface()->AddPermanentTask(Tasks::MainLoop::GetSingleton());
	return true;
}