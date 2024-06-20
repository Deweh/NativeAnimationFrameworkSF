#include "Manager.h"
#include "Util/Trampoline.h"

namespace Animation::Face
{
	typedef void (*BlinkUpdateFunc)(RE::BSFaceGenAnimationData*, float);
	typedef bool (*FaceUpdateFunc)(RE::BSFaceGenAnimationData*, float, bool);

	BlinkUpdateFunc OriginalBlinkUpdate;
	FaceUpdateFunc OriginalFaceUpdate;

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

	void Manager::AttachMorphData(RE::BSFaceGenAnimationData* a_data, std::shared_ptr<MorphData> a_morphs)
	{
		if (!a_data) {
			return;
		}

		data.lock()->controlledDatas[a_data] = a_morphs;
	}

	void Manager::DetachMorphData(RE::BSFaceGenAnimationData* a_data)
	{
		data.lock()->controlledDatas.erase(a_data);
	}

	void BlinkUpdate(RE::BSFaceGenAnimationData* a1, float a2);
	bool FaceUpdate(RE::BSFaceGenAnimationData* a1, float a2, bool a3);

	std::shared_ptr<MorphData> Manager::GetMorphData(RE::BSFaceGenAnimationData* a_data)
	{
		auto d = data.lock_read_only();
		if (auto iter = d->controlledDatas.find(a_data); iter != d->controlledDatas.end()) {
			return iter->second;
		}
		return nullptr;
	}

	void Manager::InstallHooks()
	{
		Util::Trampoline::AddHook(28, [](SFSE::Trampoline& t) {
			//BSFaceGenAnimationData::UpdateBlinking(BSFaceGenAnimationData*, float)
			REL::Relocation<uintptr_t> blinkUpdate{ REL::ID(113796), 0x147 };
			OriginalBlinkUpdate = reinterpret_cast<BlinkUpdateFunc>(SFSE::GetTrampoline().write_call<5>(blinkUpdate.address(), &BlinkUpdate));

			//BSFaceGenAnimationData::Update(BSFaceGenAnimationData*, float, bool)
			REL::Relocation<uintptr_t> faceUpdate{ REL::ID(113883), 0xE0 };
			OriginalFaceUpdate = reinterpret_cast<FaceUpdateFunc>(SFSE::GetTrampoline().write_call<5>(faceUpdate.address(), &FaceUpdate));

			INFO("Installed face update hook.");
		});
	}

	void Manager::Reset()
	{
		auto d = data.lock();
		d->noBlink.clear();
		d->controlledDatas.clear();
	}

	void BlinkUpdate(RE::BSFaceGenAnimationData* a1, float a2)
	{
		static Manager* m = Manager::GetSingleton();
		if (!m->data.lock_read_only()->noBlink.contains(a1)) {
			OriginalBlinkUpdate(a1, a2);
		}
	}

	bool FaceUpdate(RE::BSFaceGenAnimationData* a1, float a2, bool a3)
	{
		static Manager* m = Manager::GetSingleton();
		bool res = OriginalFaceUpdate(a1, a2, a3);
		if (auto d = m->GetMorphData(a1); d != nullptr) {
			auto morphs = d->lock();
			for (size_t i = 0; i < RE::BSFaceGenAnimationData::morphSize; i++) {
				a1->morphs[i] = (*morphs)[i];
			}
		}
		return res;
	}
}