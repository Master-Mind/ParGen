#pragma once
#include <filesystem>
#include <clang-c/Index.h>

#include "fileParser.h"
#include <yyjson.h>

struct JSONData;

class cppParser
{
public:
	cppParser(std::filesystem::path &&path,
		bool isVcpkgEnabled = false, 
		std::string&& languageStandard = "c++23");
	[[nodiscard]] bool parse();
	void fillInData(yyjson_mut_doc* doc, yyjson_mut_val* outdata);

private:
	std::filesystem::path _path;
	CXTranslationUnit unit;
};

