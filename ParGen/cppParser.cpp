#include "cppParser.h"

#include <iostream>
#include <print>
#include <clang-c/Index.h>

// Forward declaration of the AST visitor function
CXChildVisitResult dumper(CXCursor cursor, CXCursor parent, CXClientData clientData);

void dumpAST(const std::string& filename) {
    // Create an index
    CXIndex index = clang_createIndex(0, 0);

    // Parse the source file and create a translation unit
    CXTranslationUnit unit = clang_parseTranslationUnit(
        index,
        filename.c_str(),
        nullptr, 0,
        nullptr, 0,
        CXTranslationUnit_SkipFunctionBodies | CXTranslationUnit_KeepGoing // Skip function bodies
    );

    if (unit == nullptr) {
        std::cerr << "Unable to parse translation unit. Quitting." << std::endl;
        return;
    }

    // Get the root cursor of the translation unit
    CXCursor rootCursor = clang_getTranslationUnitCursor(unit);

    // Visit the children of the root cursor
    clang_visitChildren(
        rootCursor,
        dumper,
        nullptr
    );

    // Clean up
    clang_disposeTranslationUnit(unit);
    clang_disposeIndex(index);
}

CXChildVisitResult dumper(CXCursor cursor, CXCursor parent, CXClientData clientData) {
    // Get the kind of the cursor
    CXCursorKind kind = clang_getCursorKind(cursor);

    // Get the spelling of the cursor
    CXString spelling = clang_getCursorSpelling(cursor);
    CXString kindSpelling = clang_getCursorKindSpelling(kind);
    std::cout << clang_getCString(spelling) << " (" << clang_getCString(kindSpelling) << ")" << std::endl;
    clang_disposeString(spelling);

    // Recursively visit the children of this cursor
    return CXChildVisit_Recurse;
}

bool cppParser::parse(const char* projPath)
{
	std::print("Parsing file: {}\n", projPath);

    dumpAST(projPath);

	return true;
}
