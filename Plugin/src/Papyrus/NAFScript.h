#pragma once

namespace Papyrus::NAFScript
{
	constexpr const char* SCRIPT_NAME{ "NAF" };
	void RegisterFunctions(RE::BSScript::IVirtualMachine* a_vm);
}