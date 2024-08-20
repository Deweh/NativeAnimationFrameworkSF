#include "Manager.h"
#include "Util/Trampoline.h"

namespace Animation::Face
{
	void GraphStub::BeginTween(float a_duration, RE::BSFaceGenAnimationData* a_data)
	{
		for (size_t i = 0; i < RE::BSFaceGenAnimationData::morphSize; i++) {
			snapshot[i] = a_data->morphs[i];
		}
		transition.duration = a_duration;
		transition.time = 0.0f;
		transition.state = kTween;
	}

	void GraphStub::BeginOutTransition(float a_duration)
	{
		transition.duration = a_duration;
		transition.time = 0.0f;
		transition.state = kOut;
	}

	bool GraphStub::Update(float a_deltaTime, RE::BSFaceGenAnimationData* a_data)
	{
		if (transition.state == kNone) {
			for (size_t i = 0; i < RE::BSFaceGenAnimationData::morphSize; i++) {
				a_data->morphs[i] = current[i];
			}
			return false;
		}

		transition.time += a_deltaTime;

		if (transition.time >= transition.duration) {
			if (transition.state == kOut) {
				return true;
			} else {
				transition.state = kNone;
				return Update(a_deltaTime, a_data);
			}
		}

		float ratio = transition.ease(transition.time / transition.duration);

		if (transition.state == kTween) {
			for (size_t i = 0; i < RE::BSFaceGenAnimationData::morphSize; i++) {
				a_data->morphs[i] = std::lerp(snapshot[i], current[i], ratio);
			}
		} else {
			for (size_t i = 0; i < RE::BSFaceGenAnimationData::morphSize; i++) {
				a_data->morphs[i] = std::lerp(current[i], a_data->morphs[i], ratio);
			}
		}
		return false;
	}

	Manager* Manager::GetSingleton()
	{
		static Manager instance;
		return &instance;
	}

	void Manager::OnAnimDataChange(RE::BSFaceGenAnimationData* a_old, RE::BSFaceGenAnimationData* a_new)
	{
		auto d = data.lock();
		bool noBlink = false;
		std::shared_ptr<MorphData> md = nullptr;

		if (a_old != nullptr) {
			if (auto iter = d->controlledDatas.find(a_old); iter != d->controlledDatas.end()) {
				md = iter->second;
				d->controlledDatas.erase(iter);
			}
			if (auto iter = d->noBlink.find(a_old); iter != d->noBlink.end()) {
				noBlink = true;
				d->noBlink.erase(iter);
			}
		}

		if (a_new != nullptr) {
			if (md != nullptr) {
				d->controlledDatas[a_new] = md;
			}
			if (noBlink) {
				d->noBlink.insert(a_new);
			}
		}
	}

	void Manager::SetNoBlink(RE::BSFaceGenAnimationData* a_data, bool a_noBlink)
	{
		if (!a_data) {
			return;
		}

		if (a_noBlink) {
			data.lock()->noBlink.insert(a_data);
		} else {
			data.lock()->noBlink.erase(a_data);
		}
	}

	void Manager::AttachMorphData(RE::BSFaceGenAnimationData* a_data, std::shared_ptr<MorphData> a_morphs, float a_transitionTime)
	{
		if (!a_data) {
			return;
		}

		data.lock()->controlledDatas[a_data] = a_morphs;
		a_morphs->lock()->BeginTween(a_transitionTime, a_data);
	}

	void Manager::DetachMorphData(RE::BSFaceGenAnimationData* a_data, float a_transitionTime)
	{
		if (auto m = GetMorphData(a_data); m != nullptr) {
			m->lock()->BeginOutTransition(a_transitionTime);
		}
	}

	void Manager::DoDetach(RE::BSFaceGenAnimationData* a_data)
	{
		data.lock()->controlledDatas.erase(a_data);
	}

	std::shared_ptr<MorphData> Manager::GetMorphData(RE::BSFaceGenAnimationData* a_data)
	{
		auto d = data.lock_read_only();
		if (auto iter = d->controlledDatas.find(a_data); iter != d->controlledDatas.end()) {
			return iter->second;
		}
		return nullptr;
	}

	void Manager::Reset()
	{
		auto d = data.lock();
		d->noBlink.clear();
		d->controlledDatas.clear();
	}

	static Util::Call5Hook<void(RE::BSFaceGenAnimationData*, float)> BlinkUpdateHook(113796, 0x147, "BSFaceGenAnimationData::UpdateBlinking",
		[](RE::BSFaceGenAnimationData* a1, float a2) {
			static Manager* m = Manager::GetSingleton();
			if (!m->data.lock_read_only()->noBlink.contains(a1)) {
				BlinkUpdateHook(a1, a2);
			} else {
				//eyeClosed L/R
				a1->morphs[25] = 0.0f;
				a1->morphs[26] = 0.0f;
				a1->morphs2[25] = 0.0f;
				a1->morphs2[26] = 0.0f;
			}
		});

	static Util::Call5Hook<bool(RE::BSFaceGenAnimationData*, float, bool)> FaceUpdateHook(113883, 0xE0, "BSFaceGenAnimationData::Update",
		[](RE::BSFaceGenAnimationData* a1, float a2, bool a3) -> bool {
			static Manager* m = Manager::GetSingleton();
			bool res = FaceUpdateHook(a1, a2, a3);
			auto d = m->data.lock_read_only();
			if (auto iter = d->controlledDatas.find(a1); iter != d->controlledDatas.end()) {
				bool requiresDetach = iter->second->lock()->Update(a2, a1);
				if (requiresDetach) {
					d.unlock();
					m->DoDetach(a1);
				}
			}
			return res;
		});
}