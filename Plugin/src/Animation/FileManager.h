#pragma once
#include "Util/General.h"

namespace Animation
{
	class OzzAnimation;

	class FileID
	{
	private:
		std::string filePath;
		std::string id;

	public:
		const std::string_view QPath() const;
		const std::string_view QID() const;

		FileID() = default;
		FileID(const std::string_view a_filePath, const std::string_view a_id);
		bool operator==(const FileID& a_rhs) const;
		bool operator<(const FileID& a_rhs) const;
	};

	class FileRequesterBase
	{
	public:
		std::weak_ptr<FileRequesterBase> requesterHandle;

		virtual void OnAnimationReady(const FileID& a_id, std::shared_ptr<OzzAnimation> a_anim) = 0;
		virtual void OnAnimationRequested(const FileID& a_id) = 0;
		virtual ~FileRequesterBase() noexcept = default;
	};

	class FileManager
	{
	public:
		struct AnimID
		{
			FileID file;
			std::string skeleton;

			bool operator==(const AnimID& other) const;
			bool operator<(const AnimID& other) const;
		};

		struct RequestData
		{
			std::weak_ptr<FileRequesterBase> requester;
			AnimID anim;
		};

		struct ActiveRequestData
		{
			RequestData current;
			bool cancelled = false;
		};

		struct LoadedAnimData
		{
			OzzAnimation* raw;
			std::weak_ptr<OzzAnimation> shared_handle;
		};

		FileManager();
		static FileManager* GetSingleton();
		void RequestAnimation(const FileID& a_id, const std::string_view a_skeleton, std::weak_ptr<FileRequesterBase> a_requester);
		bool CancelAnimationRequest(const FileID& a_id, std::weak_ptr<FileRequesterBase> a_requester);
		std::shared_ptr<OzzAnimation> DemandAnimation(const FileID& a_id, const std::string_view a_skeleton);
		void OnAnimationDestroyed(OzzAnimation* a_anim);

	protected:
		void ProcessRequests();
		void DoProcessRequests();
		std::shared_ptr<OzzAnimation> DoLoadAnimation(const AnimID& a_id);
		std::shared_ptr<OzzAnimation> GetLoadedAnimation(const AnimID& a_id);
		std::shared_ptr<OzzAnimation> InsertLoadedAnimation(const AnimID& a_id, std::shared_ptr<OzzAnimation> a_anim);
		void ReportAnimationLoaded(RequestData& a_req, std::shared_ptr<OzzAnimation> a_anim);
		void ReportFailedToLoadAnimation(const RequestData& a_req);

		static void NotifyAnimationReady(std::weak_ptr<FileRequesterBase> a_requester, const FileID& a_id, std::shared_ptr<OzzAnimation> a_anim);
		static void NotifyAnimationRequested(std::weak_ptr<FileRequesterBase> a_requester, const FileID& a_id);

	private:
		std::jthread workerThread;
		std::condition_variable workCV;
		Util::Guarded<std::list<RequestData>> requests;
		Util::Guarded<std::map<AnimID, LoadedAnimData>> loadedAnimations;
	};
}