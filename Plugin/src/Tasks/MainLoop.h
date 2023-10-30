#pragma once

namespace Tasks
{
	class MainLoop : public SFSE::ITaskDelegate, public RE::BSTEventSink<RE::MenuModeChangeEvent>
	{
	public:
		bool menuMode = false;

		static MainLoop* GetSingleton();
		virtual void Run();
		virtual void Destroy();
		virtual RE::BSEventNotifyControl ProcessEvent(const RE::MenuModeChangeEvent& a_event, RE::BSTEventSource<RE::MenuModeChangeEvent>* a_source);
	};
}