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

	void Manager::SetNoBlink(uint32_t a_refId, RE::BSFaceGenAnimationData* a_data, bool a_noBlink)
	{
		auto d = data.lock();
		if (auto iter = d->refMap.find(a_refId); iter != d->refMap.end()) {
			d->noBlink.erase(iter->second);
		}
		if (a_noBlink) {
			d->refMap[a_refId] = a_data;
			d->noBlink.insert(a_data);
		} else {
			d->refMap.erase(a_refId);
			d->noBlink.erase(a_data);
		}
	}

	void BlinkUpdate(RE::BSFaceGenAnimationData* a1, float a2);
	bool FaceUpdate(RE::BSFaceGenAnimationData* a1, float a2, bool a3);

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
		d->refMap.clear();
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
		return OriginalFaceUpdate(a1, a2, a3);
	}
}