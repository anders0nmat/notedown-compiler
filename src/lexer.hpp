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

/*
	Wrapper class for creating an AST
*/
class Parser {
protected:
	NotedownCompiler * compiler;

	std::istream * input;

	std::unique_ptr<ParserHandler> _lastHandler = nullptr;
	std::unique_ptr<ASTDocument> document = nullptr;

	int _lastChar = 0;

	std::unique_ptr<ASTPlainText> _parsePlainText();
	std::unique_ptr<_ASTInlineElement> _parseLine(bool allowLb = true);
	TimeSnap createTimesnap();
	void revert(TimeSnap snap);
public:
	std::string lastString;
	int lastInt;
	Token lastToken;

	Parser(NotedownCompiler * compiler, std::istream * input) : compiler(compiler), input(input) {}
	~Parser() = default;

	/*
		Gets the token chr would give
	*/
	Token peektok(int chr);

	/*
		Gets the token the next char in input belongs to
	*/
	Token peektok();

	/*
		Gets the next char in input
	*/
	int peekchar();

	/*
		Consumes the current Token and loads the next one
	*/
	Token gettok();

	/*
		Consumes amount Tokens and loads the next one if amount >= lastInt
		If lastToken is not [tokSpace, tokSym] this is identical to gettok()
	*/
	Token gettok(int amount);

	/*
		Reads Text literally (no Sym or Space collapsing) until condition is met or EOF/EOL occured
		Doesnt consume ending token if condition caused end, does consume newline
		@param condition returns true if reading should end
		@return String read and whether it ended on condition (=true) or EOF/EOL (=false)
	*/
	std::tuple<std::string, bool> readUntil(std::function<bool(Parser *)> condition);

	/**
		Reads Text of scheme <url> <text> until delim is met or EOF/EOL occured.
		<text> allows quoted text where delim is allowed.
		Consumes ending token.
		@param delim Character to end operation on, normally closing brackets
		@return [ url, text, success] where success indicates if it ended on delim (=true) or EOL/EOF (=false)
	*/
	std::tuple<std::string, std::string, bool> parseLink(char delim);


	std::unique_ptr<ParserHandler> findNextHandler();
	std::unique_ptr<ParserHandler> findNextHandler(std::string name);

	/*
		Finds the Handler after h that can handle the current Situation
		@param h The handler that failed, if nullptr, this function is equivalent to findNextHandler()
		@result A Handler that can handle the situation or nullptr if none of them can 
	*/
	std::unique_ptr<ParserHandler> findHandlerAfter(std::unique_ptr<ParserHandler> & h);

	std::unique_ptr<InlineHandler> findNextInlineHandler();
	std::unique_ptr<InlineHandler> findNextInlineHandler(std::string name);
	
	/*
		Finds the Handler after h that can handle the current Situation
		@param h The handler that failed, if nullptr, this function is equivalent to findNextInlineHandler()
		@result A Handler that can handle the situation or nullptr if none of them can 
	*/
	std::unique_ptr<InlineHandler> findInlineHandlerAfter(std::unique_ptr<InlineHandler> & h);

	// void rollBack(std::unique_ptr<ParserHandler> & lastHandler, TimeSnap & snap);

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

	/*
		Has consumed newline if second return value is true.
		@returns unique_ptr : Element to insert or nullptr. bool : Whether the current Handler was finished.
	*/
	std::tuple<std::unique_ptr<_ASTElement>, bool> parseLine(std::unique_ptr<ParserHandler> & lastHandler);
	std::unique_ptr<_ASTElement> parseLine();

	void createDocument();
	void parseDocument();
	std::unique_ptr<ASTDocument> & getDocument();
};
