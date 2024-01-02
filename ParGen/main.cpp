#include <cassert>
#include <filesystem>
#include <iostream>
#include <print>
#include <clang-c/Index.h>
#include <argparse/argparse.hpp>
#include "vsParser.h"

int main(int argc, const char *argv[])
{
	std::print("Running in folder: {}\n", std::filesystem::current_path().string());
    argparse::ArgumentParser program("ParGen");

    program.add_argument("-f", "--files")
		.help("Comma delimited list of c++ files and or project files");

    try
    {
	    program.parse_args(argc, argv);
	}
	catch (const std::exception& err)
	{
		std::cout << err.what() << std::endl;
		std::cout << program;
        return 1;
	}

    std::string files = program.get<std::string>("--files");

    //walk through the comma delimited list of files and parse them
    std::vector<std::filesystem::path> fileNames;

    std::size_t pos = 0;

    do
    {
		std::size_t newpos = files.find(',', pos);
	    std::string token = files.substr(pos, newpos - pos);
		fileNames.emplace_back(token);
		pos = newpos + 1;
	} while (pos != std::string::npos + 1); //yes, it's an overflow, and no, I don't care

	for (auto& file : fileNames)
	{
		if (file.extension() == ".vcxproj")
		{
			vsProject proj;
			assert(proj.parse(file.string().c_str()));
		}
		else if (file.extension() == ".sln")
		{
			vsSolution sln;
			assert(sln.parse(file.string().c_str()));
		}
		else
		{
			std::cout << "Unknown file type: " << file.string() << std::endl;
		}	
	}

	return 0;
}
