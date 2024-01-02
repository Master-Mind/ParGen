#pragma once
class fileParser
{
	public:
	virtual ~fileParser() = default;
	[[nodiscard]]
	virtual bool parse(const char* projPath) = 0;
};

fileParser* createParser(const char* path);