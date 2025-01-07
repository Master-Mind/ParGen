#pragma once
#include <filesystem>
#include <ostream>
#include <yyjson.h>

void renderTemplate(const std::filesystem::path& templatePath, std::ostream &out, yyjson_mut_doc *doc);
