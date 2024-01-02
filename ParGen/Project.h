#pragma once
class Project
{
	public:
	virtual ~Project() = default;
	[[nodiscard]]
	virtual bool parse(const char* projPath) = 0;
};

