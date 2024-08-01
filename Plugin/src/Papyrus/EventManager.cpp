#include "EventManager.h"
#include "Animation/GraphManager.h"

namespace Papyrus
{
	static EventManager em_singleton;

	EventManager::EventManager()
	{
		RegisterForEvent<Animation::SequencePhaseChangeEvent>(Animation::GraphManager::GetSingleton());
	}

	EventManager* EventManager::GetSingleton()
	{
		return &em_singleton;
	}

	void EventManager::RegisterScript(EventType a_type, RE::BSScript::Object* a_script, const RE::BSFixedString& a_funcName)
	{
		auto d = data.lock();
		auto& iter = d->scriptRegistrations[a_type][a_script->GetHandle()];
		iter.scriptName = a_script->type->name;
		iter.functionName = a_funcName;
	}

	void EventManager::UnregisterScript(EventType a_type, RE::BSScript::Object* a_script)
	{
		auto d = data.lock();
		d->scriptRegistrations[a_type].erase(a_script->GetHandle());
	}

	Util::Event::ListenerStatus EventManager::OnEvent(Animation::SequencePhaseChangeEvent& a_event)
	{
		if (a_event.exiting) {
			SendScriptEvent(EventType::kSequenceEnd, a_event.target.get(), a_event.name);
		} else {
			SendScriptEvent(EventType::kPhaseBegin, a_event.target.get(), static_cast<int32_t>(a_event.index), a_event.name);
		}
		return Util::Event::ListenerStatus::kUnchanged;
	}

	void EventManager::Reset()
	{
		auto d = data.lock();
		d->scriptRegistrations.clear();
	}
}