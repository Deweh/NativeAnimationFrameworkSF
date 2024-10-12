#pragma once

namespace Animation
{
	class PoseCache
	{
	public:
		class Handle
		{
		public:
			Handle(const Handle&&) = delete;
			Handle(const Handle&) = delete;
			Handle(Handle&) = delete;
			Handle(Handle&&) noexcept;
			Handle() = default;

			~Handle();

			void operator=(Handle&&) noexcept;

			std::span<ozz::math::SoaTransform> get();
			ozz::span<ozz::math::SoaTransform> get_ozz();
			void reset();
			bool is_valid() const;

		protected:
			friend class PoseCache;
			Handle(PoseCache* a_owner, size_t a_idx);

		private:
			PoseCache* _owner{ nullptr };
			size_t _impl = UINT64_MAX;
		};

		void set_pose_size(size_t a_size);
		void reserve(size_t a_numPoses);
		Handle acquire_handle();
		size_t transforms_capacity() const;
		
	protected:
		friend class Handle;
		void release_handle(size_t a_idx);
		std::span<ozz::math::SoaTransform> get_span(size_t a_idx);
		ozz::span<ozz::math::SoaTransform> get_span_ozz(size_t a_idx);

	private:
		size_t _pose_size;
		std::vector<size_t> freeIdxs;
		std::vector<ozz::math::SoaTransform> _cache;
	};
}