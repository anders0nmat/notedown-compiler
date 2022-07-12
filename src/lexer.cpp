#include "lexer.hpp"

#include <iostream>
#include <algorithm>
#include <typeinfo>

using std::string;
using std::unique_ptr;
using std::tuple;
using std::tie;
using std::make_tuple;
using std::move;
using std::make_unique;
using namespace Notedown::Handler;

Token Parser::peektok(int chr) {
	if (input->eof()) return tokEOF;

	if (compiler->symbols.count(chr) == 1)
		return tokSym;

	if (isdigit(chr))
		return tokNumber;

	switch (chr) {
	case ' ':
	case '\t':
		return tokSpace;
	case '\n':
	case '\r':
		return tokNewline;
	case '\\':
		// Escape Char
		return _tokEscape;
	default:
		return tokText;
	}
}

inline Token Parser::peektok() {
	return peektok(_lastChar);
}

Token Parser::gettok() {
	if (_lastChar == 0)
		_lastChar = input->get();

	lastToken = peektok();

	switch (lastToken) {
	case _tokEscape:
		// Escape Character
		// Valid Escape Character?
		if (compiler->symbols.count(input->peek()) == 1) {
			_lastChar = input->get(); // Ignore '\', proceed as Text
		}
		else {
			switch (input->peek()) {
				case 'n':
					_lastChar = input->get(); // now contains n
					_lastChar = '\n';
					break;
				case '.':
				case '\\': // Consume and treat as normal text
					_lastChar = input->get();
				default: break;
			}
		}
		lastToken = tokText;
		// Falls through to treat as normal text
	case tokText:
		lastString = _lastChar;
		while (peektok(_lastChar = input->get()) == tokText)
			lastString += _lastChar;
		return lastToken;
	case tokNumber:
		lastString = _lastChar;
		while (isdigit(_lastChar = input->get()))
			lastString += _lastChar;
		if (_lastChar == '.') {
			lastString += _lastChar;
			_lastChar = input->get();
		}
		lastInt = std::stoi(lastString);
		return lastToken;
	case tokSpace:
	case tokSym:
		lastInt = 1;
		lastString = _lastChar;
		while ((_lastChar = input->get()) == lastString.front()) 
			lastInt++;

		// lastInt = 0;
		// lastString = _lastChar;
		// while (_lastChar == lastString.front()) {
		// 	_lastChar = input->get();
		// 	lastInt++;
		// }
		return lastToken;
	case tokNewline:
		if (_lastChar == '\r')
			_lastChar = input->get();
		if (_lastChar == '\n')
			_lastChar = input->get();
		return lastToken;
	default:
		_lastChar = input->get(); // Consume current char
		return lastToken;
	}
}

Token Parser::gettok(int amount) {
	if (lastToken == tokSym || lastToken == tokSpace) {
		if (lastInt > amount)
			lastInt -= amount;
		else
			gettok();
		return lastToken;
	}

	return gettok(); // always consume on text
}

int Parser::peekchar() {
	return _lastChar;
}

bool Parser::isLast(Token tok, char sym, int amount) {
	return (lastToken == tok) && (lastString[0] == sym) && (amount > -1 ? lastInt == amount : true);
}

bool Parser::isLast(Token tok, std::string str, int amount) {
	return (lastToken == tok) && (lastString == str) && (amount > -1 ? lastInt == amount : true);
}

std::tuple<std::string, bool> Parser::readUntil(std::function<bool(Parser *)> condition, bool escape) {
	if (!condition)
		return make_tuple("", true);
	std::string res, peek;

	while (
		(lastToken != tokNewline) &&
		(lastToken != tokEOF) &&
		(!condition(this))
		) {
		// Take text literally
		if (lastToken == tokText || lastToken == tokNumber)
			res += peek + lastString;
		else
			res += peek + std::string(lastInt, lastString[0]);
		peek = "";
		if (!escape && peektok() == _tokEscape) {
			peek = "\\";
		}
		gettok(); // Consume inserted Text
	}

	if (lastToken == tokNewline || lastToken == tokEOF) {
		if (lastToken == tokNewline)
			gettok(); // Consume Newline
		return make_tuple(res, false);
	}
	return make_tuple(res, true);
}

unique_ptr<ParserHandler> Parser::findNextHandler() {
	for (auto & e : compiler->handlerList) {
		if (e->canHandle(this))
			return move(e->createNew());
	}
	return nullptr;
}

unique_ptr<ParserHandler> Parser::findNextHandler(string name) {
	auto p = compiler->handlerAlias.find(name);
	if (p != compiler->handlerAlias.end()) {
		return move(compiler->handlerList[p->second]->createNew());
	}
	return nullptr;
}

std::unique_ptr<ParserHandler> Parser::findHandlerAfter(std::unique_ptr<ParserHandler> & h) {
	if (h == nullptr || h->id < 0 || h->id >= compiler->handlerList.size())
		return findNextHandler();	

	for (auto i = h->id + 1; i < compiler->handlerList.size(); i++) {
		if (compiler->handlerList[i]->canHandle(this))
			return move(compiler->handlerList[i]->createNew());
	}
	return nullptr;
}

unique_ptr<InlineHandler> Parser::findNextInlineHandler() {
	for (auto & e : compiler->inlineHandlerList) {
		if (e->canHandle(this))
			return move(e->createNew());
	}
	return nullptr;
}

unique_ptr<InlineHandler> Parser::findNextInlineHandler(string name) {
	auto p = compiler->inlineHandlerAlias.find(name);
	if (p != compiler->inlineHandlerAlias.end()) {
		// auto it = p->second;
		return move(compiler->inlineHandlerList[p->second]->createNew());
	}
	return nullptr;
}

std::unique_ptr<InlineHandler> Parser::findInlineHandlerAfter(std::unique_ptr<InlineHandler> & h) {
	if (h == nullptr || h->id < 0 || h->id >= compiler->inlineHandlerList.size())
		return findNextInlineHandler();	

	for (auto i = h->id + 1; i < compiler->inlineHandlerList.size(); i++) {
		if (compiler->inlineHandlerList[i]->canHandle(this))
			return move(compiler->inlineHandlerList[i]->createNew());
	}
	return nullptr;
}

std::tuple<std::string, std::string, bool> Parser::parseLink(char delim) {
	bool success, inQuote = false;
	std::string keyword, command;

	std::tie(keyword, success) = readUntil([delim](Parser * lex) {
		return (lex->lastToken == tokSpace) || 
			(lex->lastToken == tokSym && lex->lastString[0] == delim);
	});
	if (!success)
		return std::make_tuple(keyword, "", false);
	if (lastToken == tokSpace)
		gettok(); // Consume Space
	std::tie(command, success) = readUntil([&inQuote, delim](Parser * lex) {
		if (lex->lastToken == tokSym && lex->lastString[0] == '"') {
			inQuote = !inQuote;
		}
		return (!inQuote && lex->lastToken == tokSym && lex->lastString[0] == delim);
	});
	if (!success)
		return std::make_tuple(keyword, command, false);
	gettok(1);
	// Valid
	return std::make_tuple(keyword, command, true);
}

bool Parser::addToDocument(unique_ptr<_ASTElement> element) {
	bool res = element != nullptr; 
	if (element != nullptr)
		document->addElement(element);
	return res;
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

	if (lastHandler != nullptr && !lastHandler->canHandle(this)) {
		// lastHandler was unexpectedly ended. Give him a chance to finish up
		unique_ptr<_ASTElement> e = move(lastHandler->finish(this));
		lastHandler = nullptr;
		return make_tuple(move(e), true);
	}

	bool finished = false, err = false;
	unique_ptr<_ASTElement> element;
	do {
		if (err || lastHandler == nullptr) {
			lastHandler = move(findHandlerAfter(lastHandler));
			if (lastHandler != nullptr)
				lastHandler->snap = createTimesnap();
		}
		if (lastHandler == nullptr) {
			if (lastToken == tokEOF)
				return make_tuple(nullptr, false);
			lastHandler = move(findNextHandler("H_default"));
			lastHandler->snap = createTimesnap();
		}

		tie(element, finished, err) = lastHandler->handle(this);
		if (err) revert(lastHandler->snap);
	} while (err);

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
	bool allowLb, bool unknownAsText, bool allowInlineStyling, int symReturn) {
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
			
			// So it is a tokSym

			if (lastString[0] == symReturn)
				return make_tuple(move(text), false);

			if (allowInlineStyling) {
				bool err = false;
				unique_ptr<InlineHandler> handler = nullptr;
				do {
					handler = move(findInlineHandlerAfter(handler));
					if (handler == nullptr) {
						// No appropriate handler, print it or return
						if (!unknownAsText) 
							return make_tuple(move(text), false);
						e = make_unique<ASTPlainText>(lastInt, lastString.front());
						gettok(); // Consume sym
					}
					else {
						// Valid handler found
						handler->snap = createTimesnap();
						tie(e, std::ignore, err) = handler->handle(this);
						if (err) revert(handler->snap);
					}
				} while (err);
			}
			else {
				if (unknownAsText) {
					text->addElement(make_unique<ASTPlainText>(lastInt, lastString.front()));
					gettok(); // Consume sym
				}
				else {
					return make_tuple(move(text), false);
				}
			}
		}
		text->addElement(move(e));
	}
}

unique_ptr<_ASTInlineElement> Parser::_parseLine(bool allowLb) {
	switch (lastToken) {
		case tokNumber:
		case tokText:
			return move(_parsePlainText());
		case tokSpace:
			if (allowLb && lastToken == tokSpace && lastInt >= 2 && peektok() == tokNewline) { 
				// Forced Linebreak (<Space> <Space> <Linebreak>)
				return nullptr;
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

TimeSnap Parser::createTimesnap() {
	// std::cout << "Saved Pos is: " << input->tellg() << std::endl;
	TimeSnap p{
		input->tellg(),
		_lastChar,
		lastString,
		lastInt,
		lastToken
	};
	return p;
}

void Parser::revert(TimeSnap snap) {
	input->seekg(snap.pos, std::ios::beg);
	_lastChar = snap._lastChar;
	lastString = snap.lastString;
	lastInt = snap.lastInt;
	lastToken = snap.lastToken;
}


// ----- InlineHandler ----- //

void InlineHandler::inheritId(std::unique_ptr<InlineHandler> & other) {
	other->id = this->id;
}

std::unique_ptr<InlineHandler> InlineHandler::createNew() {
	return std::make_unique<InlineHandler>();
}

std::string InlineHandler::triggerChars() {
	return "";
}

bool InlineHandler::canHandle(Parser * lex) {
	return false;
}

InlineHandlerReturn InlineHandler::handle(Parser * lex) {
	return make_result(nullptr, true);
}

std::unique_ptr<_ASTElement> InlineHandler::finish(Parser * lex) {
	return nullptr;
}

// ----- ParserHandler ----- //

void ParserHandler::inheritId(std::unique_ptr<ParserHandler> & other) {
	other->id = this->id;
}

std::unique_ptr<ParserHandler> ParserHandler::createNew() {
	return newSelf<ParserHandler>();
}

std::string ParserHandler::triggerChars() {
	return "";
}

bool ParserHandler::canHandle(Parser * lex) {
	return false;
}

HandlerReturn ParserHandler::handle(Parser * lex) {
	return make_result(nullptr, true);
}

std::unique_ptr<_ASTElement> ParserHandler::finish(Parser * lex) {
	return nullptr;
}
