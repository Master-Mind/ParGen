#include <cassert>
#include <filesystem>
#include <iostream>
#include <print>
#include <thread>
#include <argparse/argparse.hpp>

#include "cppParser.h"
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

	std::vector<cppParser> finalFileList;

	for(auto& file : fileNames)
	{
		if (file.extension() == ".cpp")
		{
			finalFileList.emplace_back(std::move(file));
		}
		else if (file.extension() == ".sln")
		{
			parseSolution(file, finalFileList);
		}
		else if (file.extension() == ".vcxproj")
		{
			parseVCProj(file, finalFileList);
		}
	}

	inja::json data;

	//divvy up the files to multiple threads
	std::size_t numPerThread = std::thread::hardware_concurrency() > finalFileList.size() ? 1 : finalFileList.size() / std::thread::hardware_concurrency();
	std::vector<std::thread> threads(std::thread::hardware_concurrency() > finalFileList.size() ? 1 : std::thread::hardware_concurrency());
	std::vector<cppParser*> threadFiles;

	threadFiles.reserve(numPerThread);

	for (int i = 0; i < finalFileList.size(); )
	{
		for ( ;  i < finalFileList.size() && threadFiles.size() < numPerThread; i++)
		{
			threadFiles.push_back(&finalFileList[i]);
		}

		threads.emplace_back([](std::vector<cppParser*> _threadFiles)
			{
				for (auto& file : _threadFiles)
				{
					bool success = file->parse();
					assert(success);
				}
			}, threadFiles);

		threadFiles.clear();
	}

	for (auto& thread : threads)
	{
		if (thread.joinable())
		{
			thread.join();
		}
	}

	for (auto& file : finalFileList)
	{
		file.fillInData(data);
	}

	std::cout << data.dump(4, ' ') << std::endl;

	return 0;
}
