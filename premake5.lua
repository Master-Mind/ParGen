workspace "ParGen"
	configurations {"Debug", "Release"}
	platforms "Win64"
	
	filter "configurations:Debug"
		cppdialect "C++latest"
		defines { "DEBUG" }
		runtime "Debug"
		symbols "On"

	filter "configurations:Release"
		cppdialect "C++latest"
		optimize "On"
		defines { "RELEASE" }
		runtime "Release"

	filter { "platforms:Win64" }
		system "Windows"
		architecture "x64"

project "ParGen"
	language "C++"
	kind "ConsoleApp"
	location "./ParGen"
	files {"./ParGen/*.h", "./ParGen/*.cpp" }
	flags { "MultiProcessorCompile" }
	includedirs { "./Clang/include" }
	links { "libclang" }
	debugargs { "--files", "../Test/Test.vcxproj" }
	filter "configurations:Debug"
		libdirs { "./Clang/Lib/Debug" }
	filter "configurations:Release"
		libdirs { "./Clang/Lib/Release" }
	
project "Test"
	language "C++"
	kind "ConsoleApp"
	location "./Test"
	files {"./Test/*.h", "./Test/*.cpp" }
	flags { "MultiProcessorCompile" }