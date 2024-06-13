#pragma once

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
}