#pragma once
#include "NiAVObject.h"
#include "Events.h"
#include "TransformsManager.h"

namespace RE::ModelDB
{
	struct Entry
	{
		void* unk00;
		void* unk08;
		void* unk10;
		void* unk18;
		BGSFadeNode* node;
	};

	inline Entry* GetEntry(const char* filename)
	{
		uint64_t flag = 0x3;
		struct UnkOut
		{
			uint32_t unk00 = 0;
			uint32_t unk04 = 0;
			uint64_t unk08 = 0;
			uint64_t unk10 = 0;
		};
		UnkOut out;
		out.unk00 = flag & 0xFFFFFFu;
		const bool isBound = true;
		out.unk00 |= ((16 * (isBound & 1)) | 0x2D) << 24;
		out.unk04 = 0xFEu;
		Entry* entry = nullptr;
		REL::Relocation<int(const char*, Entry**, UnkOut&)> GetModelDBEntry(REL::ID(183072));
		GetModelDBEntry(filename, &entry, out);
		REL::Relocation<uint64_t(UnkOut&)> DecRef(REL::ID(36741));
		DecRef(out);
		return entry;
	}
}

namespace RE::EyeTracking
{
	struct EyeData
	{
		NiPointer<NiNode> head;
		NiPointer<NiNode> leftEye;
		NiPointer<NiNode> rightEye;
		NiPointer<NiNode> eyeTarget;
	};

	struct EyeNodes
	{
		inline EyeNodes()
		{
			REL::Relocation<uint64_t*> eyeTargetMutex(REL::ID(858265));
			REL::Relocation<void(uint64_t*)> LockWrite(REL::ID(34125));
			mutex = eyeTargetMutex.get();
			LockWrite(mutex);
		}

		inline EyeNodes(const EyeNodes& other) = delete;

		inline EyeNodes(EyeNodes&& other) noexcept {
			data = other.data;
			mutex = other.mutex;
			other.data = std::span<EyeData>();
			other.mutex = nullptr;
		}

		inline ~EyeNodes()
		{
			unlock();
		}

		inline void unlock()
		{
			if (mutex) {
				REL::Relocation<void(uint64_t**)> UnlockWrite(REL::ID(36740));
				UnlockWrite(&mutex);
			}
		}

		uint64_t* mutex = nullptr;
		std::span<EyeData> data;
	};

	inline EyeNodes GetEyeNodes()
	{
		REL::Relocation<EyeData*> eyeTargetNodes(REL::ID(858268));
		//REL::Relocation<uint32_t*> eyeTargetIdx(REL::ID(880003));

		EyeNodes result;
		result.data = std::span<EyeData>(eyeTargetNodes.get(), 200);

		return result;
	}
}

namespace RE
{
	class hkVector4f
	{
	public:
		__m128 quad;  // 00
	};
	static_assert(sizeof(hkVector4f) == 0x10);

	struct bhkCharProxyController
	{
		virtual ~bhkCharProxyController();

		virtual void Unk_01();                                                        // 01
		virtual void Unk_02();                                                        // 02
		virtual void Unk_03();                                                        // 03
		virtual void Unk_04();                                                        // 04
		virtual void Unk_05();                                                        // 05
		virtual void Unk_06();                                                        // 06
		virtual void Unk_07();                                                        // 07
		virtual void Unk_08();                                                        // 08
		virtual void Unk_09();                                                        // 09
		virtual void Unk_0A();                                                        // 0A
		virtual void Unk_0B();                                                        // 0B
		virtual void Unk_0C();                                                        // 0C
		virtual void Unk_0D();                                                        // 0D
		virtual void Unk_0E();                                                        // 0E
		virtual void Unk_0F();                                                        // 0F
		virtual void Unk_10();                                                        // 10
		virtual void Unk_11();                                                        // 11
		virtual void Unk_12();                                                        // 12
		virtual void Unk_13();                                                        // 13
		virtual void Unk_14();                                                        // 14
		virtual void Unk_15();                                                        // 15
		virtual void Unk_16();                                                        // 16
		virtual void Unk_17();                                                        // 17
		virtual void Unk_18();                                                        // 18
		virtual void Unk_19();                                                        // 19
		virtual void Unk_1A();                                                        // 1A
		virtual void Unk_1B();                                                        // 1B
		virtual void Unk_1C();                                                        // 1C
		virtual void Unk_1D();                                                        // 1D
		virtual void Unk_1E();                                                        // 1E
		virtual void Unk_1F();                                                        // 1F
		virtual void Unk_20();                                                        // 20
		virtual void Unk_21();                                                        // 21
		virtual void Unk_22();                                                        // 22
		virtual void Unk_23();                                                        // 23
		virtual void Unk_24();                                                        // 24
		virtual void Unk_25();                                                        // 25
		virtual void Unk_26();                                                        // 26
		virtual void Unk_27();                                                        // 27
		virtual void Unk_28();                                                        // 28
		virtual void Unk_29();                                                        // 29
		virtual void Unk_2A();                                                        // 2A
		virtual void Unk_2B();                                                        // 2B
		virtual void Unk_2C();                                                        // 2C
		virtual void Unk_2D();                                                        // 2D
		virtual void Unk_2E();                                                        // 2E
		virtual void Unk_2F();                                                        // 2F
		virtual void Unk_30();                                                        // 30
		virtual void Unk_31();                                                        // 31
		virtual void Unk_32();                                                        // 32
		virtual void Unk_33();                                                        // 33
		virtual void Unk_34();                                                        // 34
		virtual void Unk_35();                                                        // 35
		virtual void Unk_36();                                                        // 36
		virtual void Unk_37();                                                        // 37
		virtual void Unk_38();                                                        // 38
		virtual void Unk_39();                                                        // 39
		virtual void Unk_3A();                                                        // 3A
		virtual void Unk_3B();                                                        // 3B
		virtual void Unk_3C();                                                        // 3C
		virtual void Unk_3D();                                                        // 3D
		virtual void Unk_3E();                                                        // 3E
		virtual void Unk_3F();                                                        // 3F
		virtual void Unk_40();                                                        // 40
		virtual void Unk_41();                                                        // 41
		virtual void Unk_42();                                                        // 42
		virtual void Unk_43();                                                        // 43
		virtual void Unk_44();                                                        // 44
		virtual void Unk_45();                                                        // 45
		virtual void Unk_46();                                                        // 46
		virtual void SetPosition(const RE::NiPoint3A& a_pos, bool a_unkFlag = true);  // 47
		virtual void Unk_48();                                                        // 48
		virtual void Unk_49();                                                        // 49
		virtual void Unk_4A();                                                        // 4A
		virtual void Unk_4B();                                                        // 4B
		virtual void Unk_4C();                                                        // 4C
		virtual void Unk_4D();                                                        // 4D
		virtual void* SetLinearVelocityImpl(const hkVector4f& a_velocity);            // 4E
		virtual void Unk_4F();                                                        // 4F
		virtual void Unk_50();                                                        // 50
		virtual void Unk_51();                                                        // 51
		virtual void Unk_52();                                                        // 52
		virtual void Unk_53();                                                        // 53
		virtual void Unk_54();                                                        // 54
		virtual void Unk_55();                                                        // 55
		virtual void Unk_56();                                                        // 56
		virtual void Unk_57();                                                        // 57
		virtual void Unk_58();                                                        // 58
		virtual void Unk_59();                                                        // 59
		virtual void Unk_5A();                                                        // 5A
		virtual void Unk_5B();                                                        // 5B
		virtual void Unk_5C();                                                        // 5C
		virtual void Unk_5D();                                                        // 5D
		virtual void Unk_5E();                                                        // 5E
		virtual void Unk_5F();                                                        // 5F
		virtual void Unk_60();                                                        // 60
	};

	struct MiddleHighProcessData
	{
		std::byte unk00[0x458];
		bhkCharProxyController* charProxy;
	};
	static_assert(offsetof(MiddleHighProcessData, charProxy) == 0x458);
}