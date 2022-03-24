#include "lexer.hpp"

#include <iostream>

using std::string;
using std::unique_ptr;
using std::tuple;
using std::tie;
using std::make_tuple;
using std::move;
using std::make_unique;

Parser::Parser(string filename) {
	input = std::ifstream(filename, std::ifstream::in);

	if (!input.is_open())
		throw "File not found";
}

Token Parser::peektok(int chr) {
	if (input.eof()) return tokEOF;

	if (symbols.find_first_of(chr) != std::string::npos)
		return tokSym;

	if (inlineSymbols.find_first_of(chr) != std::string::npos)
		return tokInlineSym;

	if (isdigit(chr))
		return tokNumber;

	switch (chr) {
	case ' ':
		return tokSpace;
	case '\n':
		return tokNewline;
	default:
		return tokText;
	}
}

inline Token Parser::peektok() {
	return peektok(_lastChar);
}

inline int Parser::peekchar() {
	return input.peek();
}

Token Parser::gettok() {
	if (_lastChar == 0)
		_lastChar = input.get();

	lastToken = peektok();

	switch (lastToken) {
	case tokText:
		lastString = _lastChar;
		while (peektok(_lastChar = input.get()) == tokText)
			lastString += _lastChar;
		return lastToken;
	case tokNumber:
		lastString = _lastChar;
		while (isdigit(_lastChar = input.get()))
			lastString += _lastChar;
		if (_lastChar == '.') {
			lastString += _lastChar;
			_lastChar = input.get();
		}
		lastInt = std::stoi(lastString);
		return lastToken;
	case tokSpace:
	case tokInlineSym:
	case tokSym:
		lastInt = 1;
		lastString = _lastChar;
		while ((_lastChar = input.get()) == lastString.front()) 
			lastInt++;
		return lastToken;
	default:
		_lastChar = input.get(); // Consume current char
		return lastToken;
	}
}

void Parser::getchar() {
	_lastChar = input.get();
}

int Parser::currchar() {
	return _lastChar;
}

std::string Parser::escaped(int chr) {
	switch (chr) {
	case '\\':
		return "\\";
	case '"':
		return "\"";
	default:
		return "\\" + chr;
	}
}

void Parser::puttok() {
	switch (lastToken) {
		case tokNumber:
		case tokText:
			input.seekg(-lastString.length() - 1, std::ios_base::cur);
			break;
		case tokSpace:
		case tokInlineSym:
		case tokSym:
			input.seekg(-lastInt - 1, std::ios_base::cur);
			break;
		case tokNewline:
		case tokEOF:
		default:
			input.seekg(-2, std::ios_base::cur);
	}
}

std::tuple<std::string, bool> Parser::extractText(bool allowRange, std::string delimiter) {
	if (lastToken == tokNewline || lastToken == tokEOF)
		return make_tuple("", true);
	bool rangeStarted = false;
	
	puttok(); // We need to get every char, disable Token System
	getchar(); // Load first Char
	
	if (allowRange && _lastChar == '"') {
		rangeStarted = true;
		getchar(); // Consume "
	}

	std::string result;

	while (true) {
		if (_lastChar == '\n' || input.eof())
			return make_tuple("", true);
		if (!rangeStarted && delimiter.find_first_of(_lastChar) != std::string::npos) {
			// End of Read
			getchar(); // Consume delim
			return make_tuple(result, false);
		}
		if (rangeStarted && _lastChar == '"') {
			// Range end
			getchar(); // Consume closing "
			rangeStarted = false;
			break; // Consume until delimiter
		}
		if (_lastChar == '\\') {
			// Escape Sequence
			getchar(); // Consume \ 
			result += escaped(_lastChar);
			getchar(); // Consume escaped char
		}
		result += _lastChar;
	}

	while (delimiter.find_first_of(_lastChar) == std::string::npos &&
		_lastChar != '\n' && !input.eof()) {
		getchar();
	}

	// Next Char is delimiter or Newline or EOF
	if (_lastChar == '\n' && input.eof())
		return make_tuple("", true);
	return make_tuple(result, false);

	gettok(); // Reload Token System
}

unique_ptr<ParserHandler> Parser::findNextHandler() {
	for (auto & e : handlerList) {
		if (e->canHandle(this))
			return move(e->createNew());
	}
	return nullptr;
}

unique_ptr<ParserHandler> Parser::findNextHandler(string name) {
	auto p = handlerAlias.find(name);
	if (p != handlerAlias.end()) {
		return move(handlerList[p->second]->createNew());
	}
	return nullptr;
}

unique_ptr<InlineHandler> Parser::findNextInlineHandler() {
	for (auto & e : inlineHandlerList) {
		if (e->canHandle(this))
			return move(e->createNew());
	}
	return nullptr;
}

unique_ptr<InlineHandler> Parser::findNextInlineHandler(string name) {
	auto p = inlineHandlerAlias.find(name);
	if (p != inlineHandlerAlias.end()) {
		// auto it = p->second;
		return move(inlineHandlerList[p->second]->createNew());
	}
	return nullptr;
}

bool Parser::addToDocument(unique_ptr<_ASTElement> element) {
	bool res = element != nullptr; 
	if (element != nullptr)
		document->addElement(element);
	return res;
}

bool Parser::addHandler(string name, unique_ptr<ParserHandler> handler) {
	if (handlerAlias.count(name) == 1)
		return false;
	
	symbols += handler->triggerChars();
	handlerList.push_back(move(handler));
	return handlerAlias.emplace(name, handlerList.size() - 1).second;
}

bool Parser::addHandlerAlias(std::string alias, std::string name) {
	if (handlerAlias.count(alias) == 1 || handlerAlias.count(name) == 0)
		return false;
	
	return handlerAlias.emplace(alias, handlerAlias[name]).second;
}

bool Parser::addInlineHandler(string name, unique_ptr<InlineHandler> handler) {
	if (inlineHandlerAlias.count(name) == 1)
		return 0;
	
	inlineSymbols += handler->triggerChars();
	inlineHandlerList.push_back(move(handler));
	return inlineHandlerAlias.emplace(name, inlineHandlerList.size() - 1).second;
}

bool Parser::addInlineHandlerAlias(std::string alias, std::string name) {
	if (inlineHandlerAlias.count(alias) == 1 || inlineHandlerAlias.count(name) == 0)
		return false;
	
	return inlineHandlerAlias.emplace(alias, inlineHandlerAlias[name]).second;
}

std::tuple<unique_ptr<_ASTElement>, bool> Parser::parseLine(unique_ptr<ParserHandler> & lastHandler) {
	if (lastToken == tokEOF) {
		if (lastHandler != nullptr) {
			unique_ptr<_ASTElement> e = move(lastHandler->finish(this));
			lastHandler = nullptr;
			return make_tuple(move(e), true);
		}
		return make_tuple(nullptr, false);
	}

	if (lastHandler == nullptr || !lastHandler->canHandle(this)) {
		if (lastHandler != nullptr) {
			// lastHandler was unexpectedly ended. Give him a chance to finish up
			unique_ptr<_ASTElement> e = move(lastHandler->finish(this));
			lastHandler = nullptr;
			return make_tuple(move(e), true);
		}
		// Skip everything until <Newline> <!Space>
		// while (lastToken == tokNewline) {
		// 	gettok(); // Consume Newline
		// 	if (lastToken == tokSpace && peektok() == tokNewline)
		// 		gettok(); // Consume empty lines
		// }

		lastHandler = move(findNextHandler());

		// No handler for current situation (e.g. end of file or invalid char combination)
		if (lastHandler == nullptr) {
			if (lastToken == tokEOF)
				return make_tuple(nullptr, false);
			// default handler
			lastHandler = findNextHandler("H_default");
		}
	}

	bool finished;
	unique_ptr<_ASTElement> element;
	tie(element, finished) = lastHandler->handle(this);

	if (finished)
		lastHandler = nullptr;

	return make_tuple(move(element), false);
}

inline unique_ptr<_ASTElement> Parser::parseLine() {
	return move(std::get<0>(parseLine(_lastHandler)));
}

void Parser::createDocument() {
	document = make_unique<ASTDocument>();
}

void Parser::parseDocument() {
	createDocument();

	gettok(); // Loads Start of File 

	// Parse until error or end of file	
	while (lastToken != tokEOF) {
		addToDocument(std::move(parseLine()));
	}

	// To finish any block elements that unexpectedly got ended on EOF
	addToDocument(std::move(parseLine()));
}

unique_ptr<ASTDocument> & Parser::getDocument() {
	return document;
}

tuple<unique_ptr<ASTInlineText>, bool> Parser::parseText(
	bool allowLb, bool unknownAsText, bool allowInlineStyling, int inlineSymReturn, int symReturn) {
	unique_ptr<ASTInlineText> text = make_unique<ASTInlineText>();

	while (true) {
		unique_ptr<_ASTInlineElement> e = _parseLine(allowLb);
		if (e == nullptr) {
			// Forced Linebreak, EOF, EOL or Sym
			if (lastToken == tokSpace && lastInt >= 2 && peektok() == tokNewline) {
				// only the case if forced linebreak happened
				gettok(); // Consume Spaces, either way
				if (allowLb) { 
					// Forced Linebreak (<Space> <Space> <Linebreak>)
					if (text->size() == 0)
						return make_tuple(nullptr, true);
					text->addElement(make_unique<ASTLinebreak>());
				}
				continue;
			}

			if (lastToken == tokNewline || lastToken == tokEOF) {
				if (text->size() == 0)
					return make_tuple(nullptr, true);
				return make_tuple(move(text), true);
			}
			
			if (lastToken == tokSym) {
				// Not an inline element, print literally?
				if (lastString.front() == symReturn) {
					return make_tuple(move(text), false);
				}

				if (unknownAsText) {
					text->addElement(make_unique<ASTPlainText>(lastInt, lastString.front()));
					gettok(); // Consume sym
				}
				else {
					return make_tuple(move(text), false);
				}
				continue;
			}

			if (!allowInlineStyling) {
				// Inline Sym but not allowed
				if (unknownAsText) {
					text->addElement(make_unique<ASTPlainText>(lastInt, lastString.front()));
				}
				gettok(); // Consume Sym

				continue;
			}

			// So it is a inline symbol
			if (lastString.front() == inlineSymReturn) {
				if (text->size() == 0)
					return make_tuple(nullptr, false);
				return make_tuple(move(text), false);
			}

			unique_ptr<InlineHandler> handler = findNextInlineHandler();
			if (handler == nullptr) {
				// No appropriate handler, print it or return
				if (!unknownAsText) 
					return make_tuple(move(text), false);
				e = make_unique<ASTPlainText>(lastInt, lastString.front());
				gettok(); // Consume sym
			}
			else {
				// Valid handler found
				tie(e, std::ignore) = handler->handle(this);
			}
		}
		text->addElement(move(e));
	}
}

unique_ptr<_ASTInlineElement> Parser::_parseLine(bool allowLb) {
	switch (lastToken) {
		case tokText:
			return move(_parsePlainText());
		case tokSpace:
			if (allowLb && lastToken == tokSpace && lastInt >= 2 && peektok() == tokNewline) { 
				// Forced Linebreak (<Space> <Space> <Linebreak>)
				//gettok(); // Consume Spaces
				//return std::make_unique<ASTLinebreak>();
				return nullptr; // Caller handles that case now
			}
			else if (peektok() == tokText) {
				// Space and then text (e.g. after inline styling)
				return move(_parsePlainText());
			}
			else if (peektok() == tokNewline) {
				gettok(); // Consume space
				return nullptr;
			}
			break;
		case tokSym:
		case tokInlineSym:
		case tokNewline:
		default:
			return nullptr;
	}
	return nullptr;
}

unique_ptr<ASTPlainText> Parser::_parsePlainText() {
	string str = lastString;

	gettok(); // Consume Text

	while (lastToken == tokText || lastToken == tokNumber || 
		(lastToken == tokSpace && (lastInt < 2 || peektok() != tokNewline))) {
		str += (lastToken == tokText || lastToken == tokNumber) ? lastString : string(" ");
		gettok(); // Consume inserted Text
	}

	return make_unique<ASTPlainText>(str);
}

// ----- InlineHandler ----- \\ 

std::unique_ptr<InlineHandler> InlineHandler::createNew() {
	return std::make_unique<InlineHandler>();
}

std::string InlineHandler::triggerChars() {
	return "";
}

bool InlineHandler::canHandle(Parser * lex) {
	return false;
}

std::tuple<std::unique_ptr<_ASTInlineElement>, bool> InlineHandler::handle(Parser * lex) {
	return std::make_tuple(nullptr, true);
}

std::unique_ptr<_ASTElement> InlineHandler::finish(Parser * lex) {
	return nullptr;
}

// ----- ParserHandler ----- \\ 

std::unique_ptr<ParserHandler> ParserHandler::createNew() {
	return std::make_unique<ParserHandler>();
}

std::string ParserHandler::triggerChars() {
	return "";
}

bool ParserHandler::canHandle(Parser * lex) {
	return false;
}

std::tuple<std::unique_ptr<_ASTElement>, bool> ParserHandler::handle(Parser * lex) {
	return std::make_tuple(nullptr, true);
}

std::unique_ptr<_ASTElement> ParserHandler::finish(Parser * lex) {
	return nullptr;
}
