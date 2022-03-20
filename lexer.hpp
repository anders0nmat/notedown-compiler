#pragma once
#include <string>
#include <fstream>
#include <unordered_map>
#include <tuple>

#include "AST.hpp"

enum Token : int {
	
	tokEOF = -1, // End of File
	tokText = -2, // lastString contains text
	tokSpace = -3, // lastString contains ' ', lastInt contains ampunt of spaces
	tokNewline = -4, // Newline Character read (\n)
	tokInlineSym = -5, // indicates inline formatting, lastString contains Symbol
	tokSym = -6, // Indicates other formatting Symbol, lastString contains it, lastInt contains amount
};

class ParserHandler;
class InlineHandler;

/*
	Wrapper class for creating an AST
*/
class Parser {
protected:
	std::ifstream input;

	std::unordered_map<std::string, size_t> handlerAlias;
	std::vector<std::unique_ptr<ParserHandler>> handlerList;
	std::string symbols = "";

	std::unordered_map<std::string, size_t> inlineHandlerAlias;
	std::vector<std::unique_ptr<InlineHandler>> inlineHandlerList;
	std::string inlineSymbols = "";

	std::unique_ptr<ParserHandler> _lastHandler = nullptr;
	std::unique_ptr<ASTDocument> document = nullptr;

	int _lastChar = 0;

	std::unique_ptr<ASTPlainText> _parsePlainText();
	std::unique_ptr<_ASTInlineElement> _parseLine(bool allowLb = true);

public:

	std::string lastString;
	int lastInt;
	Token lastToken;


	Parser(std::string filename);
	~Parser() = default;

	Token peektok(int chr);
	Token peektok();

	int peekchar();

	Token gettok();

	std::unique_ptr<ParserHandler> findNextHandler();
	std::unique_ptr<ParserHandler> findNextHandler(std::string name);

	std::unique_ptr<InlineHandler> findNextInlineHandler();
	std::unique_ptr<InlineHandler> findNextInlineHandler(std::string name);

	bool addToDocument(std::unique_ptr<_ASTElement> element);

	/*
		@param allowLb Inserts forced linebreak if line ends with <space><space><newline>
		@param unknownAsText Also returns on unknown symbols (symbols that are not handled by inlineHandlers)
		@param inlineSymReturn Returns if this Inline-Sym occures
		@returns If second parameter is true, it ended on linebreak. If False it ended on inlineSymReturn or unknown Symbol (Only if unknownAsText == true)
	*/
	std::tuple<std::unique_ptr<ASTInlineText>, bool> parseText(bool allowLb = true, bool unknownAsText = true, int inlineSymReturn = 0);


	bool addHandler(std::string name, std::unique_ptr<ParserHandler> handler);
	bool addHandlerAlias(std::string alias, std::string name);
	bool addInlineHandler(std::string name, std::unique_ptr<InlineHandler> handler);
	bool addInlineHandlerAlias(std::string alias, std::string name);

	/*
		Has consumed newline if second return value is true.
		@returns unique_ptr : Element to insert or nullptr. bool : Whether the current Handler was finished.
	*/
	std::tuple<std::unique_ptr<_ASTElement>, bool> parseLine(std::unique_ptr<ParserHandler> & lastHandler);
	std::unique_ptr<_ASTElement> parseLine();

	void createDocument();
	void parseDocument();
	std::unique_ptr<ASTDocument> & getDocument();

	void addDefaultHandlers();
};


/*
	Template Class for Inline Parser Hooks
*/
class InlineHandler {
protected:

public:

	InlineHandler() {}

	virtual ~InlineHandler() {}

	virtual std::unique_ptr<InlineHandler> createNew();

	virtual std::string triggerChars();

	virtual bool canHandle(Parser * lex);

	virtual std::tuple<std::unique_ptr<_ASTInlineElement>, bool> handle(Parser * lex);

	virtual std::unique_ptr<_ASTElement> finish(Parser * lex);
};

/*
	Template Class for Parser Hooks
*/
class ParserHandler {
protected:

public:

	ParserHandler() {}

	virtual ~ParserHandler() {}

	virtual std::unique_ptr<ParserHandler> createNew();

	virtual std::string triggerChars();

	virtual bool canHandle(Parser * lex);

	virtual std::tuple<std::unique_ptr<_ASTElement>, bool> handle(Parser * lex);

	virtual std::unique_ptr<_ASTElement> finish(Parser * lex);
};
