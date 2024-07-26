#include "FileID.h"
#include "Util/String.h"

namespace Animation
{
	const std::string_view FileID::QPath() const
	{
		return filePath;
	}

	const std::string_view FileID::QID() const
	{
		return id;
	}

	FileID::FileID(const std::string_view a_filePath, const std::string_view a_id)
	{
		filePath = Util::String::ToLower(a_filePath);
		id = Util::String::ToLower(a_id);
	}

	bool FileID::operator==(const FileID& a_rhs) const
	{
		return filePath == a_rhs.filePath &&
		       id == a_rhs.id;
	}

	bool FileID::operator<(const FileID& a_rhs) const
	{
		return filePath < a_rhs.filePath ||
		       id < a_rhs.id;
	}

	bool AnimID::operator==(const AnimID& other) const
	{
		return file == other.file && skeleton == other.skeleton;
	}

	bool AnimID::operator<(const AnimID& other) const
	{
		return file < other.file || skeleton < other.skeleton;
	}
}