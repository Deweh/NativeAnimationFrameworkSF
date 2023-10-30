#pragma once

namespace RE
{
	class BSLight;
	class BSGeometry;
	class BSFaceGenNiNode;

	template <class T>
	struct BSArray
	{
		uint32_t size;
		uint32_t capacity;
		T*       data;
	};

	struct NiUpdateData
	{
		float     delta;
		float     unk04;
		float     unk08;
		uint32_t  unk0C;
		NiPoint4* unk10;
		NiPoint4* unk18;
		uint64_t  unk20;
		uint64_t  unk28;
		uint64_t  unk30;
		uint32_t  flags;
		uint32_t  flags2;
		uint32_t  flags3;
		uint32_t  unk44;
		float     unk48;
		uint32_t  unk4C;
		float     unk50;
		float     unk54;
		float     unk58;
		uint32_t  unk5C;
	};

	class BGSModelNode
	{
	public:
		virtual ~BGSModelNode() = default;
		virtual void* Refresh(NiNode* rootNode, NiUpdateData*);
		struct UpdateData
		{
			NiTransform t1;
			NiTransform t2;
			NiTransform t3;
			NiPoint4    point;
		};
		virtual bool Update(UpdateData* data, NiUpdateData*, NiTransform*);
		uint32_t     refcount;
		uint32_t     pad0C;

		struct Unk10Struct
		{
			void* unk00_unk88[0x88 >> 3];
			bool  needsUpdate;
		};
		Unk10Struct* unk10;

		struct NodeEntry
		{
			uint16_t index;
			uint16_t unk02;
			uint16_t unk04;
			uint16_t unk06;
			NiNode*  node;
		};

		BSArray<NodeEntry>        nodes;       //18
		BSArray<BSGeometry*>      geometries;  //28
		void*                     unk38;
		void*                     unk40;
		BSArray<BSFaceGenNiNode*> facegenNodes;  //48
		BSArray<NiNode*>          otherNodes;    //58
		void*                     unk68;
		void*                     bhkNPModelNodeComponent;
		void*                     unk78;
		void*                     unk80;
		void*                     unk88;
	};
	static_assert(offsetof(BGSModelNode, nodes) == 0x18);

	template <class T>
	class NiTArray
	{
	public:
		virtual ~NiTArray() = default;
		T*       entries;
		uint16_t m_arrayBufLen;    // 10 - max elements storable in m_data
		uint16_t m_emptyRunStart;  // 12 - index of beginning of empty slot run
		uint16_t m_size;           // 14 - number of filled slots
		uint16_t m_growSize;       // 16 - number of slots to grow m_data by
	};

	class NiBound
	{
	public:
		NiPoint3 center;
		float radius;
	};

	class NiRTTI
	{
	public:
		const char* name;
		const char* parent;
	};

	class NiAVObject {
	public:
		virtual ~NiAVObject() = default;
		virtual void* DeleteThis();
		virtual NiRTTI* GetRTTI();
		virtual NiNode* GetAsNiNode();
		virtual NiNode* GetAsNiSwitchNode();
		virtual void* Unk5() { return nullptr; }
		virtual void* Unk6() { return nullptr; }
		virtual void* Unk7() { return nullptr; }
		virtual void* Unk8() { return nullptr; }
		virtual NiAVObject* GetAsBSGeometry() { return nullptr; }
		virtual void* Unk10() { return nullptr; }
		virtual void* Unk11() { return nullptr; }
		virtual void* Unk12() { return nullptr; }
		virtual void* Unk13() { return nullptr; }
		virtual void* Unk14() { return nullptr; }
		virtual void* Unk15() { return nullptr; }
		virtual void* Unk16() { return nullptr; }
		virtual void* Unk17() { return nullptr; }
		virtual void* Unk18() { return nullptr; }
		virtual void* Unk19() { return nullptr; }
		virtual void* Unk20() { return nullptr; }
		virtual void* Unk21() { return nullptr; }
		virtual void* Unk22() { return nullptr; }
		virtual void* Unk23() { return nullptr; }
		virtual void* Unk24() { return nullptr; }
		virtual void* Unk25() { return nullptr; }
		virtual void* Unk26() { return nullptr; }
		virtual void* Unk27() { return nullptr; }
		virtual void* Unk28() { return nullptr; }
		virtual void* Unk29() { return nullptr; }
		virtual void* Unk30() { return nullptr; }
		virtual BSLight* GetAsBSLight() { return nullptr; }
		virtual void* Unk32() { return nullptr; }
		virtual void* Unk33() { return nullptr; }
		virtual void* Unk34() { return nullptr; }
		virtual void* Unk35() { return nullptr; }
		virtual void* Unk36() { return nullptr; }
		virtual void* Unk37() { return nullptr; }
		virtual void* Unk38() { return nullptr; }
		virtual void* Unk39() { return nullptr; }
		virtual void* Unk40() { return nullptr; }
		virtual void* Unk41();
		virtual void* Unk42();
		virtual void* Unk43();
		virtual void* Unk44();
		virtual void* Unk45();
		virtual void* Unk46();
		virtual void* Unk47();
		virtual void* Unk48();
		virtual void* Unk49();
		virtual void* Unk50();
		virtual void* Unk51();
		virtual void* Unk52();
		virtual void* Unk53();
		virtual void* Unk54();
		//NiAVObject
		virtual void* Unk55();
		virtual void* Unk56();
		virtual void* Unk57();
		virtual void* Unk58();
		virtual void* Unk59();
		virtual void* Unk60();
		virtual void* Unk61();
		virtual NiNode* GetObjectByName(const BSFixedString& name);
		virtual void* SetSelectiveUpdateFlags();
		virtual void* Unk64();
		virtual void* Unk65();
		virtual void* Unk66();
		virtual void* Unk67();
		virtual void* Unk68();
		virtual void* Unk69();
		virtual void* Unk70();
		virtual void* Unk71();
		virtual void* Unk72();
		virtual void* Unk73();
		virtual void* Unk74();
		virtual void* Unk75();
		virtual void* Unk76();
		virtual void* Unk77();
		virtual void* Unk78();
		virtual void* UpdateWorldData(NiUpdateData* data);
		virtual void* UpdateTransformAndBounds();
		virtual void* UpdateTransforms(NiUpdateData* data);
		virtual void* Unk82();
		virtual void* Unk83();

		uint32_t refcount;
		uint32_t pad0C;
		BSFixedString name;
		void* controller;
		void* unk28;
		void* unk30;
		NiNode* parent;				//38
		NiTransform local;			//40
		NiTransform world;			//80
		NiTransform previousWorld;  //C0
		NiBound	worldBound;			//100
		void* collisionObject;
		uint64_t flags;				//118
		void* unk120;
		void* unk128;
	};
	static_assert(offsetof(NiAVObject, parent) == 0x38);
	static_assert(offsetof(NiAVObject, local) == 0x40);

	class NiNode : public NiAVObject {
	public:
		virtual ~NiNode() = default;
		virtual void* AttachChild(NiAVObject* child, bool firstAvailable);
		virtual void* InsertChildAt(uint32_t idx, NiAVObject* child);
		virtual void* DetachChild(NiAVObject* child);
		virtual void* DetachChildOut(NiAVObject* child, NiAVObject** outObj); //RE::NiPointer<NiAVObject>&
		virtual void* DetachChildAt(uint32_t idx);
		virtual void* DetachChildAtOut(uint32_t idx, NiAVObject** outObj); //RE::NiPointer<NiAVObject>&
		virtual void* SetAt(uint32_t idx, NiAVObject* child);
		virtual void* SetAtOut(uint32_t idx, NiAVObject* child, NiAVObject** outObj); //RE::NiPointer<NiAVObject>&
		virtual void* Unk92();
		virtual void* Unk93();
		virtual void* Unk94();

		NiTArray<NiNode*> children; //NiTObjectArray<NiPointer<NiAVObject>> //130
		void* unk148;
	};
	static constexpr size_t tst{ offsetof(NiNode, children) };

	class BGSFadeNode : public NiNode
	{
	public:
		virtual ~BGSFadeNode() = default;

		void*         unk150;
		void*         unk158;
		void*         unk160;
		void*         unk168;
		void*         unk170;
		void*         unk178;
		BGSModelNode* bgsModelNode;
	};

	class BSGeometry : public NiAVObject
	{
	public:
		virtual ~BSGeometry() = default;

		void*         unk130;
		void*         unk138;
		void*         unk140;
		void*         unk148;
		void*         unk150;
		void*         unk158;
		void*         unk160;
		void*         unk168;
		void*         unk170;
		void*         unk178;
		void*         unk180;
		void*         unk188;
		void*         unk190;
		void*         unk198;
		void*         unk1A0;
		void*         unk1A8;
		void*         skinInstance;
		void*         morphTargetData;
		void*         unk1C0;
		void*         unk1C8;
		BSFixedString materialPath;
	};

	class BSFaceGenAnimationData
	{
	public:
		virtual ~BSFaceGenAnimationData() = default;

		static constexpr size_t morphSize = 0x68;

		void*         unk08;
		void*         unk10;
		float         morphs[morphSize];
		float         morphs2[morphSize];
		void*         unk358;
		void*         unk360;
		void*         unk368;
		void*         unk370;
		float         unk378;
		uint32_t      unk37C;
		void*         unk380;
		void*         unk388;
		void*         unk390;
		void*         unk398;
		void*         unk3A0;
		BSFixedString animFaceArchetype;
		uint32_t      unk3B0;
		uint32_t      unk3B4;
		void*         facefx_actor;
		uint32_t      unk3C0;
		uint32_t      unk3C4;
		void*         unk3C8;
		void*         unk3D0;
		void*         unk3D8;
		void*         unk3E0;
		void*         unk3E8;
		BSFixedString morphNames[morphSize];  //These are sometimes missing
	};

	class BSFaceGenNiNode : public NiNode
	{
	public:
		virtual ~BSFaceGenNiNode() = default;
		NiMatrix3               unk150;
		BSFaceGenAnimationData* faceGenAnimData;  //180
		float                   unk188;
		uint32_t                facegenFlags;
	};

	struct BSModelDBEntry
	{
		void*        unk00;
		void*        unk08;
		void*        unk10;
		void*        unk18;
		BGSFadeNode* node;
	};

	struct TESObjectREFRLoadedData
	{
		BSModelDBEntry** baseEntry;
		BGSFadeNode*     rootNode;
		//...
	};
}