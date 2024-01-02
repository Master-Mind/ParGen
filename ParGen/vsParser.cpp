#include "vsParser.h"

#include <iostream>
#include <fstream>
#include <print>
#include <string>
#include <pugixml.hpp>

vsProject::vsProject()
{
}

vsProject::vsProject(std::string name, std::string path) : _name(name), _path(path)
{
	
}

vsProject::~vsProject()
{
}

bool vsProject::parse(const char* projPath)
{
	std::print("Parsing project: {}\n", projPath);

    if (projPath != nullptr)
    {
	    projPath = _path.c_str();
	}

    //parse the project file with pugixml
    pugi::xml_document doc;


    return true;
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


    return true;
}
