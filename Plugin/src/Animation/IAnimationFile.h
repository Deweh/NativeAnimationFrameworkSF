#pragma once
#include "FileID.h"

namespace Animation
{
	class Generator;

	class IAnimationFile : public std::enable_shared_from_this<IAnimationFile>
	{
	public:
		struct ExtraData
		{
			float loadTime = -1.0f;
			AnimID id;
		};

		ExtraData extra;

		virtual std::unique_ptr<Generator> CreateGenerator() = 0;
		virtual ~IAnimationFile();
	};
}