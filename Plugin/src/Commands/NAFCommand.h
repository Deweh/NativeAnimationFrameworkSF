#pragma once

namespace Commands::NAFCommand
{
	void Run(const std::vector<std::string_view>& args, const std::string_view& fullStr, RE::TESObjectREFR* refr);
}