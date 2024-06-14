#pragma once

namespace Util
{
	class String
	{
	public:
		static const std::filesystem::path& GetGamePath();
		static const std::filesystem::path& GetDataPath();
		static std::string ToLower(const std::string_view s);
		static std::string_view TransformToLower(std::string& s);
		static std::vector<std::string_view> Split(const std::string_view& s, const std::string_view& delimiter, const std::optional<char>& escapeChar = std::nullopt);
		static std::optional<uint32_t> HexToUInt(const std::string&& s);
	};
}