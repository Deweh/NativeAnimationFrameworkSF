#pragma once

namespace RE
{
	struct MenuModeChangeEvent
	{
		BSFixedString menuName;
		bool enteringMenuMode;
	};
	static_assert(sizeof(MenuModeChangeEvent) == 0x10);

	struct InitLoadEvent
	{
		enum Stage
		{
			Unk1 = 1,
			Unk2,
			Unk3,
			Unk4,
			Unk5
		};
		uint32_t stage;
		uint32_t stageMax;
		uint64_t unk;

		static void RegisterSink(BSTEventSink<InitLoadEvent>* a_sink) {
			using func_t = decltype(&RegisterSink);
			REL::Relocation<func_t> func(REL::ID(37725));
			return func(a_sink);
		}
	};
}