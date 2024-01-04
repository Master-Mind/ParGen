#include "vsParser.h"

#include <filesystem>
#include <iostream>
#include <fstream>
#include <print>
#include <string>

vsProject::vsProject()
{
}

vsProject::vsProject(std::string name, std::string path) : _name(name), _path(path)
{
	
}

vsProject::vsProject(const vsProject& other)
{
	_name = other._name;
	_path = other._path;
	_files = other._files;
	_isVcpkgEnabled = other._isVcpkgEnabled;
	_precompiledHeaderFile = other._precompiledHeaderFile;
	_languageStandard = other._languageStandard;
	_outputType = other._outputType;
	_includeDirs = other._includeDirs;
}

vsProject::~vsProject()
{
}

bool vsProject::parse(const char* projPath)
{
    if (projPath == nullptr)
    {
	    projPath = _path.c_str();
	}

	assert(std::filesystem::exists(projPath));

	std::print("Parsing project: {}\n", projPath);

	size_t fsize = std::filesystem::file_size(projPath);

	//load the file into memory
	char *filedata = new char[fsize + 1];

	std::ifstream file(projPath, std::ifstream::binary);

	if (!file.is_open())
	{
		return false;
	}

	file.read(filedata, fsize);

	file.close();

	//the file is in utf-8-bom, so we need to skip the first 3 bytes and add a null terminator at a weird place
	char *truefiledata = strchr(filedata, '<');
	*(strrchr(truefiledata, '>') + 1) = '\0';

	_doc.parse<0>(truefiledata);

	parseFiles();
	parseVcpkgInfo();
	parsePrecompiledHeader();
	parseLanguageStandard();
	parseOutputType();
	parseIncludeDirectories();

	for (auto& file : _files)
	{
		auto path = std::filesystem::path(projPath).parent_path() / file;
		fileParser *par = createParser(path.string().c_str());

		if (!par)
		{
			return false;
		}

		assert(par->parse(path.string().c_str()));
	}

    return true;
}

void vsProject::parseFiles()
{
	for (auto itemGroup = _doc.first_node("Project")->first_node("ItemGroup"); 
		itemGroup; 
		itemGroup = itemGroup->next_sibling("ItemGroup"))
	{
		for (auto clCompile = itemGroup->first_node("ClCompile");
			clCompile;
			clCompile = clCompile->next_sibling("ClCompile"))
		{
			_files.push_back(clCompile->first_attribute("Include")->value());
		}
	}
}

void vsProject::parseVcpkgInfo()
{
	for (auto propertyGroup = _doc.first_node("Project")->first_node("PropertyGroup");
		propertyGroup;
		propertyGroup = propertyGroup->next_sibling("PropertyGroup"))
	{
		auto label = propertyGroup->first_attribute("Label");
		if (label && strstr(label->value(), "Vcpkg"))
		{
			//VcpkgEnabled defaults to true if it doesn't exist
			auto vcpkgEnabled = propertyGroup->first_node("VcpkgEnabled");
			_isVcpkgEnabled = vcpkgEnabled ? strstr(vcpkgEnabled->value(), "true") : true;
			return;
		}
	}	
}

void vsProject::parsePrecompiledHeader()
{
	//for (auto& itemDefinitionGroup : _doc.child("Project").children("ItemDefinitionGroup")) {
	//	auto pchNode = itemDefinitionGroup.child("ClCompile").child("PrecompiledHeader");
	//	if (pchNode) {
	//		_precompiledHeaderFile = pchNode.child_value();
	//		break;
	//	}
	//}
}

void vsProject::parseLanguageStandard()
{
	for (auto propertyGroup = _doc.first_node("Project")->first_node("ItemDefinitionGroup");
		propertyGroup;
		propertyGroup = propertyGroup->next_sibling("ItemDefinitionGroup")) 
	{
		auto clCompile = propertyGroup->first_node("ClCompile");

		if (clCompile)
		{
			auto langStdNode = clCompile->first_node("LanguageStandard");

			if (langStdNode)
			{
				_languageStandard = langStdNode->value();
				break;
			}
		}
	}
}

void vsProject::parseOutputType()
{
	for(auto propertyGroup = _doc.first_node("Project")->first_node("PropertyGroup");
		propertyGroup; 
		propertyGroup = propertyGroup->next_sibling("PropertyGroup"))
	{
		auto config = propertyGroup->first_node("ConfigurationType");
		if (config)
		{
			_outputType = config->value();
			return;
		}
	}
}

void vsProject::parseIncludeDirectories()
{
	//for (auto& itemDefinitionGroup : _doc.child("Project").children("ItemDefinitionGroup")) {
	//	for (auto& clCompile : itemDefinitionGroup.children("ClCompile")) {
	//		for (auto& includeDir : clCompile.children("AdditionalIncludeDirectories")) {
	//			_includeDirs.push_back(includeDir.child_value());
	//		}
	//	}
	//}
}

vsSolution::vsSolution()
{
}

vsSolution::~vsSolution()
{
}

bool vsSolution::parse(const char* slnPath)
{
	std::print("Parsing solution: {}\n", slnPath);
    std::ifstream file(slnPath);

    if (!file.is_open()) {
        std::cerr << "Unable to open file: " << slnPath << std::endl;
        return false;
    }

    std::string line;
    std::string name, path, guid;
    std::size_t pos = 0;

    while (std::getline(file, line))
    {
        if (line.find("Project(\"{") != std::string::npos)
        {
	        			//found a project line
			//get the name
			pos = line.find(" = \"");
			name = line.substr(pos + 4, line.find("\",") - pos - 4);

			//get the path
			pos = line.find("\"", pos + 5 + name.size());
			path = line.substr(pos + 1, line.find("\",", pos + 1) - pos - 1);

			//add the project to the list
			projects.emplace_back(name, path);
		}
    }

	for (auto& proj : projects)
	{
		if (!proj.parse())
		{
			return false;
		}
	}

    return true;
}
