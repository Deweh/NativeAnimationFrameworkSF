#include "Input.h"

namespace Tasks
{
	Input* Input::GetSingleton()
	{
		static Input instance;
		return &instance;
	}

	void Input::RegisterForKey(BS_BUTTON_CODE a_key, ButtonCallback a_callback)
	{
		callbacks[a_key] = a_callback;
	}

	static Util::VFuncHook<void(const RE::PlayerCamera*, const RE::InputEvent*)> PerformInputProcessingHook(459729, 0x1, "PlayerCamera::PerformInputProcessing",
		[](const RE::PlayerCamera* a_camera, const RE::InputEvent* a_queueHead) {
			static Input* m = Input::GetSingleton();
			for (auto curEvent = a_queueHead; curEvent != nullptr && curEvent->status != RE::InputEvent::Status::kStop; curEvent = curEvent->next) {
				if (curEvent->eventType != RE::InputEvent::EventType::kButton) {
					continue;
				}

				auto btnEvent = static_cast<const RE::ButtonEvent*>(curEvent);
				bool downState;

				if (btnEvent->heldDownSecs <= 0.0f && btnEvent->value > 0.0f) {
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
			PerformInputProcessingHook(a_camera, a_queueHead);
		});
}