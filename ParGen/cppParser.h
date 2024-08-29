#pragma once
#include "fileParser.h"
#include <inja/inja.hpp>

class cppParser
{
public:
	cppParser(std::filesystem::path &&path,
		bool isVcpkgEnabled = false, 
		std::string&& languageStandard = "c++23");
	[[nodiscard]] bool parse(inja::json &outdata);

private:
	std::filesystem::path _path;
};

