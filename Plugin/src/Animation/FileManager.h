#pragma once
#include "Util/General.h"
#include "Util/Event.h"
#include "FileID.h"
#include "IAnimationFile.h"

namespace Animation
{
	struct OzzAnimation;
	struct OzzSkeleton;

	namespace Procedural
	{
		class PGraph;
	}

	class FileRequesterBase : public std::enable_shared_from_this<FileRequesterBase>
	{
	public:
		virtual void OnAnimationReady(const FileID& a_id, std::shared_ptr<IAnimationFile> a_anim) = 0;
		virtual void OnAnimationRequested(const FileID& a_id) = 0;
		virtual ~FileRequesterBase() noexcept = default;
	};

	struct FileLoadUnloadEvent
	{
		FileID file;
		std::string skeleton;
		bool loaded;
	};

	class FileManager : public Util::Event::Dispatcher<FileLoadUnloadEvent>
	{
	public:
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
			IAnimationFile* raw;
			std::weak_ptr<IAnimationFile> shared_handle;
		};

		FileManager();
		static FileManager* GetSingleton();
		void RequestAnimation(const FileID& a_id, const std::string& a_skeleton, std::weak_ptr<FileRequesterBase> a_requester);
		bool CancelAnimationRequest(const FileID& a_id, std::weak_ptr<FileRequesterBase> a_requester);
		std::shared_ptr<IAnimationFile> DemandAnimation(const FileID& a_id, const std::string_view a_skeleton, bool a_noBlendGraphs);
		void OnAnimationDestroyed(IAnimationFile* a_anim);
		void GetAllLoadedAnimations(std::vector<std::pair<AnimID, std::weak_ptr<IAnimationFile>>>& a_animsOut);

	protected:
		void ProcessRequests();
		void DoProcessRequests();
		std::shared_ptr<Procedural::PGraph> DoLoadBlendGraph(const AnimID& a_id);
		std::shared_ptr<OzzAnimation> DoLoadAnimation(const AnimID& a_id);
		std::shared_ptr<IAnimationFile> GetLoadedAnimation(const AnimID& a_id);
		std::shared_ptr<IAnimationFile> InsertLoadedAnimation(const AnimID& a_id, std::shared_ptr<IAnimationFile> a_anim);
		void ReportAnimationLoaded(RequestData& a_req, std::shared_ptr<IAnimationFile> a_anim);
		void ReportFailedToLoadAnimation(const RequestData& a_req);

		static void NotifyAnimationReady(std::weak_ptr<FileRequesterBase> a_requester, const FileID& a_id, std::shared_ptr<IAnimationFile> a_anim, bool a_queue = true);
		static void NotifyAnimationRequested(std::weak_ptr<FileRequesterBase> a_requester, const FileID& a_id, bool a_queue = true);

	private:
		std::jthread workerThread;
		std::condition_variable workCV;
		Util::Guarded<std::list<RequestData>> requests;
		Util::Guarded<std::map<AnimID, LoadedAnimData>> loadedAnimations;
	};
}