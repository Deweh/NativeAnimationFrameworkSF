#pragma once
#include "Util/Event.h"
#include "Util/General.h"
#include "Util/VM.h"
#include "Animation/Sequencer.h"

namespace Papyrus
{
	enum EventType : uint8_t
	{
		kPhaseBegin,
		kSequenceEnd,
		kTotal
	};

	class EventManager :
		public Util::Event::MultiListener<Animation::SequencePhaseChangeEvent>
	{
	public:
		struct Registration
		{
			std::string scriptName;
			std::string functionName;
		};

		struct InternalData
		{
			using RegistrationMap = std::unordered_map<size_t, Registration>;

			std::array<RegistrationMap, EventType::kTotal> scriptRegistrations;
		};

		EventManager();

		static EventManager* GetSingleton();

		template <class... Args>
		void SendScriptEvent(EventType a_type, Args&&... a_args)
		{
			auto d = data.lock();
			auto& scripts = d->scriptRegistrations[a_type];
			auto vm = RE::BSScript::Internal::VirtualMachine::GetSingleton();
			for (auto& reg : scripts)
			{
				Util::VM::CallFunction(vm, reg.first, reg.second.scriptName, reg.second.functionName, nullptr, std::forward<Args&&>(a_args)...);
			}
		}

		void RegisterScript(EventType a_type, RE::BSScript::Object* a_script, const RE::BSFixedString& a_funcName);
		void UnregisterScript(EventType a_type, RE::BSScript::Object* a_script);

		virtual ListenerStatus OnEvent(Animation::SequencePhaseChangeEvent& a_event);

		void Reset();

	private:
		Util::Guarded<InternalData> data;
	};
}