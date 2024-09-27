#include "cppParser.h"

#include <cassert>
#include <iostream>
#include <print>
#include <unordered_map>
#include <unordered_set>
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

struct Attribute
{
    std::string name;
    std::string fulltext;
};

struct JSONData
{
	yyjson_mut_val* curroot;
	yyjson_mut_doc* doc;
	CXTranslationUnit unit;
	cppParser& parser;
	std::unordered_map<std::string, std::vector<Attribute>> attributes;
	std::unordered_set<std::filesystem::path> tokenizedFiles;

	JSONData(yyjson_mut_val* data, yyjson_mut_doc* doc, CXTranslationUnit unit, cppParser& parser) : curroot(data), doc(doc), unit(unit), parser(parser){}
};

std::string getLocStr(CXSourceLocation loc, CXString spelling)
{
    CXFile file;
    unsigned int line, column, offset;
    clang_getExpansionLocation(loc, &file, &line, &column, &offset);
    return std::format("{}({},{},{}): {}", clang_getCString(clang_getFileName(file)), line, column, offset, clang_getCString(spelling));
}

//clang strips out custom attributes from the AST, so we have to scan the token stream to find them
//god it's all so ugly
class TokenHandler
{
public:
	TokenHandler(JSONData& data) : data(data)
	{
	}

	//it's assumed that all attributes will be grouped together with the identifier they are attached to before a punctuation token is found
    //i.e.:
    //[[ id1Attrib ]] class id1 { [[ thisAttribAppliesToSomethingElse ]]...
	//I'm not sure that all valid attribute sequences follow this rule, but it's a good enough heuristic for now
    void operator()(CXToken token)
    {
	    switch (currentState)
	    {
	    case state::scanning:
			if (clang_getTokenKind(token) == CXToken_Punctuation)
			{
				CXString spelling = clang_getTokenSpelling(data.unit, token);
                const char* spellc = clang_getCString(spelling);

				if (spellc[0] == '[')
				{
					currentState = state::lookingForOpenBracket;
				}
                else if (curAttributes.size() && curIDValid)
                {
	                //time to flush the attribs
                    CXString temp = clang_getTokenSpelling(data.unit, currentIdentifier);
					std::string locStr = getLocStr(clang_getTokenLocation(data.unit, currentIdentifier), temp);
					data.attributes[locStr] = curAttributes;
					curAttributes.clear();
					curIDValid = false;
                    clang_disposeString(temp);
                }
				clang_disposeString(spelling);
			}
			else if (clang_getTokenKind(token) == CXToken_Identifier)
			{
				currentIdentifier = token;
                curIDValid = true;
			}
            break;
	    case state::attribute:
			if (clang_getTokenKind(token) == CXToken_Punctuation)
			{
                CXString spelling = clang_getTokenSpelling(data.unit, token);
                const char* spellc = clang_getCString(spelling);

                if (spellc[0] == ']')
                {
                    currentState = state::lookingForCloseBracket;
                }
				//we found another attribute
                else if (spellc[0] == ',')
                {
					curAttributes.push_back(currentAttribute);
					currentAttribute.name.clear();
					currentAttribute.fulltext.clear();
                }

                clang_disposeString(spelling);
			}
            else
            {
				currentState = state::gatheringAttributeName;
            }
            break;
		case state::lookingForOpenBracket:
			{
	            CXString spelling = clang_getTokenSpelling(data.unit, token);
	            const char* spellc = clang_getCString(spelling);

	            if (spellc[0] == '[')
	            {
	                currentState = state::gatheringAttributeName;
	            }
	            else
	            {
	                currentState = state::scanning;
	            }

	            clang_disposeString(spelling);
			}
            break;
		case state::lookingForCloseBracket: 
			//we're in the attribute sequence at this point, but we might have just exited an array inside the sequence, look for the double close bracket
			{
	            CXString spelling = clang_getTokenSpelling(data.unit, token);
	            const char* spellc = clang_getCString(spelling);

	            if (spellc[0] == ']')
	            {
                    currentState = state::scanning;

                    if (currentAttribute.name.size())
                    {
                        curAttributes.push_back(currentAttribute);
                        currentAttribute.name.clear();
                        currentAttribute.fulltext.clear();
                    }
	            }
	            else
	            {
	                currentState = state::attribute;
	            }

	            clang_disposeString(spelling);
			}
            break;
		case state::gatheringAttributeName:
            {
	            CXString spelling = clang_getTokenSpelling(data.unit, token);
	            const char* spellc = clang_getCString(spelling);

	            if (clang_getTokenKind(token) == CXToken_Punctuation)
	            {
	                CXString spelling = clang_getTokenSpelling(data.unit, token);
	                const char* spellc = clang_getCString(spelling);

	                if (spellc[0] == ':' && spellc[1] == ':')
	                {
                        currentAttribute.name += spellc;
                        currentAttribute.fulltext += spellc;
	                }
	                //we found another attribute
	                else if (spellc[0] == ',')
	                {
	                    curAttributes.push_back(currentAttribute);
	                    currentAttribute.name.clear();
	                    currentAttribute.fulltext.clear();
	                }
                    else
                    {
                        if (spellc[0] == '(' || spellc[0] == '[' || spellc[0] == '{')
                        {
                            numParens++;
                        }
                        else if (spellc[0] == ')' || spellc[0] == ']' || spellc[0] == '}')
                        {
                            numParens--;
                        }

                        currentAttribute.fulltext += spellc;
						currentState = state::gatheringAttributeDetails;
                    }
	            }
	            else
	            {
                    currentAttribute.name += spellc;
					currentAttribute.fulltext += spellc;
	            }

	            clang_disposeString(spelling);
			}
            
            break;
		case state::gatheringAttributeDetails:
		{
			CXString spelling = clang_getTokenSpelling(data.unit, token);
			const char* spellc = clang_getCString(spelling);

			if (clang_getTokenKind(token) == CXToken_Punctuation)
			{
				if (numParens == 0 && spellc[0] == ',')
				{
                    curAttributes.push_back(currentAttribute);
                    currentAttribute.name.clear();
                    currentAttribute.fulltext.clear();
					currentState = state::gatheringAttributeName;
				}
                else if (numParens == 0 && spellc[0] == ']')
                {
                    curAttributes.push_back(currentAttribute);
                    currentAttribute.name.clear();
                    currentAttribute.fulltext.clear();
                    currentState = state::lookingForCloseBracket;
                }
                else if (spellc[0] == '(' || spellc[0] == '[' || spellc[0] == '{')
                {
                    numParens++;
                    currentAttribute.fulltext += spellc;
                }
                else if (spellc[0] == ')' || spellc[0] == ']' || spellc[0] == '}')
                {
                    numParens--;
                    currentAttribute.fulltext += spellc;
                }
				else
				{
					currentAttribute.fulltext += spellc;
				}
			}
			else
			{
				currentAttribute.fulltext += spellc;
			}

			clang_disposeString(spelling);
		}
            break;
		default:
            assert(false);
	    }
    }
private:
    void printToken(CXToken token)
    {

        CXString spelling = clang_getTokenSpelling(data.unit, token);
        CXTokenKind kind = clang_getTokenKind(token);

        std::cout << clang_getCString(spelling) << " (";

        switch (kind)
        {
        case CXToken_Punctuation:
            std::cout << "Punctuation";
            break;
        case CXToken_Keyword:
            std::cout << "Keyword";
            break;
        case CXToken_Identifier:
            std::cout << "Identifier";
            break;
        case CXToken_Literal:
            std::cout << "Literal";
            break;
        case CXToken_Comment:
            std::cout << "Comment";
            break;
        default:
            std::cout << "Unknown";
        }

        std::cout << ")" << std::endl;
    }

	enum class state
    {
	    scanning,
        attribute,
		lookingForOpenBracket,
		lookingForCloseBracket,
        gatheringAttributeName,
        gatheringAttributeDetails
    };

	JSONData& data;
	state currentState = state::scanning;
	Attribute currentAttribute;
	std::vector<Attribute> curAttributes;
	CXToken currentIdentifier;
	bool curIDValid = false;
    int numParens = 0;
};

void scanWholeFileForAttribs(CXFile file, JSONData& data)
{
    // Get the source range of the file
    CXString fileName = clang_getFileName(file);
    CXSourceLocation start = clang_getLocationForOffset(data.unit, file, 0);
    CXSourceLocation end = clang_getLocationForOffset(data.unit, file, static_cast<unsigned>(std::filesystem::file_size(clang_getCString(fileName))));

    clang_disposeString(fileName);

    CXSourceRange fileRange = clang_getRange(start, end);

    // Tokenize the source range
    CXToken* tokens = nullptr;
    unsigned int numTokens = 0;
    clang_tokenize(data.unit, fileRange, &tokens, &numTokens);

	//ez state machine to handle the tokens
	TokenHandler tokenHandler(data);

    // Scan the tokens for attributes
    for (unsigned int i = 0; i < numTokens; ++i)
    {
		CXToken token = tokens[i];
		tokenHandler(token);
    }

    clang_disposeTokens(data.unit, tokens, numTokens);
}

CXChildVisitResult jsonConverter(CXCursor cursor, CXCursor parent, CXClientData clientData)
{
    JSONData* data = static_cast<JSONData*>(clientData);

    yyjson_mut_val* cursorObj = yyjson_mut_obj(data->doc);
    CXString usr = clang_getCursorUSR(cursor);

	if (yyjson_mut_obj_get(data->curroot, clang_getCString(usr)))
	{
		//we've already seen this cursor, so don't bother with it
		return CXChildVisit_Continue;
	}

    yyjson_mut_val* key = yyjson_mut_str(data->doc, clang_getCString(usr));

	//check if the file this cursor is in has already had it's attribs parsed
    CXSourceLocation loc = clang_getCursorLocation(cursor);
    CXFile file;
    unsigned int line, column, offset;
    clang_getExpansionLocation(loc, &file, &line, &column, &offset);

    CXString fileName = clang_getFileName(file);

    yyjson_mut_val* locVal = yyjson_mut_obj(data->doc);

    yyjson_mut_obj_add_strcpy(data->doc, locVal, "file", clang_getCString(fileName));
    yyjson_mut_obj_add_int(data->doc, locVal, "line", line);
    yyjson_mut_obj_add_int(data->doc, locVal, "column", column);
    yyjson_mut_obj_add_int(data->doc, locVal, "offset", offset);

    yyjson_mut_obj_add_val(data->doc, cursorObj, "location", locVal);

	if (data->tokenizedFiles.find(clang_getCString(fileName)) == data->tokenizedFiles.end())
	{
        data->tokenizedFiles.insert(clang_getCString(fileName));
		scanWholeFileForAttribs(file, *data);
	}

	clang_disposeString(fileName);
    
    // Get the spelling of the cursor
    CXString spelling = clang_getCursorSpelling(cursor);
    // Get the kind of the cursor
    CXCursorKind kind = clang_getCursorKind(cursor);
    CXString kindSpelling = clang_getCursorKindSpelling(kind);
    CXString resultTypeSpelling = clang_getTypeSpelling(clang_getCursorResultType(cursor));
    CXString typeSpelling = clang_getTypeSpelling(clang_getCursorType(cursor));

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

	//handle attributes
	std::string locStr = getLocStr(loc, spelling);

    if (data->attributes.count(locStr))
    {
		yyjson_mut_val* attribs = yyjson_mut_obj(data->doc);
		for (auto& attrib : data->attributes[locStr])
		{
			char* key = new char[attrib.name.size() + 1];
			strcpy_s(key, attrib.name.size() + 1, attrib.name.c_str());
			key[attrib.name.size()] = '\0';

			yyjson_mut_obj_add_strncpy(data->doc, attribs, key, attrib.fulltext.c_str(), attrib.fulltext.size());
		}

		yyjson_mut_obj_add(cursorObj, yyjson_mut_str(data->doc, "attributes"), attribs);
    }

    yyjson_mut_val* children = yyjson_mut_obj(data->doc);
    JSONData temp(children, data->doc, data->unit, data->parser);

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
    JSONData temp(outdata, doc, unit, *this);

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
