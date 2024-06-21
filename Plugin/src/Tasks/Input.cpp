#include "Input.h"

namespace Tasks
{
	typedef void (*PerformInputProcessingFunc)(const RE::PlayerCamera* a_camera, const RE::InputEvent* a_queueHead);
	PerformInputProcessingFunc OriginalPerformProcessing;

	Input* Input::GetSingleton()
	{
		static Input instance;
		return &instance;
	}

	void Input::RegisterForKey(BS_BUTTON_CODE a_key, ButtonCallback a_callback)
	{
		callbacks[a_key] = a_callback;
	}

	void PerformInputProcessing(const RE::PlayerCamera* a_camera, const RE::InputEvent* a_queueHead)
	{
		static Input* m = Input::GetSingleton();
		for (auto curEvent = a_queueHead; curEvent != nullptr && curEvent->status != RE::InputEvent::Stop; curEvent = curEvent->next) {
			if (curEvent->eventType != RE::InputEvent::Button) {
				continue;
			}

			auto btnEvent = static_cast<const RE::ButtonEvent*>(curEvent);
			bool downState;

			if (btnEvent->held <= 0.0f && btnEvent->value > 0.0f) {
				downState = true;
			} else if (btnEvent->value <= 0.0f) {
				downState = false;
			} else {
				continue;
			}

			if (auto iter = m->callbacks.find(btnEvent->idCode); iter != m->callbacks.end()) {
				iter->second(static_cast<Input::BS_BUTTON_CODE>(btnEvent->idCode), downState);
			}
		}
		OriginalPerformProcessing(a_camera, a_queueHead);
	}

	void Input::InstallHooks()
	{
		REL::Relocation<uintptr_t> cameraVtbl{ REL::ID(428955) };
		OriginalPerformProcessing = reinterpret_cast<PerformInputProcessingFunc>(cameraVtbl.write_vfunc(1, &PerformInputProcessing));
	}
}