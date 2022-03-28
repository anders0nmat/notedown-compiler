#pragma once

#include <memory>
#include <vector>
#include <string>
#include <unordered_set>
#include <unordered_map>
#include <initializer_list>
#include <mutex>
#include <sstream>

#include "AST.hpp"
#include "notedown-templates.hpp"

/*
	Class for compiling notedown documents into html
*/
class NotedownCompiler {
protected:
	friend class Parser;

	std::mutex mtx_documents;
	std::vector<std::unique_ptr<ASTDocument>> documents;

	std::unordered_map<std::string, size_t> handlerAlias;
	std::vector<std::unique_ptr<ParserHandler>> handlerList;

	std::unordered_map<std::string, size_t> inlineHandlerAlias;
	std::vector<std::unique_ptr<InlineHandler>> inlineHandlerList;

	std::unordered_set<int> symbols;

	std::stringbuf buf;
	
	void addSymbols(std::string str);

	void addFromFile(std::string filename, size_t order);

public:
	
	NotedownCompiler() {}

	template<class Cl>
	bool addHandler(std::string name) {
		return addHandler(name, std::make_unique<Cl>());
	}
	bool addHandler(std::string name, std::unique_ptr<ParserHandler> handler);
	bool addHandlerAlias(std::string alias, std::string name);

	template<class Cl>
	bool addInlineHandler(std::string name) {
		return addInlineHandler(name, std::make_unique<Cl>());
	}
	bool addInlineHandler(std::string name, std::unique_ptr<InlineHandler> handler);
	bool addInlineHandlerAlias(std::string alias, std::string name);

	void addDefaultHandlers();

	void addFile(std::string filename);
	void addFile(std::initializer_list<std::string> filenames, bool multithread);

	std::stringbuf * getRawHtml();
	std::stringbuf * getRawHtml(size_t index);

	std::unique_ptr<ASTDocument> & getDocument(size_t index);
};


namespace Notedown {
	/*
		Converts raw into a valid id.
		This includes converting it to lowercase, replacing Spaces with '-'
		and stripping invalid characters.
		Valid Characters: A-Z a-z 0-9 SPACE _ -
	*/
	std::string makeId(std::string raw);

	/*
		Checks whether id is a valid id string
		Valid id characters: a-z 0-9 _ -
	*/
	bool isId(std::string id);

}