#pragma once

namespace Commands
{
	typedef std::function<void(const std::vector<std::string_view>&, const std::string_view&, RE::TESObjectREFR*)> CommandFunction;

	void InstallHooks();
	void RegisterCommand(const std::string& name, const CommandFunction& func);
}