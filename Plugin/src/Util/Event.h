#pragma once
#include "Util/General.h"

namespace Util::Event
{
	enum ListenerStatus : uint8_t
	{
		kUnchanged = 0,
		kRemove = 1
	};

	template <typename E>
	class ListenerBase
	{
	public:
		virtual ListenerStatus OnEvent(E&) = 0;
		virtual ~ListenerBase() = default;
	};

	template <typename E>
	class Dispatcher
	{
	public:
		void AddListener(ListenerBase<E>* a_listener)
		{
			auto d = data.lock();
			auto iter = std::find(d->listeners.begin(), d->listeners.end(), a_listener);
			if (iter == d->listeners.end()) {
				d->listeners.push_back(a_listener);
			}
		}

		void RemoveListener(ListenerBase<E>* a_listener)
		{
			auto d = data.lock();
			auto iter = std::find(d->listeners.begin(), d->listeners.end(), a_listener);
			if (iter != d->listeners.end()) {
				d->listeners.erase(iter);
			}
		}

		void SendEvent(E&& a_event)
		{
			auto d = data.lock();
			for (auto iter = d->listeners.begin(); iter != d->listeners.end(); iter++) {
				if ((*iter)->OnEvent(a_event) == ListenerStatus::kRemove) {
					iter = d->listeners.erase(iter);
				}
			}
		}

	private:
		struct InternalData
		{
			std::vector<ListenerBase<E>*> listeners;
		};

		Guarded<InternalData> data;
	};

	template<typename... Ts>
	class MultiDispatcher : public Dispatcher<Ts>...
	{
	public:
		using Dispatcher<Ts>::AddListener...;
		using Dispatcher<Ts>::RemoveListener...;
		using Dispatcher<Ts>::SendEvent...;
	};

	template <typename E>
	class Listener : public ListenerBase<E>
	{
	public:
		using ListenerStatus = Util::Event::ListenerStatus;

		void RegisterForEvent(Dispatcher<E>* a_dispatcher)
		{
			UnregisterForEvent();
			eventDispatcher = a_dispatcher;
			a_dispatcher->AddListener(this);
		}

		void UnregisterForEvent()
		{
			auto dispatcherVal = eventDispatcher.load();
			if (dispatcherVal) {
				dispatcherVal->RemoveListener(this);
				eventDispatcher = nullptr;
			}
		}

		virtual ~Listener()
		{
			UnregisterForEvent();
		}

	protected:
		std::atomic<Dispatcher<E>*> eventDispatcher = nullptr;
	};

	template <typename... Ts>
	class MultiListener : public Listener<Ts>...
	{
	public:
		template <typename T>
		void RegisterForEvent(Dispatcher<T>* a_dispatcher)
		{
			Listener<T>::RegisterForEvent(a_dispatcher);
		}

		template <typename T>
		void UnregisterForEvent()
		{
			Listener<T>::UnregisterForEvent();
		}
	};
}