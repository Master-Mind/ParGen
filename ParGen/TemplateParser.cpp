#include "TemplateParser.h"

#include <cassert>
#include <filesystem>
#include <iostream>
#include <ranges>
#include <unordered_map>
#include <yyjson.h>

struct templContext
{
	std::unordered_map<std::string_view, yyjson_mut_val*> variables;
};

struct templNode
{
	virtual ~templNode() = default;
	virtual void render(std::ostream& out, yyjson_mut_val* curNode, templContext& context) = 0;
};

struct parentNode : templNode
{
	virtual void render(std::ostream& out, yyjson_mut_val* curNode, templContext& context) override
	{
		out << "Parent Node\n";
		for (auto child : children)
		{
			child->render(out, curNode, context);
		}

	}

	~parentNode() override
	{
		for (auto child : children)
		{
			delete child;
		}
	}

	std::vector<templNode*> children;
};

struct textBlock : templNode
{
	textBlock(std::string_view text) : text(text) {}
	void render(std::ostream& out, yyjson_mut_val* curNode, templContext& context) override
	{
		out << text;
	}
	std::string_view text;
};

struct foreachBlock : parentNode
{
	foreachBlock(std::string_view iterName, std::string_view seachItem) : seachItem(seachItem), iterName(iterName) {}

	void renderInternal(std::ostream& out, yyjson_mut_val* curNode, templContext& context)
	{
		for (auto child : children)
		{
			child->render(out, curNode, context);
		}
	}

	void search(std::ostream& out, yyjson_mut_val* curNode, templContext& context)
	{
		yyjson_mut_val* kind = yyjson_mut_obj_get(curNode, "kind");
		if (kind && yyjson_mut_get_str(kind) == seachItem)
		{
			context.variables[iterName] = curNode;
			//render on the node
			renderInternal(out, curNode, context);
			context.variables.erase(iterName);
		}
		
		//recursively search the children
		yyjson_mut_val* key, * val;
		yyjson_mut_obj_iter iter;
		yyjson_mut_obj_iter_init(curNode, &iter);
		while ((key = yyjson_mut_obj_iter_next(&iter))) 
		{
			val = yyjson_mut_obj_iter_get_val(key);
			
			if (yyjson_mut_is_obj(val) || yyjson_mut_is_arr(val))
			{
				search(out, val, context);
			}
		}
	}

	void render(std::ostream& out, yyjson_mut_val* curNode, templContext& context) override
	{
		//out << "foreach " << iterName << " in " << seachItem;

		if (context.variables.contains(iterName))
		{
			std::cerr << "Variable already exists: " << iterName << std::endl;
			return;
		}

		search(out, curNode, context);
	}
	std::string_view seachItem;
	std::string_view iterName;
};

struct fieldBlock : templNode
{
	fieldBlock(std::string_view fieldPath) : fieldPath(fieldPath) {}
	void render(std::ostream& out, yyjson_mut_val* curNode, templContext& context) override
	{
		//out << fieldPath;
		auto tokens = std::views::split(fieldPath, '.');

		auto it = tokens.begin();

		yyjson_mut_val* curVal = curNode;

		if (context.variables.contains(std::string_view(*it)))
		{
			curVal = context.variables[std::string_view(*it)];
			it++;
		}

		//oh boy this is jank
		char temp[256];

		for (; it != tokens.end(); it++)
		{
			if (yyjson_mut_is_obj(curVal))
			{
				std::size_t len = std::string_view(*it).copy(temp, 256);
				assert(len < 256);
				temp[len] = '\0';
				curVal = yyjson_mut_obj_get(curVal, temp);
			}
			else if (yyjson_mut_is_arr(curVal))
			{
			}
			else
			{
				std::cerr << "Invalid field path: " << fieldPath << std::endl;
				return;
			}
		}

		if (yyjson_mut_is_str(curVal))
		{
			out << yyjson_mut_get_str(curVal);
		}
	}
	std::string_view fieldPath;
};

void trimWhitespace(std::string_view& str)
{
	while (str.size() && std::isspace(str.front()))
	{
		str.remove_prefix(1);
	}

	while (str.size() && std::isspace(str.back()))
	{
		str.remove_suffix(1);
	}
}

void parse(std::size_t &curpos, parentNode *parent, const std::string_view & templateData, templContext& context)
{
	using std::operator""sv;

	while (curpos < templateData.size())
	{
		std::size_t start = templateData.find("{{", curpos);

		if (start == std::string::npos)
		{
			parent->children.push_back(new textBlock{ templateData.substr(curpos) });
			curpos = std::string::npos;
			break;
		}
		
		parent->children.push_back(new textBlock{ templateData.substr(curpos, start - curpos) });

		std::size_t end = templateData.find("}}", start);

		if (end == std::string::npos)
		{
			std::cerr << "Template error: missing }}\n";
			curpos = std::string::npos;
			break;
		}

		std::string_view statement = templateData.substr(start + 2, end - start - 2);
		auto tokens = std::views::split(statement, ' ');

		if (std::string_view(*tokens.begin()) == "foreach"sv)
		{
			auto it = tokens.begin();
			it++;
			std::string_view iterName(*it);
			it++;

			if (std::string_view(*it) != "in"sv)
			{
				std::cerr << "Expected 'in' after foreach variable name\n";
				return;
			}

			it++;
			std::string_view searchItem(*it);

			foreachBlock* foreach = new foreachBlock{ iterName, searchItem };

			parent->children.push_back(foreach);

			curpos = end + 2;
			const yyjson_mut_val* temp;

			if (context.variables.contains(iterName))
			{
				std::cerr << "Variable already exists: " << iterName << std::endl;
				return;
			}
			else
			{
				//context.variables[iterName] = temp;
			}

			parse(curpos, foreach, templateData, context);

			context.variables.erase(iterName);
		}
		else if (std::string_view(*tokens.begin()) == "/foreach"sv)
		{
			curpos = end + 2;
			return;
		}
		else
		{
			trimWhitespace(statement);
			parent->children.push_back(new fieldBlock{ statement });

			curpos = end + 2;
		}
	}
}

void renderTemplate(const std::filesystem::path& templatePath, std::ostream& out, yyjson_mut_doc* doc)
{
	if (!exists(templatePath))
	{
		std::cerr << "Template file does not exist: " << templatePath << std::endl;
		return;
	}

	assert(doc);

	//load the template file
	FILE* file = nullptr;
	errno_t err = fopen_s(&file, templatePath.string().c_str(), "rb");
	size_t fileSize = std::filesystem::file_size(templatePath);
	char *buffer = new char[fileSize + 1];

	size_t numread = fread_s(buffer, fileSize, 1, fileSize, file);

	fclose(file);

	if (!numread)
	{
		std::cerr << "Failed to read template file: " << templatePath << std::endl;
		delete[] buffer;
		return;
	}

	buffer[fileSize] = '\0';

	std::string_view templateData(buffer, fileSize);

	std::size_t curpos = 0;
	parentNode root;
	templContext context;

	parse(curpos, &root, templateData, context);

	root.render(out, yyjson_mut_doc_get_root(doc), context);

	//out << templateData;
}
