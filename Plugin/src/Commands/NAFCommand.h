#pragma once
#include "API/CCF_API.h"

namespace Commands::NAFCommand
{
	void Run(const CCF::simple_array<CCF::simple_string_view>& a_args, const char* a_fullString, CCF::ConsoleInterface* a_intfc);
	void RegisterKeybinds();
}