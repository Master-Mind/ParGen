#pragma once
#include "fileParser.h"
#include <inja/inja.hpp>
class cppParser : public fileParser
{
public:
	[[nodiscard]] bool parse(const char* projPath) override;
private:
	inja::json _data;
};

