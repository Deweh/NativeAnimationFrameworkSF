#pragma once

namespace Animation
{
	class FileID
	{
	private:
		std::string filePath;
		std::string id;

	public:
		const std::string_view QPath() const;
		const std::string_view QID() const;

		FileID() = default;
		FileID(const std::string_view a_filePath, const std::string_view a_id);
		bool operator==(const FileID& a_rhs) const;
		bool operator<(const FileID& a_rhs) const;
	};

	struct AnimID
	{
		FileID file;
		std::string skeleton;

		bool operator==(const AnimID& other) const;
		bool operator<(const AnimID& other) const;
	};
}