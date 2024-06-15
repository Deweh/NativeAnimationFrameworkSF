#include "DKUtil/Hook.hpp"
#include "Settings/Settings.h"
#include "Animation/GraphManager.h"
#include "Commands/NAFCommand.h"
#include "Tasks/SaveLoadListener.h"
#include "API/CCF_API.h"
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
}

DLLEXPORT bool SFSEAPI SFSEPlugin_Load(const SFSE::LoadInterface* a_sfse)
{
	SFSE::Init(a_sfse, false);
	DKUtil::Logger::Init(Plugin::NAME, std::to_string(Plugin::Version));
	INFO("{} v{} loaded", Plugin::NAME, Plugin::Version);
	
	INFO("Starfield Offset: {:X}", REL::Module::get().base());
	SFSE::AllocTrampoline(14);
	Animation::GraphManager::GetSingleton()->InstallHooks();
	Tasks::SaveLoadListener::InstallHooks();

	Settings::Load();
	SFSE::GetMessagingInterface()->RegisterListener(MessageCallback);
	//SFSE::GetTaskInterface()->AddPermanentTask(Tasks::MainLoop::GetSingleton());
	//RE::InitLoadEvent::RegisterSink(InitSink::GetSingleton());
	return true;
}