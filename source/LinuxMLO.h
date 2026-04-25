#pragma once
#include <string>

namespace Parless
{
	std::string ToUnixViaWine(const std::string& win_path);

	void HandleLinuxMLO();
}
