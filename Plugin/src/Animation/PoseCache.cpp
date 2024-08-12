#include "PoseCache.h"

namespace Animation
{
	PoseCache::Handle::Handle(Handle&& a_rhs) noexcept
	{
		operator=(std::forward<Handle&&>(a_rhs));
	}

	PoseCache::Handle::Handle(PoseCache* a_owner, size_t a_idx)
	{
		_owner = a_owner;
		_impl = a_idx;
	}

	PoseCache::Handle::~Handle()
	{
		reset();
	}

	inline void PoseCache::Handle::operator=(Handle&& a_rhs) noexcept
	{
		reset();
		_impl = a_rhs._impl;
		_owner = a_rhs._owner;
		a_rhs._impl = UINT64_MAX;
		a_rhs._owner = nullptr;
	}

	std::span<ozz::math::SoaTransform> PoseCache::Handle::get()
	{
		if (_owner) {
			return _owner->get_span(_impl);
		} else {
			return {};
		}
	}

	ozz::span<ozz::math::SoaTransform> PoseCache::Handle::get_ozz()
	{
		if (_owner) {
			return _owner->get_span_ozz(_impl);
		} else {
			return {};
		}
	}

	void PoseCache::Handle::reset()
	{
		if (_owner != nullptr && _impl != UINT64_MAX) {
			_owner->release_handle(_impl);
			_impl = UINT64_MAX;
			_owner = nullptr;
		}
	}

	void PoseCache::set_pose_size(size_t a_size)
	{
		_pose_size = a_size;
	}

	void PoseCache::reserve(size_t a_numPoses)
	{
		_cache.reserve(a_numPoses * _pose_size);
	}

	PoseCache::Handle PoseCache::acquire_handle()
	{
		if (!freeIdxs.empty()) {
			size_t targetIdx = freeIdxs.back();
			freeIdxs.pop_back();

			return Handle{ this, targetIdx };
		} else {
			size_t start = _cache.size();
			_cache.resize(start + _pose_size);
			
			return Handle{ this, start };
		}
	}

	size_t PoseCache::transforms_capacity() const
	{
		return _cache.capacity();
	}

	void PoseCache::release_handle(size_t a_idx)
	{
		freeIdxs.push_back(a_idx);
	}

	std::span<ozz::math::SoaTransform> PoseCache::get_span(size_t a_idx)
	{
		return { &_cache[a_idx], _pose_size };
	}

	ozz::span<ozz::math::SoaTransform> PoseCache::get_span_ozz(size_t a_idx)
	{
		return { &_cache[a_idx], _pose_size };
	}
}