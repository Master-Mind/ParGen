#include "cppParser.h"

#include <iostream>
#include <print>
#include <clang-c/Index.h>

// Forward declaration of the AST visitor functions
CXChildVisitResult dumper(CXCursor cursor, CXCursor parent, CXClientData clientData);
CXChildVisitResult injaConverter(CXCursor cursor, CXCursor parent, CXClientData clientData);

void parseInternal(const std::filesystem::path& filename, inja::json &data) {
    // Create an index
    CXIndex index = clang_createIndex(0, 0);

    // Parse the source file and create a translation unit
    CXTranslationUnit unit = clang_parseTranslationUnit(
        index,
        filename.string().c_str(),
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
    
	CXString usr = clang_getCursorUSR(cursor);
    // Get the spelling of the cursor
    CXString spelling = clang_getCursorSpelling(cursor);
    CXString kindSpelling = clang_getCursorKindSpelling(kind);
    CXString resultTypeSpelling = clang_getTypeSpelling(clang_getCursorResultType(cursor));
    CXString typeSpelling = clang_getTypeSpelling(clang_getCursorType(cursor));

    if (!CXStrNullOrEmpty(spelling))
    {
        data[clang_getCString(usr)]["spelling"] = clang_getCString(spelling);
    }

    if (!CXStrNullOrEmpty(resultTypeSpelling))
    {
        data[clang_getCString(usr)]["result_type"] = clang_getCString(resultTypeSpelling);
    }

    if (!CXStrNullOrEmpty(kindSpelling))
    {
        data[clang_getCString(usr)]["kind"] = clang_getCString(kindSpelling);
    }

    if (!CXStrNullOrEmpty(typeSpelling))
    {
        data[clang_getCString(usr)]["type"] = clang_getCString(typeSpelling);
    }

    clang_disposeString(resultTypeSpelling);
    clang_disposeString(kindSpelling);
    clang_disposeString(typeSpelling);
    clang_disposeString(spelling);
    
    //recurse
    clang_visitChildren(
        cursor,
        injaConverter,
        &data[clang_getCString(usr)]["children"]
    );

    if (data[clang_getCString(usr)]["children"].empty())
    {
        //"destroy the child, kill them all" - Alex Jones
        data[clang_getCString(usr)].erase("children");
    }

    clang_disposeString(usr);

    // Visit the siblings of this cursor
    return CXChildVisit_Continue;
}

bool cppParser::parse(inja::json& outdata)
{
	std::print("Parsing file: {}\n", _path.string().c_str());

    parseInternal(_path, outdata);

	return true;
}

cppParser::cppParser(std::filesystem::path&& path, bool isVcpkgEnabled, std::string&& languageStandard) : _path(path)
{
}
