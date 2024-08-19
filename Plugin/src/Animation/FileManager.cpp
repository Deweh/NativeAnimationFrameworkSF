#include "FileManager.h"
#include "Util/String.h"
#include "Serialization/GLTFImport.h"
#include "Serialization/BlendGraphImport.h"
#include "Settings/Settings.h"
#include "Ozz.h"
#include "Util/Timing.h"

namespace Animation
{
	inline bool FileIsBlendGraph(const FileID& a_id)
	{
		return a_id.QPath().ends_with(Serialization::BlendGraphImport::FILE_EXTENSION);
	}

	FileManager::FileManager()
	{
		workerThread = std::jthread(&FileManager::DoProcessRequests, this);
	}

	FileManager* FileManager::GetSingleton()
	{
		static FileManager instance;
		return &instance;
	}

	void FileManager::RequestAnimation(const FileID& a_id, const std::string& a_skeleton, std::weak_ptr<FileRequesterBase> a_requester)
	{
		AnimID aID{ .file = a_id, .skeleton = a_skeleton };
		if (auto anim = GetLoadedAnimation(aID); anim != nullptr) {
			NotifyAnimationRequested(a_requester, a_id, false);
			NotifyAnimationReady(a_requester, a_id, anim);
			return;
		}

		auto reqData = requests.lock();
		for (auto iter = reqData->begin(); iter != reqData->end(); iter++) {
			if (iter->anim == aID && iter->requester.lock() == a_requester.lock()) {
				return;
			}
		}
		NotifyAnimationRequested(a_requester, a_id, false);
		reqData->push_back(RequestData{ .requester = a_requester, .anim = aID });
		ProcessRequests();
	}

	bool FileManager::CancelAnimationRequest(const FileID& a_id, std::weak_ptr<FileRequesterBase> a_requester)
	{
		auto req_lock = a_requester.lock();
		auto reqData = requests.lock();
		for (auto iter = reqData->begin(); iter != reqData->end(); iter++) {
			if (iter->anim.file == a_id && iter->requester.lock() == req_lock) {
				reqData->erase(iter);
				return true;
			}
		}
		return false;
	}

	std::shared_ptr<IAnimationFile> FileManager::DemandAnimation(const FileID& a_id, const std::string_view a_skeleton, bool a_noBlendGraphs)
	{
		bool isBlendGraph = FileIsBlendGraph(a_id);

		if (a_noBlendGraphs && isBlendGraph) {
			return nullptr;
		}

		AnimID aID{ .file = a_id, .skeleton = std::string(a_skeleton) };
		if (auto anim = GetLoadedAnimation(aID); anim != nullptr) {
			return anim;
		}

		std::shared_ptr<IAnimationFile> result{ nullptr };
		if (isBlendGraph) {
			result = DoLoadBlendGraph(aID);
		} else {
			result = DoLoadAnimation(aID);
		}
		
		if (result != nullptr) {
			return InsertLoadedAnimation(aID, result);
		}

		return nullptr;
	}

	void FileManager::OnAnimationDestroyed(IAnimationFile* a_anim)
	{
		auto loaded = loadedAnimations.lock();
		for (auto iter = loaded->begin(); iter != loaded->end(); iter++) {
			if (iter->second.raw == a_anim) {
				SFSE::GetTaskInterface()->AddTask([id = iter->first, inst = this]() {
					inst->SendEvent({
						.file = id.file,
						.skeleton = id.skeleton,
						.loaded = false
					});
				});
				loaded->erase(iter);
				return;
			}
		}
	}

	void FileManager::GetAllLoadedAnimations(std::vector<std::pair<AnimID, std::weak_ptr<IAnimationFile>>>& a_animsOut)
	{
		a_animsOut.clear();
		auto loaded = loadedAnimations.lock();
		for (auto& iter : *loaded) {
			a_animsOut.emplace_back(iter.first, iter.second.shared_handle);
		}
	}

	void FileManager::ProcessRequests()
	{
		workCV.notify_one();
	}

	void FileManager::DoProcessRequests()
	{
		RequestData nextReq;
		std::unique_lock l{ requests.internal_mutex(), std::defer_lock };
		auto& reqData = requests.internal_data();

		while (true) {
			l.lock();
			if (reqData.empty()) {
				workCV.wait(l, [&]() { return !reqData.empty(); });
			}
			nextReq = reqData.front();
			reqData.pop_front();
			l.unlock();

			std::shared_ptr<IAnimationFile> loadedFile{ nullptr };
			if (FileIsBlendGraph(nextReq.anim.file)) {
				loadedFile = DoLoadBlendGraph(nextReq.anim);
			} else {
				loadedFile = DoLoadAnimation(nextReq.anim);
			}

			if (loadedFile == nullptr) {
				ReportFailedToLoadAnimation(nextReq);
				continue;
			}

			ReportAnimationLoaded(nextReq, loadedFile);
		}
	}

	std::shared_ptr<Procedural::PGraph> FileManager::DoLoadBlendGraph(const AnimID& a_id)
	{
		auto start = Util::Timing::HighResTimeNow();
		auto result = Serialization::BlendGraphImport::LoadGraph(Util::String::GetDataPath() / a_id.file.QPath(), a_id.skeleton);
		if (result) {
			result->extra.loadTime = Util::Timing::HighResTimeDiffMilliSec(start);
			result->extra.id = a_id;
		}
		return result;
	}

	std::shared_ptr<OzzAnimation> FileManager::DoLoadAnimation(const AnimID& a_id)
	{
		auto start = Util::Timing::HighResTimeNow();
		auto file = Serialization::GLTFImport::LoadGLTF(Util::String::GetDataPath() / a_id.file.QPath());
		if (!file) {
			return nullptr;
		}

		fastgltf::Animation* storedAnim = nullptr;
		auto id = a_id.file.QID();
		if (id.empty()) {
			if (!file->asset.animations.empty()) {
				storedAnim = std::addressof(file->asset.animations[0]);
			} else {
				return nullptr;
			}
		} else {
			for (auto& a : file->asset.animations) {
				if (Util::String::ToLower(a.name) == id) {
					storedAnim = std::addressof(a);
					break;
				}
			}
			if (storedAnim == nullptr) {
				return nullptr;
			}
		}

		auto result = Serialization::GLTFImport::CreateRuntimeAnimation(file.get(), storedAnim, Settings::GetSkeleton(a_id.skeleton)->data.get());
		if (result) {
			result->extra.loadTime = Util::Timing::HighResTimeDiffMilliSec(start);
			result->extra.id = a_id;
		}
		return result;
	}

	std::shared_ptr<IAnimationFile> FileManager::GetLoadedAnimation(const AnimID& a_id)
	{
		auto anims = loadedAnimations.lock();
		if (auto iter = anims->find(a_id); iter != anims->end()) {
			return iter->second.shared_handle.lock();
		}
		return nullptr;
	}

	std::shared_ptr<IAnimationFile> FileManager::InsertLoadedAnimation(const AnimID& a_id, std::shared_ptr<IAnimationFile> a_anim)
	{
		auto anims = loadedAnimations.lock();
		auto insertedAnim = anims->insert(std::make_pair(a_id, LoadedAnimData{ .raw = a_anim.get(), .shared_handle = a_anim })).first->second.shared_handle.lock();
		anims.unlock();
		auto reqData = requests.lock();
		for (auto iter = reqData->begin(); iter != reqData->end();) {
			if (iter->anim == a_id) {
				NotifyAnimationReady(iter->requester, a_id.file, insertedAnim);
				iter = reqData->erase(iter);
			} else {
				iter++;
			}
		}
		SFSE::GetTaskInterface()->AddTask([id = a_id, inst = this]() {
			inst->SendEvent({
				.file = id.file,
				.skeleton = id.skeleton,
				.loaded = true
			});
		});
		return insertedAnim;
	}

	void FileManager::ReportAnimationLoaded(RequestData& a_req, std::shared_ptr<IAnimationFile> a_anim)
	{
		auto insertedAnim = InsertLoadedAnimation(a_req.anim, a_anim);
		NotifyAnimationReady(a_req.requester, a_req.anim.file, insertedAnim);
	}

	void FileManager::ReportFailedToLoadAnimation(const RequestData& a_req)
	{
		NotifyAnimationReady(a_req.requester, a_req.anim.file, nullptr);
	}

	void DoNotifyAnimationReady(std::weak_ptr<FileRequesterBase> a_requester, const FileID& a_id, std::shared_ptr<IAnimationFile> a_anim)
	{
		if (auto r = a_requester.lock(); r != nullptr) {
			r->OnAnimationReady(a_id, a_anim);
		}
	}

	void DoNotifyAnimationRequested(std::weak_ptr<FileRequesterBase> a_requester, const FileID& a_id)
	{
		if (auto r = a_requester.lock(); r != nullptr) {
			r->OnAnimationRequested(a_id);
		}
	}

	void FileManager::NotifyAnimationReady(std::weak_ptr<FileRequesterBase> a_requester, const FileID& a_id, std::shared_ptr<IAnimationFile> a_anim, bool a_queue)
	{
		if (a_queue) {
			SFSE::GetTaskInterface()->AddTask(std::bind(DoNotifyAnimationReady, a_requester, a_id, a_anim));
		} else {
			DoNotifyAnimationReady(a_requester, a_id, a_anim);
		}
	}

	void FileManager::NotifyAnimationRequested(std::weak_ptr<FileRequesterBase> a_requester, const FileID& a_id, bool a_queue)
	{
		if (a_queue) {
			SFSE::GetTaskInterface()->AddTask(std::bind(DoNotifyAnimationRequested, a_requester, a_id));
		} else {
			DoNotifyAnimationRequested(a_requester, a_id);
		}
	}
}