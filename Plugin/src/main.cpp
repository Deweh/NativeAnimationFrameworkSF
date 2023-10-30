#include "DKUtil/Hook.hpp"
#include "Settings/Settings.h"
#include "Animation/GraphManager.h"
#include "Commands/Commands.h"
#include "Commands/NAFCommand.h"
#include "Tasks/SaveLoadListener.h"
#define NDEBUG

namespace
{
	/*
	class InitSink : public RE::BSTEventSink<RE::InitLoadEvent>
	{
	public:
		static InitSink* GetSingleton() {
			static InitSink singleton;
			return &singleton;
		}

		virtual RE::BSEventNotifyControl ProcessEvent(const RE::InitLoadEvent& a_event, RE::BSTEventSource<RE::InitLoadEvent>* a_source)
		{
			if (a_event.stage == RE::InitLoadEvent::Unk5) {
				RE::UI::GetSingleton()->RegisterSink(Tasks::MainLoop::GetSingleton());
				//AddConsoleCommand();
			}
			return RE::BSEventNotifyControl::kContinue;
		}
	};
	
	void MessageCallback(SFSE::MessagingInterface::Message* a_msg) noexcept
	{
		switch (a_msg->type) {
		case SFSE::MessagingInterface::kPostDataLoad:
			{
			}
			break;
		default:
			break;
		}
	}
	*/
}


DLLEXPORT bool SFSEAPI SFSEPlugin_Load(const SFSE::LoadInterface* a_sfse)
{
	SFSE::Init(a_sfse, false);
	DKUtil::Logger::Init(Plugin::NAME, std::to_string(Plugin::Version));
	INFO("{} v{} loaded", Plugin::NAME, Plugin::Version);
	
	SFSE::AllocTrampoline(28);
	Animation::GraphManager::GetSingleton()->InstallHooks();
	Tasks::SaveLoadListener::InstallHooks();
	Commands::InstallHooks();
	Commands::RegisterCommand("naf", Commands::NAFCommand::Run);

	Settings::Load();
	//SFSE::GetMessagingInterface()->RegisterListener(MessageCallback);
	//SFSE::GetTaskInterface()->AddPermanentTask(Tasks::MainLoop::GetSingleton());
	//RE::InitLoadEvent::RegisterSink(InitSink::GetSingleton());
	return true;
}