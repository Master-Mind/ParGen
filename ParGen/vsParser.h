#pragma once
#include <string>
#include <vector>

#include "Project.h"

class vsProject : public Project
{
public:
	vsProject();
	vsProject(std::string name, std::string path);
	~vsProject() override;

	[[nodiscard]]
	bool parse(const char* projPath = nullptr) override;
private:
	std::string _name;
	std::string _path;
};

class vsSolution : public Project
{
public:
	vsSolution();
	~vsSolution();

	[[nodiscard]]
	bool parse(const char* slnPath);
private:
	std::vector<vsProject> projects;
};