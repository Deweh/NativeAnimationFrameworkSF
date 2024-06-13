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

	Entry* GetEntry(const char* filename)
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