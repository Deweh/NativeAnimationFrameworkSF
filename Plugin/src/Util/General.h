#pragma once
#include "Trampoline.h"

namespace Util
{
	template <typename T, typename M = std::mutex, typename WL = std::unique_lock<M>, typename RL = std::shared_lock<M>>
	class Guarded
	{
	public:

		template <typename L>
		class DataLock
		{
		private:
			T* data;
			L lock;

		public:
			DataLock(T& data, M& mutex) :
				data(&data), lock(mutex)
			{}

			void unlock() {
				lock.unlock();
				data = nullptr;
			}

			T* get() const {
				return data;
			}

			T* operator->() const {
				return data;
			}

			T& operator*() const {
				return *data;
			}
		};

		DataLock<WL> lock() {
			return DataLock<WL>(data, mutex);
		}

		DataLock<RL> lock_read_only() {
			return DataLock<RL>(data, mutex);
		}

		M& internal_mutex() {
			return mutex;
		}

		T& internal_data() {
			return data;
		}
	private:
		T data;
		M mutex;
	};

	template <typename Signature>
	class VFuncHook
	{
	};

	template <typename R, typename... Args>
	class VFuncHook<R(Args...)>
	{
	public:
		typedef R (*func_t)(Args...);

		VFuncHook(size_t a_id, size_t a_idx, const char* a_name, func_t thunk)
		{
			Util::Trampoline::AddHook(0, [this, a_id = a_id, a_idx = a_idx, thunk = thunk, a_name = a_name](SFSE::Trampoline&) {
				REL::Relocation reloc{ REL::ID(a_id) };
				_original = reinterpret_cast<func_t>(reloc.write_vfunc(a_idx, thunk));
				INFO("Applied {} vfunc hook at {:X}", a_name, reloc.address() + (a_idx * 0x8));
			});
		}
		
		inline R operator()(Args... a_args)
		{
			return _original(a_args...);
		}

		func_t _original;
	};

	template <typename Signature>
	class Call5Hook
	{
	};

	template <typename R, typename... Args>
	class Call5Hook<R(Args...)>
	{
	public:
		typedef R (*func_t)(Args...);

		Call5Hook(size_t a_id, const ptrdiff_t a_offset, const char* a_name, func_t thunk)
		{
			Util::Trampoline::AddHook(14, [this, a_id = a_id, a_offset = a_offset, thunk = thunk, a_name = a_name](SFSE::Trampoline& t) {
				REL::Relocation reloc{ REL::ID(a_id), a_offset };
				_original = reinterpret_cast<func_t>(t.write_call<5>(reloc.address(), thunk));
				INFO("Applied {} call hook at {:X}", a_name, reloc.address());
			});
		}

		inline R operator()(Args... a_args)
		{
			return _original(a_args...);
		}

		func_t _original;
	};
}