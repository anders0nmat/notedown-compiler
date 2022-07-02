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

	See examples/ for example of use
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

	std::unordered_map<std::string, _ASTElement *> iddef;
	std::unordered_map<std::string, ASTModFunc> modFuncs;

	std::unordered_set<int> symbols;

	std::unordered_map<std::string, std::string> emojis;
	ASTPlainText temp_emoji;

	HighlighterEngine syntax_highlighter;
	std::unique_ptr<ASTContainerSyntaxHighlight> syntax_highlighter_ptr;

	std::stringbuf buf;
	
	void addSymbols(std::string str);

	void addFromFile(std::string filename, size_t order);

	_ASTElement * handleRequest(std::string request);
	ASTModFunc handleModRequest(std::string request);
public:
	
	NotedownCompiler() : syntax_highlighter_ptr(std::make_unique<ASTContainerSyntaxHighlight>(&syntax_highlighter)) {}

	/**
	 * Constructs the specified handler 
	 * @param <Cl> Class inheriting ParserHandler
	 * @param name A unique name for identifing 
	 * @return Success, if name is unique
	 */
	template<class Cl>
	bool addHandler(std::string name) {
		return addHandler(name, std::make_unique<Cl>());
	}

	/**
	 * Inserts handler with unique name
	 * @param name A unique name for identifing
	 * @param handler A unique_ptr containing a Class inheriting ParserHandler
	 * @return Success, if name is unique
	 */
	bool addHandler(std::string name, std::unique_ptr<ParserHandler> handler);

	/**
	 * Registers an alternative name
	 * @param alias A unique alias for identifing
	 * @param name The name the alias is for
	 * @return Success, if alias is unique
	 */
	bool addHandlerAlias(std::string alias, std::string name);

	/**
	 * Constructs the specified inline handler 
	 * @param <Cl> Class inheriting InlineHandler
	 * @param name A unique name for identifing 
	 * @return Success, if name is unique
	 */
	template<class Cl>
	bool addInlineHandler(std::string name) {
		return addInlineHandler(name, std::make_unique<Cl>());
	}

	/**
	 * Inserts inline handler with unique name
	 * @param name A unique name for identifing
	 * @param handler A unique_ptr containing a Class inheriting InlineHandler
	 * @return Success, if name is unique
	 */
	bool addInlineHandler(std::string name, std::unique_ptr<InlineHandler> handler);

	/**
	 * Registers an alternative name
	 * @param alias A unique alias for identifing
	 * @param name The name the alias is for
	 * @return Success, if alias is unique
	 */
	bool addInlineHandlerAlias(std::string alias, std::string name);

	/**
	 * Default handlers are all handlers needed to support defined features

	 * See docs/syntax-elements.md
	 */
	void addDefaultHandlers();

	/**
	 * Adds an lookup table for emoji shortcodes
	 */
	void addEmojiLUT(std::string filename);

	/**
	 * Adds a list of lookup tables for emoji shortcodes.
	 * @param filenames List of valid filenames
	 * @param multithread Whether each file should be parsed with a separate thread (can speed up compilation)
	 */
	void addEmojiLUT(std::vector<std::string> filenames);

	/**
	 * Adds a language syntax file
	 * @param filename Path(s) to file
	 */
	void addSyntax(std::string filename);
	void addSyntax(std::vector<std::string> filenames);

	/**
	 * Adds a modifying function to use in command blocks
	 */
	void addModifierFunc(std::string name, ASTModFunc func);
	void addModifierFunc(std::string name, std::function<void(NotedownCompiler *, _ASTElement *, ASTProcess, std::vector<std::string> &)> func);

	/**
	 * Adds a generating function to use in command blocks
	 */
	void addGeneratorFunc(std::string name, ASTModFunc func);
	void addGeneratorFunc(std::string name, std::function<void(NotedownCompiler *, _ASTElement *, ASTProcess, std::vector<std::string> &)> func);

	/**
	 * Parse filename and add created document to list of documents
	 * @param filename A valid filename containing Notedown Syntax
	 */
	void addFile(std::string filename);

	/**
	 * Parse filenames and add to document list (in the order specified).
	 * @param filenames List of valid filenames
	 * @param multithread Whether each file should be parsed with a separate thread (can speed up compilation)
	 */
	void addFile(std::vector<std::string> & filenames, bool multithread = true);

	/**
	 * Processes the AST. Goes through several steps to resolve dependencies and execute commands
	 */
	void prepareAST();

	/**
	 * Compiles all documents to html.
	 * @return Pointer to stringbuf, valid until next call to a getHtml-Method
	 */
	std::stringbuf * getRawHtml();

	/**
	 * Compiles specified document to html
	 * @param index Index of document (identical to order added)
	 * @return Pointer to stringbuf, valid until next call to a getHtml-Method
	 */
	std::stringbuf * getRawHtml(size_t index);

	/**
	 * Get document AST for direct manipulation
	 * @param index Index of document (identical to order added)
	 * @return Reference to pointer of ASTDocument
	 */
	std::unique_ptr<ASTDocument> & getDocument(size_t index);

	/**
	 * Returns the amount of documents (corresponds with loaded files)
	 */
	size_t documentCount();
};

namespace Notedown {
	/**
	 * Converts raw into a valid id.
	 * This includes converting it to lowercase, replacing Spaces with '-'
	 * and stripping invalid characters.
	 * Valid Characters: A-Z a-z 0-9 SPACE _ -
	 */
	std::string makeId(std::string raw);

	/**
	 * Checks whether id is a valid id string
	 * Valid id characters: a-z 0-9 _ -
	 */
	bool isId(std::string id);
}
