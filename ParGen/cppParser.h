#pragma once
#include "fileParser.h"

class cppParser : public fileParser
{
public:
	[[nodiscard]] bool parse(const char* projPath) override;
};

