#pragma once
#include <rapidxml/rapidxml.hpp>
#include <string>
#include <vector>

#include "fileParser.h"

class vsProject : public fileParser
{
public:
	vsProject();
	vsProject(std::string name, std::string path);
	vsProject(const vsProject& other);
	~vsProject() override;

	[[nodiscard]]
	bool parse(const char* projPath = nullptr) override;
private:
	std::string _name;
	std::string _path;
	rapidxml::xml_document<> _doc;

	std::vector<std::string> _files;
	bool _isVcpkgEnabled;
	std::string _precompiledHeaderFile;
	std::string _languageStandard;
	std::string _outputType;
	std::vector<std::string> _includeDirs;

    void parseFiles();
	void parseVcpkgInfo();
	void parsePrecompiledHeader();
	void parseLanguageStandard();
	void parseOutputType();
	void parseIncludeDirectories();
};

class vsSolution : public fileParser
{
public:
	vsSolution();
	~vsSolution();

	[[nodiscard]]
	bool parse(const char* slnPath);
private:
	std::vector<vsProject> projects;
};