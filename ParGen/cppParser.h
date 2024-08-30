#pragma once
#include <clang-c/Index.h>

#include "fileParser.h"
#include <inja/inja.hpp>

class cppParser
{
public:
	cppParser(std::filesystem::path &&path,
		bool isVcpkgEnabled = false, 
		std::string&& languageStandard = "c++23");
	[[nodiscard]] bool parse();
	void fillInData(inja::json& outdata);

private:
	std::filesystem::path _path;
	CXTranslationUnit unit;
};

