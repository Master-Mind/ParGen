#include "cppParser.h"

#include <iostream>
#include <print>
#include <clang-c/Index.h>

// Forward declaration of the AST visitor functions
CXChildVisitResult dumper(CXCursor cursor, CXCursor parent, CXClientData clientData);
CXChildVisitResult jsonConverter(CXCursor cursor, CXCursor parent, CXClientData clientData);

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

struct JSONData
{
	yyjson_mut_val* curroot;
	yyjson_mut_doc* doc;

	JSONData(yyjson_mut_val* data, yyjson_mut_doc* doc) : curroot(data), doc(doc) {}
};

CXChildVisitResult jsonConverter(CXCursor cursor, CXCursor parent, CXClientData clientData)
{
    JSONData* data = static_cast<JSONData*>(clientData);
	// Get the kind of the cursor
    CXCursorKind kind = clang_getCursorKind(cursor);
    
	CXString usr = clang_getCursorUSR(cursor);
    
    // Get the spelling of the cursor
    CXString spelling = clang_getCursorSpelling(cursor);
    CXString kindSpelling = clang_getCursorKindSpelling(kind);
    CXString resultTypeSpelling = clang_getTypeSpelling(clang_getCursorResultType(cursor));
    CXString typeSpelling = clang_getTypeSpelling(clang_getCursorType(cursor));
    
    yyjson_mut_val* key = yyjson_mut_str(data->doc, clang_getCString(usr));

	yyjson_mut_val* cursorObj = yyjson_mut_obj(data->doc);

    if (!CXStrNullOrEmpty(spelling))
    {
        yyjson_mut_obj_add_str(data->doc, cursorObj, "spelling", clang_getCString(spelling));
    }

    if (!CXStrNullOrEmpty(resultTypeSpelling))
    {
        yyjson_mut_obj_add_str(data->doc, cursorObj, "result_type", clang_getCString(resultTypeSpelling));
    }

    if (!CXStrNullOrEmpty(kindSpelling))
    {
        yyjson_mut_obj_add_str(data->doc, cursorObj, "kind", clang_getCString(kindSpelling));
    }

    if (!CXStrNullOrEmpty(typeSpelling))
    {
        yyjson_mut_obj_add_str(data->doc, cursorObj, "type", clang_getCString(typeSpelling));
    }

    switch (clang_getCXXAccessSpecifier(cursor))
    {
    case CX_CXXInvalidAccessSpecifier: break;
    case CX_CXXPublic:
        yyjson_mut_obj_add_str(data->doc, cursorObj, "access", "public");
        break;
    case CX_CXXProtected:
        yyjson_mut_obj_add_str(data->doc, cursorObj, "access", "protected");
        break;
    case CX_CXXPrivate:
        yyjson_mut_obj_add_str(data->doc, cursorObj, "access","private");
        break;
    default: ;
    }

    //don't dispose of strings so that we don't have to do as many deep copies
    //clang_disposeString(resultTypeSpelling);
    //clang_disposeString(kindSpelling);
    //clang_disposeString(typeSpelling);
    //clang_disposeString(spelling);
    //clang_disposeString(kind);

    yyjson_mut_val* children = yyjson_mut_obj(data->doc);
    JSONData temp(children, data->doc);

    //recurse
    clang_visitChildren(
        cursor,
        jsonConverter,
		&temp
    );

    if (!yyjson_mut_obj_size(children))
    {
		//yyson uses memory pools, so no need to free the children
    }
    else
    {
        yyjson_mut_val* childkey = yyjson_mut_str(data->doc, "children");
        yyjson_mut_obj_add(cursorObj, childkey, children);
    }

    //clang_disposeString(usr);
    yyjson_mut_obj_add(data->curroot, key, cursorObj);

    // Visit the siblings of this cursor
    return CXChildVisit_Continue;
}

bool cppParser::parse()
{
	std::print("Parsing file: {}\n", _path.string().c_str());
    // Create an index
    CXIndex index = clang_createIndex(0, 0);

    // Parse the source file and create a translation unit
    unit = clang_parseTranslationUnit(
        index,
        _path.string().c_str(),
        nullptr, 0,
        nullptr, 0,
        CXTranslationUnit_SkipFunctionBodies | CXTranslationUnit_KeepGoing // Skip function bodies
    );

    if (unit == nullptr) {
        std::cerr << "Unable to parse translation unit. Quitting." << std::endl;
        return false;
    }

    clang_disposeIndex(index);

	return true;
}

void cppParser::fillInData(yyjson_mut_doc* doc, yyjson_mut_val* outdata)
{
    // Get the root cursor of the translation unit
    CXCursor rootCursor = clang_getTranslationUnitCursor(unit);
    JSONData temp(outdata, doc);

    // Visit the children of the root cursor
    clang_visitChildren(
        rootCursor,
        jsonConverter,
        &temp
    );

    // Clean up
    clang_disposeTranslationUnit(unit);
}

cppParser::cppParser(std::filesystem::path&& path, bool isVcpkgEnabled, std::string&& languageStandard) : _path(path),
	unit(nullptr)
{
}
