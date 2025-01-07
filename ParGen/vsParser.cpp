#include "vsParser.h"

#include <filesystem>
#include <iostream>
#include <fstream>
#include <print>
#include <string>
#include <rapidxml/rapidxml.hpp>

void parseFiles(const rapidxml::xml_document<>& doc, std::vector<cppParser>& outfiles, const std::filesystem::path& projPath)
{
	for (auto itemGroup = doc.first_node("Project")->first_node("ItemGroup");
		itemGroup;
		itemGroup = itemGroup->next_sibling("ItemGroup"))
	{
		for (auto clCompile = itemGroup->first_node("ClCompile");
			clCompile;
			clCompile = clCompile->next_sibling("ClCompile"))
		{
			outfiles.push_back(cppParser(projPath / clCompile->first_attribute("Include")->value()));
		}
	}
}

void parseVcpkgInfo(const rapidxml::xml_document<>& doc, bool& isVcpkgEnabled)
{
	for (auto propertyGroup = doc.first_node("Project")->first_node("PropertyGroup");
		propertyGroup;
		propertyGroup = propertyGroup->next_sibling("PropertyGroup"))
	{
		auto label = propertyGroup->first_attribute("Label");
		if (label && strstr(label->value(), "Vcpkg"))
		{
			//VcpkgEnabled defaults to true if it doesn't exist
			auto vcpkgEnabled = propertyGroup->first_node("VcpkgEnabled");
			isVcpkgEnabled = vcpkgEnabled ? strstr(vcpkgEnabled->value(), "true") : true;
			return;
		}
	}
}

void parsePrecompiledHeader()
{
	//for (auto& itemDefinitionGroup : _doc.child("Project").children("ItemDefinitionGroup")) {
	//	auto pchNode = itemDefinitionGroup.child("ClCompile").child("PrecompiledHeader");
	//	if (pchNode) {
	//		_precompiledHeaderFile = pchNode.child_value();
	//		break;
	//	}
	//}
}

void parseLanguageStandard(const rapidxml::xml_document<>& doc, std::string &languageStandard)
{
	for (auto propertyGroup = doc.first_node("Project")->first_node("ItemDefinitionGroup");
		propertyGroup;
		propertyGroup = propertyGroup->next_sibling("ItemDefinitionGroup"))
	{
		auto clCompile = propertyGroup->first_node("ClCompile");

		if (clCompile)
		{
			auto langStdNode = clCompile->first_node("LanguageStandard");

			if (langStdNode)
			{
				languageStandard = langStdNode->value();
				break;
			}
		}
	}
}

void parseOutputType(const rapidxml::xml_document<>& doc, std::string& outputType)
{
	for (auto propertyGroup = doc.first_node("Project")->first_node("PropertyGroup");
		propertyGroup;
		propertyGroup = propertyGroup->next_sibling("PropertyGroup"))
	{
		auto config = propertyGroup->first_node("ConfigurationType");
		if (config)
		{
			outputType = config->value();
			return;
		}
	}
}

void parseIncludeDirectories()
{
	//for (auto& itemDefinitionGroup : _doc.child("Project").children("ItemDefinitionGroup")) {
	//	for (auto& clCompile : itemDefinitionGroup.children("ClCompile")) {
	//		for (auto& includeDir : clCompile.children("AdditionalIncludeDirectories")) {
	//			_includeDirs.push_back(includeDir.child_value());
	//		}
	//	}
	//}
}

bool parseVCProj(std::filesystem::path& projPath, std::vector<cppParser>& outfiles)
{
	assert(std::filesystem::exists(projPath));

	std::print("Parsing project: {}\n", projPath.string().c_str());

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

	rapidxml::xml_document<> doc;

	doc.parse<0>(truefiledata);

	bool isVcpkgEnabled = false;
	std::string languageStandard;
	std::string outputType;

	parseFiles(doc, outfiles, projPath.parent_path());
	parseVcpkgInfo(doc, isVcpkgEnabled);
	parsePrecompiledHeader();
	parseLanguageStandard(doc, languageStandard);
	parseOutputType(doc, outputType);
	parseIncludeDirectories();

    return true;
}

bool parseSolution(std::filesystem::path& slnPath, std::vector<cppParser>& outfiles)
{
	std::print("Parsing solution: {}\n", slnPath.string().c_str());
    std::ifstream file(slnPath);

    if (!file.is_open()) {
        std::cerr << "Unable to open file: " << slnPath << std::endl;
        return false;
    }

    std::string line;
    std::string name, path, guid;
    std::size_t pos = 0;
	std::vector<std::filesystem::path> projPaths;

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
			projPaths.push_back(path);
		}
    }

	for (auto& proj : projPaths)
	{
		if (!parseVCProj(proj, outfiles))
		{
			return false;
		}
	}

    return true;
}

