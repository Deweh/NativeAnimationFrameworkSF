#include "Settings/Settings.h"
#include "Settings/Impl.h"
#include "Commands/NAFCommand.h"
#include "API/CCF_API.h"
#include "Util/Trampoline.h"
#include "Papyrus/NAFScript.h"
#include "Settings/SkeletonImpl.h"

namespace
{
	void MessageCallback(SFSE::MessagingInterface::Message* a_msg) noexcept
	{
		switch (a_msg->type) {
		case SFSE::MessagingInterface::kPostDataLoad:
			{
				Settings::Init();
				Settings::LoadBaseSkeletons();
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

bool g_implInitialized = ([]() {
	Settings::ImplFunctions& funcs = Settings::GetImplFunctions();
	funcs.GetSkeletonBonesForActor = [](Settings::SkeletonDescriptor&, RE::Actor*) {};
	funcs.LoadOtherAnimationFile = [](const Animation::AnimID&, const Animation::OzzSkeleton*) -> std::unique_ptr<Animation::IBasicAnimation> { return nullptr; };
	return true;
})();

extern "C" DLLEXPORT bool SFSEAPI SFSEPlugin_Load(const SFSE::LoadInterface* a_sfse)
{
	SFSE::Init(a_sfse, true);
	logger::info("Starfield Offset: {:X}", REL::Module::get().base());

	Util::Trampoline::ProcessHooks();
	SFSE::SetPapyrusCallback(&BindPapyrusScripts);
	SFSE::GetMessagingInterface()->RegisterListener(MessageCallback);
	Commands::NAFCommand::RegisterKeybinds();
	return true;
}