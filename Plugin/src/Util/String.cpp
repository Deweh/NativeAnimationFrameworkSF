#include "String.h"

namespace Util
{
	const std::filesystem::path& String::GetGamePath()
	{
		static std::optional<std::filesystem::path> BasePath = std::nullopt;

		if (!BasePath) {
			std::filesystem::path p = REL::WinAPI::GetProcPath(nullptr);
			BasePath = p.parent_path();
		}

		return *BasePath;
	}

	const std::filesystem::path& String::GetDataPath()
	{
		static std::optional<std::filesystem::path> DataPath = std::nullopt;

		if (!DataPath) {
			DataPath = GetGamePath() / "Data" / "NAF";
		}

		return *DataPath;
	}

	std::string String::ToLower(const std::string_view s)
	{
		std::string result(s);
		std::transform(result.begin(), result.end(), result.begin(), [](unsigned char c) { return std::tolower(c); });
		return result;
	}

	std::string_view String::TransformToLower(std::string& s)
	{
		std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return std::tolower(c); });
		return s;
	}

	std::vector<std::string_view> String::Split(const std::string_view& s, const std::string_view& delimiter, const std::optional<char>& escapeChar)
	{
		std::vector<std::string_view> substrings;
		size_t start = 0;
		size_t end = 0;
		bool escaped = false;

		while (end < s.length()) {
			if (escapeChar.has_value() && s[end] == escapeChar) {
				escaped = !escaped;
			} else if (!escaped && s.substr(end, delimiter.length()) == delimiter) {
				substrings.push_back(s.substr(start, end - start));
				start = end + delimiter.length();
				end = start - 1;
			}
			end++;
		}

		if (start < s.length()) {
			substrings.push_back(s.substr(start));
		}

		if (escapeChar.has_value()) {
			for (auto& substring : substrings) {
				if (substring.size() > 1 && substring.front() == escapeChar.value() && substring.back() == escapeChar.value()) {
					substring.remove_prefix(1);
					substring.remove_suffix(1);
				}
			}
		}

		return substrings;
	}
}