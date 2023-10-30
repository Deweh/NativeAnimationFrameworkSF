#pragma once

namespace Animation
{
	//K = Time Type, T = Element Type, ITP = Interpolation Functor (T prev, T next, K normalizedTime, T& outValue)
	template <typename K, typename T, typename ITP>
	class Timeline
	{
	public:
		ITP interpolator;
		std::map<K, T> keys;

		void Init() {
			cachedNextIter = keys.begin();
			cachedPrevIter = keys.begin();
		}

		void GetValueAtTime(K a_time, T& a_outVal) {
			if (keys.size() < 1) {
				a_outVal = T();
				return;
			}

			if (cachedNextIter->first < a_time || cachedPrevIter->first > a_time) {
				auto nextKey = NextOrLowerBound(a_time);

				if (nextKey == keys.end()) {
					a_outVal = std::prev(nextKey)->second;
					return;
				} else if (nextKey == keys.begin() || keys.size() < 2) {
					a_outVal = nextKey->second;
					return;
				} else {
					cachedNextIter = nextKey;
					cachedPrevIter = std::prev(nextKey);
				}
			}

			if (a_time == cachedPrevIter->first) {
				a_outVal = cachedPrevIter->second;
			} else if (a_time == cachedNextIter->first) {
				a_outVal = cachedNextIter->second;
			} else {
				auto normalizedTime = (a_time - cachedPrevIter->first) / (cachedNextIter->first - cachedPrevIter->first);
				interpolator(cachedPrevIter->second, cachedNextIter->second, normalizedTime, a_outVal);
			}
		}

	private:
		std::map<K, T>::iterator cachedNextIter;
		std::map<K, T>::iterator cachedPrevIter;

		std::map<K, T>::iterator NextOrLowerBound(K a_time)
		{
			auto prevKey = cachedNextIter;
			auto nextKey = std::next(cachedNextIter);
			if (nextKey == keys.end()) {
				nextKey = prevKey;
			}

			if (nextKey->first < a_time || prevKey->first > a_time) {
				return keys.lower_bound(a_time);
			} else {
				return nextKey;
			}
		}
	};
}