#pragma once

namespace RE
{
	class BSLight;
	class BSGeometry;
	class BSFaceGenNiNode;

	struct BSAnimationUpdateData
	{
		NiPoint3A rootLocation1;
		NiPoint3A rootAngle1;
		NiPoint3A rootLocation2;
		NiPoint3A rootAngle2;
		float unk01;
		float unk02;
		IPostAnimationChannelUpdateFunctor* postUpdateFunctor;
		float unk05;
		float unk06;
		float unk07;
		float unk08;
		float timeDelta;
		float unk09;
		uint16_t unk11;
		bool forceUpdate;

		//It appears this is also set to true when an actor is very far away & switches out to a lower-resolution LOD.
		//When the actor is even further away, the animation update routine stops getting called at all, likely due to ShouldUpdateAnimation() returning false.
		bool modelCulled;
		bool unk13;
		bool unkFlag;
		bool unk15;
		float unk16;
	};
	static_assert(offsetof(BSAnimationUpdateData, timeDelta) == 0x60);
	static_assert(offsetof(BSAnimationUpdateData, modelCulled) == 0x6B);

	template <class T>
	struct BSArray
	{
		uint32_t size;
		uint32_t capacity;
		T*       data;
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

	class NiRTTI
	{
	public:
		const char* name;
		const char* parent;
	};

	static constexpr size_t tst123{ offsetof(NiAVObject, NiAVObject::world) };

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

	class BGSFadeNode : public NiNode
	{
	public:
		virtual ~BGSFadeNode() = default;

		struct UnkEntry
		{
			std::byte    unk00[32];
			NiBound      worldBound;
			NiAVObject*  node;
			void*        unk01;
		};

		void*         unk150;
		void*         unk158;
		void*         unk160;
		void*         unk168;
		BSArray<UnkEntry> geometries;
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