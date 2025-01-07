#pragma once
#include <string>
#include <vector>

class fileParser
{
	public:
	virtual ~fileParser() = default;
	[[nodiscard]]
	virtual bool parse() = 0;
};

fileParser* createParser(const char* path);