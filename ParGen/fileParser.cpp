#include "fileParser.h"

#include <filesystem>

#include "cppParser.h"
#include "vsParser.h"

fileParser* createParser(const char* path)
{
	if (path == nullptr)
	{
		return nullptr;
	}

	std::string ext = std::filesystem::path(path).extension().string();

	if (ext == ".vcxproj")
	{
		return new vsProject();
	}
	else if (ext == ".sln")
	{
		return new vsSolution();
	}
	else if (ext == ".cpp" || ext == ".h" || ext == ".cc")
	{
		return new cppParser();
	}
	else
	{
		return nullptr;
	}
}
