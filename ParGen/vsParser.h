#pragma once
#include <filesystem>
#include <string>
#include <vector>

#include "cppParser.h"

bool parseSolution(std::filesystem::path& slnPath, std::vector<cppParser>& outfiles);
bool parseVCProj(std::filesystem::path& projPath, std::vector<cppParser>& outfiles);