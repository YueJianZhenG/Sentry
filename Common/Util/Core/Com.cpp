//
// Created by 64658 on 2025/11/1.
//

#include "Com.h"
#include "bundled/format.h"

namespace help
{
	std::string com::BytesToString(long long bytes)
	{
		std::string result;
		double fileSize = (double)bytes;
		if (std::abs(fileSize) >= help::com::GB)
		{
			double val = fileSize / help::com::GB;
			return fmt::format("{:.2f}GB", (float)val);
		}
		if (std::abs(fileSize) >= help::com::MB)
		{
			double val = fileSize / help::com::MB;
			return fmt::format("{:.2f}MB", (float)val);
		}
		if (std::abs(fileSize) >= help::com::KB)
		{
			double val = fileSize / help::com::KB;
			return fmt::format("{:.2f}KB", (float)val);
		}
		return fmt::format("{}B", fileSize);
	}
}