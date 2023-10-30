#include "MainLoop.h"

namespace Tasks
{
	MainLoop* MainLoop::GetSingleton() {
		static MainLoop singleton;
		return &singleton;
	}

	void MainLoop::Run() {
		//graphManager->Update(menuMode);
	}

	void MainLoop::Destroy() {
		delete this;
	}

	RE::BSEventNotifyControl MainLoop::ProcessEvent(const RE::MenuModeChangeEvent& a_event, RE::BSTEventSource<RE::MenuModeChangeEvent>* a_source)
	{
		menuMode = a_event.enteringMenuMode;
		return RE::BSEventNotifyControl::kContinue;
	}
}