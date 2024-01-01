#include <iostream>
#include <clang-c/Index.h>

int main(void)
{
    CXIndex index = clang_createIndex(0, 0); //Create index
    CXTranslationUnit unit = clang_parseTranslationUnit(
        index,
        "../Test/main.cpp", nullptr, 0,
        nullptr, 0,
        CXTranslationUnit_None); //Parse "file.cpp"


    if (unit == nullptr) {
        std::cerr << "Unable to parse translation unit. Quitting.\n";
        return 0;
    }
    CXCursor cursor = clang_getTranslationUnitCursor(unit); //Obtain a cursor at the root of the translation unit

	return 0;
}