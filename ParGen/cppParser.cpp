#include "cppParser.h"

#include <iostream>
#include <print>
#include <clang-c/Index.h>

// Forward declaration of the AST visitor functions
CXChildVisitResult dumper(CXCursor cursor, CXCursor parent, CXClientData clientData);
CXChildVisitResult injaConverter(CXCursor cursor, CXCursor parent, CXClientData clientData);

void parseInternal(const std::string& filename, inja::json &data) {
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
        injaConverter,
        &data
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

bool CXStrNullOrEmpty(const CXString &str)
{
    return !(clang_getCString(str) && clang_getCString(str)[0]);
}

CXChildVisitResult injaConverter(CXCursor cursor, CXCursor parent, CXClientData clientData)
{
    inja::json& data = *static_cast<inja::json*>(clientData);
	// Get the kind of the cursor
    CXCursorKind kind = clang_getCursorKind(cursor);
    //clang_getCursor
    // Get the spelling of the cursor
    CXString spelling = clang_getCursorSpelling(cursor);
    CXString kindSpelling = clang_getCursorKindSpelling(kind);
    CXString resultTypeSpelling = clang_getTypeSpelling(clang_getCursorResultType(cursor));
    CXString typeSpelling = clang_getTypeSpelling(clang_getCursorType(cursor));

    if (!CXStrNullOrEmpty(resultTypeSpelling))
    {
        data[clang_getCString(spelling)]["result_type"] = clang_getCString(resultTypeSpelling);
    }

    if (!CXStrNullOrEmpty(kindSpelling))
    {
        data[clang_getCString(spelling)]["kind"] = clang_getCString(kindSpelling);
    }

    if (!CXStrNullOrEmpty(typeSpelling))
    {
        data[clang_getCString(spelling)]["type"] = clang_getCString(typeSpelling);
    }

    clang_disposeString(resultTypeSpelling);
    clang_disposeString(kindSpelling);
    clang_disposeString(typeSpelling);
    
    //recurse
    clang_visitChildren(
        cursor,
        injaConverter,
        &data[clang_getCString(spelling)]["children"]
    );

    if (data[clang_getCString(spelling)]["children"].empty())
    {
        //"destroy the child, kill them all" - Alex Jones
        data[clang_getCString(spelling)].erase("children");
    }

    clang_disposeString(spelling);

    // Visit the siblings of this cursor
    return CXChildVisit_Continue;
}

bool cppParser::parse(const char* projPath)
{
	std::print("Parsing file: {}\n", projPath);

    parseInternal(projPath, _data);
    std::cout << _data.dump(4, ' ') << std::endl;
	return true;
}
