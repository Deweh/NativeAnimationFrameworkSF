#pragma once
#include "Util/Math.h"

namespace RE
{
	struct TransformsManager
	{
		static TransformsManager* GetSingleton()
		{
			REL::Relocation<TransformsManager**> singleton(REL::ID(881086));
			return *singleton;
		}

		void RequestTransformUpdate(uint32_t a_flags, RE::TESObjectREFR* a_refr, char a_axis, double a_value)
		{
			using func_t = decltype(&TransformsManager::RequestTransformUpdate);
			REL::Relocation<func_t> func{ REL::ID(149852) };
			func(this, a_flags, a_refr, a_axis, a_value);
		}

		void RequestPositionUpdate(RE::TESObjectREFR* a_ref, const RE::NiPoint3& a_pos)
		{
			using func_t = decltype(&TransformsManager::RequestTransformUpdate);
			REL::Relocation<func_t> func{ REL::ID(149852) };
			func(this, 0x1007, a_ref, 'X', a_pos.x);
			func(this, 0x1007, a_ref, 'Y', a_pos.y);
			func(this, 0x1007, a_ref, 'Z', a_pos.z);
		}

		void RequestRotationUpdate(RE::TESObjectREFR* a_ref, const RE::NiPoint3& a_rot)
		{
			using func_t = decltype(&TransformsManager::RequestTransformUpdate);
			REL::Relocation<func_t> func{ REL::ID(149852) };
			func(this, 0x1009, a_ref, 'X', a_rot.x * Util::RADIAN_TO_DEGREE);
			func(this, 0x1009, a_ref, 'Y', a_rot.y * Util::RADIAN_TO_DEGREE);
			func(this, 0x1009, a_ref, 'Z', a_rot.z * Util::RADIAN_TO_DEGREE);
		}

		void RequestPosRotUpdate(RE::TESObjectREFR* a_ref, const RE::NiPoint3& a_pos, const RE::NiPoint3& a_rot)
		{
			using func_t = decltype(&TransformsManager::RequestTransformUpdate);
			REL::Relocation<func_t> func{ REL::ID(149852) };
			func(this, 0x1007, a_ref, 'X', a_pos.x);
			func(this, 0x1007, a_ref, 'Y', a_pos.y);
			func(this, 0x1007, a_ref, 'Z', a_pos.z);
			func(this, 0x1009, a_ref, 'X', a_rot.x * Util::RADIAN_TO_DEGREE);
			func(this, 0x1009, a_ref, 'Y', a_rot.y * Util::RADIAN_TO_DEGREE);
			func(this, 0x1009, a_ref, 'Z', a_rot.z * Util::RADIAN_TO_DEGREE);
		}
	};
}