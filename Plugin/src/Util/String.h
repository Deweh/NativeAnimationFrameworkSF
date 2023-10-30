#pragma once

namespace Util
{
	class String
	{
	public:
		static std::vector<std::string_view> Split(const std::string_view& s, const std::string_view& delimiter, const std::optional<char>& escapeChar = std::nullopt);
	};
}