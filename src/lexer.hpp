#pragma once
#include <string>
#include <fstream>
#include <unordered_map>
#include <tuple>
#include <unordered_set>
#include <functional>

#include "AST.hpp"

#include "notedown-compiler.hpp"
#include "notedown-templates.hpp"

enum Token : int {
	
	tokEOF = -1, // End of File
	tokText = -2, // lastString contains text
	tokNumber = -3, // lastInt contains number, lastString contains string read (reads <Number>.)
	tokSpace = -4, // lastString contains ' ', lastInt contains amount of spaces
	tokNewline = -5, // Newline Character read (\n)
	tokSym = -6, // Indicates formatting Symbol, lastString contains it, lastInt contains amount
	_tokEscape = -7, // Used internally for escape sequences. Will never be the state of lastToken after gettok()
};

/*
	Wrapper class for creating an AST
*/
class Parser {
protected:
	NotedownCompiler * compiler;

	std::istream * input;

	// std::unordered_map<std::string, size_t> handlerAlias;
	// std::vector<std::unique_ptr<ParserHandler>> handlerList;

	// std::unordered_set<int> symbols;

	// std::unordered_map<std::string, size_t> inlineHandlerAlias;
	// std::vector<std::unique_ptr<InlineHandler>> inlineHandlerList;

	std::unique_ptr<ParserHandler> _lastHandler = nullptr;
	std::unique_ptr<ASTDocument> document = nullptr;

	int _lastChar = 0;

	std::vector<std::unique_ptr<ASTDocument>> documents;

	std::unique_ptr<ASTPlainText> _parsePlainText();
	std::unique_ptr<_ASTInlineElement> _parseLine(bool allowLb = true);

	void puttok();

	// void addSymbols(std::string str);

public:

	std::string lastString;
	int lastInt;
	Token lastToken;


	Parser(NotedownCompiler * compiler, std::istream * input) : compiler(compiler), input(input) {}
	~Parser() = default;

	Token peektok(int chr);
	Token peektok();

	int peekchar();

	Token gettok();
	Token gettok(int amount);
	void getchar();
	int currchar();

	std::string escaped(int chr);

	/*
		@param allowRange Whether delimiter is allowed if in ""
		@param delimiter Symbol to end. Will consume delimiter
		@return Unformatted String and whether it ended because of a newline
	*/
	std::tuple<std::string, bool> extractText(bool allowRange, std::string delimiter);

	std::tuple<std::string, std::string, bool> parseLink(char delim);

	/*
		Reads Text literally (no Sym or Space collapsing) until condition is met or EOF/EOL occured
		Doesnt consume ending token if condition caused end, does consume newline
		@param condition returns true if reading should end
		@return String read and whether it ended on condition (=true) or EOF/EOL (=false)
	*/
	std::tuple<std::string, bool> readUntil(std::function<bool(Parser *)> condition);

	std::unique_ptr<ParserHandler> findNextHandler();
	std::unique_ptr<ParserHandler> findNextHandler(std::string name);

	std::unique_ptr<InlineHandler> findNextInlineHandler();
	std::unique_ptr<InlineHandler> findNextInlineHandler(std::string name);

	bool addToDocument(std::unique_ptr<_ASTElement> element);

	/*
		@param allowLb Inserts forced linebreak if line ends with <space><space><newline>
		@param unknownAsText Also returns on unknown symbols (symbols that are not handled by inlineHandlers)
		@param allowInlineStyling Whether other inline styling elements are allowed. Printed as literal text
		@param inlineSymReturn Returns if this Inline-Sym occures
		@param symReturn Returns if this Sym occures
		@returns If second parameter is true, it ended on linebreak (But didn't consume it!). If False it ended on symReturn or unknown Symbol (Only if unknownAsText == false)
	*/
	std::tuple<std::unique_ptr<ASTInlineText>, bool> parseText(
		bool allowLb = true, bool unknownAsText = true, bool allowInlineStyling = true, int symReturn = 0);

	// std::tuple<std::unique_ptr<ASTInlineText>, bool> parseText(allowLb, unknownAsText, allowInlineStyling, inlineSymReturn, symReturn)

	// template<class Cl>
	// bool addHandler(std::string name) {
	// 	return addHandler(name, std::make_unique<Cl>());
	// }

	// bool addHandler(std::string name, std::unique_ptr<ParserHandler> handler);
	// bool addHandlerAlias(std::string alias, std::string name);

	// template<class Cl>
	// bool addInlineHandler(std::string name) {
	// 	return addInlineHandler(name, std::make_unique<Cl>());
	// }

	// bool addInlineHandler(std::string name, std::unique_ptr<InlineHandler> handler);
	// bool addInlineHandlerAlias(std::string alias, std::string name);

	/*
		Has consumed newline if second return value is true.
		@returns unique_ptr : Element to insert or nullptr. bool : Whether the current Handler was finished.
	*/
	std::tuple<std::unique_ptr<_ASTElement>, bool> parseLine(std::unique_ptr<ParserHandler> & lastHandler);
	std::unique_ptr<_ASTElement> parseLine();

	void createDocument();
	void parseDocument();
	std::unique_ptr<ASTDocument> & getDocument();

	// bool parseFile(std::string filename);
	// bool parseStream(std::istream & stream);

	// void addDefaultHandlers();
};