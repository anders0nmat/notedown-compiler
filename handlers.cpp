#include "inline_handler.hpp"
#include "parser_handler.hpp"

void Parser::addDefaultHandlers() {
	
}

// ------------------------------------ \\ 
// ---------- Block Handlers ---------- \\ 
// ------------------------------------ \\ 

#pragma region ParagraphHandler
// ----- ParagraphHandler ----- \\ 

std::unique_ptr<ParserHandler> ParagraphHandler::createNew() {
	return std::make_unique<ParagraphHandler>();
}

bool ParagraphHandler::canHandle(Parser * lex) {
	return 
		(lex->lastToken == tokSpace) || 
		(lex->lastToken == tokText);
}

std::tuple<std::unique_ptr<_ASTElement>, bool> ParagraphHandler::handle(Parser * lex) {
	if (content == nullptr) {
		// Remove Spaces in front
		while (lex->lastToken == tokSpace) 
			lex->gettok(); // Eat Space
		content = std::make_unique<ASTParagraph>();
	}

	std::unique_ptr<ASTInlineText> e;
	std::tie(e, std::ignore) = lex->parseText(true, true);
	lex->gettok(); // Consume Newline (parseText always ends on newline)

	if (e == nullptr) {
		// An empty line occured
		if (content->size() == 0)
			return std::make_tuple(nullptr, true);
		return std::make_tuple(std::move(content), true);
	}
	content->addElement(std::move(e));

	// Dont finish, dont return anything
	return std::make_tuple(nullptr, false);
}

std::unique_ptr<_ASTElement> ParagraphHandler::finish(Parser * lex) {
	return std::move(content);
}

#pragma endregion ParagraphHandler

#pragma region HeadingHandler
// ----- HeadingHandler ----- \\ 

std::unique_ptr<ParserHandler> HeadingHandler::createNew() {
	return std::make_unique<HeadingHandler>();
}

std::string HeadingHandler::triggerChars() {
	return "#";
}

bool HeadingHandler::canHandle(Parser * lex) {
	return (lex->lastToken == tokSym) && 
		(lex->lastString[0] == '#') &&
		(lex->lastInt <= 6) &&
		(lex->peektok() == tokSpace || lex->peektok() == tokNewline);
}

std::tuple<std::unique_ptr<_ASTElement>, bool> HeadingHandler::handle(Parser * lex) {
	int level = lex->lastInt;
	lex->gettok(); // Consume '#'
	if (lex->lastToken == tokSpace) {
		// Space after #
		lex->gettok(); // Consume spaces
	}

	// No Text in Heading
	if (lex->lastToken == tokNewline) {
		// Empty Heading
		lex->gettok(); // Consume newline
		return std::make_tuple(std::make_unique<ASTHeading>(level, nullptr), true);
	}

	std::unique_ptr<ASTInlineText> t;
	std::tie(t, std::ignore) = lex->parseText(false);
	lex->gettok(); // Consume newline

	return std::make_tuple(std::make_unique<ASTHeading>(level, std::move(t)), true);
}

std::unique_ptr<_ASTElement> HeadingHandler::finish(Parser * lex) {
	return nullptr;
}

#pragma endregion HeadingHandler

#pragma region HLineHandler
// ----- HLineHandler ----- \\ 

std::unique_ptr<ParserHandler> HLineHandler::createNew() {
	return std::make_unique<HLineHandler>();
}

std::string HLineHandler::triggerChars() {
	return "-";
}

bool HLineHandler::canHandle(Parser * lex) {
	return (lex->lastToken == tokSym) && 
		(lex->lastString[0] == '-') &&
		(lex->lastInt >= 3) &&
		(lex->peektok() == tokNewline);
}

std::tuple<std::unique_ptr<_ASTElement>, bool> HLineHandler::handle(Parser * lex) {
	lex->gettok(); // Consume ---
	lex->gettok(); // Consume Newline
	return std::make_tuple(std::make_unique<ASTHLine>(), true);
}

std::unique_ptr<_ASTElement> HLineHandler::finish(Parser * lex) {
	return nullptr;
}

#pragma endregion HLineHandler

#pragma region BlockquoteHandler
// ----- BlockquoteHandler ----- \\ 

std::unique_ptr<ParserHandler> BlockquoteHandler::createNew() {
	return std::make_unique<BlockquoteHandler>();
}

std::string BlockquoteHandler::triggerChars() {
	return ">";
}

bool BlockquoteHandler::canHandle(Parser * lex) {
	if (content != nullptr)
		return (canHandleBlock(lex) && (indentStyle == ' ' || indentStyle == 0)) ||
			(indentStyle == '>' || indentStyle == 0) &&
			((lex->lastToken == tokSym) &&
			(lex->lastString[0] == '>') && 
			(lex->lastInt == (centered ? 2 : 1)) && 
			(lex->peektok() == tokSpace || lex->peektok() == tokNewline));

	return (lex->lastToken == tokSym) &&
		(lex->lastString[0] == '>') && 
		(lex->lastInt <= 2) && 
		(lex->peektok() == tokSpace || lex->peektok() == tokNewline);
}

std::tuple<std::unique_ptr<_ASTElement>, bool> BlockquoteHandler::handle(Parser * lex) {
	if (content != nullptr && indentStyle == 0)
		indentStyle = lex->lastString[0];
	
	if (content == nullptr) {
		content = std::make_unique<ASTBlockquote>(lex->lastInt > 1);
		centered = lex->lastInt > 1;
	}

	if (lex->lastToken == tokSpace) {
		// Indent Block
		handleBlock(lex);
	}
	else {
		// '>' Block
		lex->gettok(); // Consume >
		if (lex->lastToken == tokSpace)	
			lex->gettok(); // Consume space

		// Else dont consume newline, parseLine will take care of that
		std::unique_ptr<_ASTElement> e;
		bool redo;
		do {
			std::tie(e, redo) = lex->parseLine(handler);
			content->addElement(e);
		} while (redo);
	}

	return std::make_tuple(nullptr, false);
}

std::unique_ptr<_ASTElement> BlockquoteHandler::finish(Parser * lex) {
	// if (handler != nullptr) {
	// 	std::unique_ptr<_ASTElement> e = std::move(handler->finish(lex));
	// 	content->addElement(e);
	// }
	finishBlock(lex);
	return std::move(content);
}

#pragma endregion BlockquoteHandler

#pragma region UnorderedListHandler
// ----- UnorderedListHandler ----- \\ 

std::unique_ptr<ParserHandler> UnorderedListHandler::createNew() {
	return std::make_unique<UnorderedListHandler>();
}

std::string UnorderedListHandler::triggerChars() {
	return "-";
}

bool UnorderedListHandler::canHandle(Parser * lex) {
	return canHandleBlock(lex) ||
		((lex->lastToken == tokSym) &&
		(lex->lastString[0] == '-') && 
		(lex->lastInt == 1) && 
		(lex->peektok() == tokSpace || lex->peektok() == tokNewline));
}

std::tuple<std::unique_ptr<_ASTElement>, bool> UnorderedListHandler::handle(Parser * lex) {
	if (list == nullptr) {
		// Create List
		list = std::make_unique<ASTUnorderedList>();
	}

	if (lex->lastToken == tokSpace) {
		// Indent Block
		handleBlock(lex);
	}
	else {
		// '-' Block
		if (handler != nullptr)
			content->addElement(handler->finish(lex));
		if (content != nullptr)
			list->addElement(content);

		// Either way, new Element started
		content = std::make_unique<ASTListElement>();

		lex->gettok(); // Consume -
		if (lex->lastToken == tokSpace)	
			lex->gettok(); // Consume space

		// Else dont consume newline, parseLine will take care of that
		std::unique_ptr<_ASTElement> e;
		bool redo;
		do {
			std::tie(e, redo) = lex->parseLine(handler);
			content->addElement(e);
		} while (redo);
	}

	return std::make_tuple(nullptr, false);
}

std::unique_ptr<_ASTElement> UnorderedListHandler::finish(Parser * lex) {
	finishBlock(lex);
	if (list != nullptr)
		list->addElement(content);
	return std::move(list);
}

#pragma endregion UnorderedListHandler

#pragma region OrderedListHandler
// ----- OrderedListHandler ----- \\ 

std::unique_ptr<ParserHandler> OrderedListHandler::createNew() {
	return std::make_unique<OrderedListHandler>();
}

std::string OrderedListHandler::triggerChars() {
	return "";
}

bool OrderedListHandler::canHandle(Parser * lex) {
	return canHandleBlock(lex) ||
		((lex->lastToken == tokNumber) &&
		(lex->lastString[lex->lastString.length()] == '.') &&
		(lex->peektok() == tokSpace || lex->peektok() == tokNewline));
}

std::tuple<std::unique_ptr<_ASTElement>, bool> OrderedListHandler::handle(Parser * lex) {
	if (list == nullptr) {
		// Create List
		list = std::make_unique<ASTOrderedList>();
	}

	if (lex->lastToken == tokSpace) {
		// Indent Block
		handleBlock(lex);
	}
	else {
		// '<Num>.' Block
		if (handler != nullptr)
			content->addElement(handler->finish(lex));
		if (content != nullptr)
			list->addElement(content);

		// Either way, new Element started
		content = std::make_unique<ASTListElement>(lex->lastInt);

		lex->gettok(); // Consume <Num>.
		if (lex->lastToken == tokSpace)	
			lex->gettok(); // Consume space

		// Else dont consume newline, parseLine will take care of that
		std::unique_ptr<_ASTElement> e;
		bool redo;
		do {
			std::tie(e, redo) = lex->parseLine(handler);
			content->addElement(e);
		} while (redo);
	}

	return std::make_tuple(nullptr, false);
}

std::unique_ptr<_ASTElement> OrderedListHandler::finish(Parser * lex) {
	finishBlock(lex);
	if (list != nullptr)
		list->addElement(content);
	return std::move(list);
}

#pragma endregion OrderedListHandler

#pragma region CodeHandler
// ----- CodeHandler ----- \\ 

std::unique_ptr<ParserHandler> CodeHandler::createNew() {
	return std::make_unique<CodeHandler>();
}

std::string CodeHandler::triggerChars() {
	return "`";
}

bool CodeHandler::canHandle(Parser * lex) {
	if (fenceCount != 0)
		return true;
	return 
		(lex->lastToken == tokSym) && 
		(lex->lastString[0] == '`') &&
		(lex->lastInt >= 3);
}

std::tuple<std::unique_ptr<_ASTElement>, bool> CodeHandler::handle(Parser * lex) {
	if (fenceCount == 0) {
		// Remove Spaces in front
		while (lex->lastToken == tokSpace) 
			lex->gettok(); // Eat Space
		fenceCount = lex->lastInt;
		lex->gettok(); // Eat ```
		lang = (lex->lastToken == tokText) ? lex->lastString : "";
		if (lang != "")
			lex->gettok(); // Eat Language Name
		while (lex->lastToken == tokSpace)
			lex->gettok(); // Consume Space
		std::tie(firstLine, std::ignore) = lex->parseText(false);
		lex->gettok(); // Eat Newline
		return std::make_tuple(nullptr, false);
	}

	// std::unique_ptr<ASTInlineText> line, e;
	// std::unique_ptr<_ASTInlineElement> e;
	// bool eol;
	std::string currLine;

	// Own read function so multiple spaces are represented correctly
	// while (
	// 	(lex->lastToken != tokNewline) &&
	// 	(lex->lastToken != tokEOF) &&
	// 	(lex->lastToken != tokSym || lex->lastString[0] != '`' || lex->lastInt != fenceCount || (lex->peektok() != tokNewline && lex->peektok() != tokEOF))
	// 	) {
	// 	// Take text literally
	// 	if (lex->lastToken == tokText || lex->lastToken == tokNumber)
	// 		currLine += lex->lastString;
	// 	else
	// 		currLine += std::string(lex->lastInt, lex->lastString[0]);
	// 	lex->gettok(); // Consume inserted Text
	// }
	bool eol;
	int count = fenceCount;
	std::tie(currLine, eol) = lex->readUntil([count](Parser * lex) {
		return (lex->lastToken == tokSym && lex->lastString[0] == '`' && lex->lastInt == count && (lex->peektok() == tokNewline || lex->peektok() == tokEOF));
	});

	// One only escapes if it is newline or end of block
	
	content.push_back(std::make_unique<ASTPlainText>(currLine));
	if (!eol) {
		// Finishing symbol
		lex->gettok(); // Consume ```
		lex->gettok(); // Consume newline
		std::unique_ptr<ASTCodeBlock> code = std::make_unique<ASTCodeBlock>(lang);
		for (auto & e : content)
			code->addElement(std::move(e));
		return std::make_tuple(std::move(code), true);
	}

	// do {


		// std::tie(e, eol) = lex->parseText(false, true, false, '`');
		
		// if (!eol) {
		// 	// Possibly ended
		// 	if (lex->lastInt == fenceCount && (lex->peektok() == tokNewline || lex->peektok() == tokEOF)) {
		// 		// Block ended
		// 		lex->gettok(); // Consume ```
		// 		lex->gettok(); // Consume Newline
		// 		content.push_back(std::move(e));
		// 		std::unique_ptr<ASTCodeBlock> code = std::make_unique<ASTCodeBlock>(lang);
		// 		for (auto & e : content)
		// 			code->addElement(std::move(e));
		// 		return std::make_tuple(std::move(code), true);
		// 	}
		// 	else {
		// 		// Treat as regular text
		// 		if (line == nullptr)
		// 			line = std::move(e);
		// 		else
		// 			line->addElement(std::move(e));
				
		// 		line->addElement(std::make_unique<ASTPlainText>(lex->lastInt, '`'));
		// 		lex->gettok(); // Consume `
		// 	}
		// }
		// else {
		// 	lex->gettok(); // Consume Newline
		// 	if (e == nullptr)
		// 		e = std::make_unique<ASTInlineText>(); // Empty Lines are taken seriously
		// 	if (line == nullptr)
		// 		line = std::move(e);
		// 	else
		// 		line->addElement(std::move(e));
		// }
	// } while (!eol);

	// content.push_back(std::move(line));
	// Dont finish, dont return anything
	return std::make_tuple(nullptr, false);
}

std::unique_ptr<_ASTElement> CodeHandler::finish(Parser * lex) {
	std::unique_ptr<ASTParagraph> p = std::make_unique<ASTParagraph>();
	for (auto & e : content)
		p->addElement(std::move(e));
	return std::move(p);
}

#pragma endregion


// ------------------------------------- \\ 
// ---------- Inline Handlers ---------- \\ 
// ------------------------------------- \\ 

#pragma region InlineCodeHandler
// ----- InlineCodeHandler ----- \\ 

std::unique_ptr<InlineHandler> InlineCodeHandler::createNew() {
	return std::make_unique<InlineCodeHandler>();
}

std::string InlineCodeHandler::triggerChars() {
	return "`";
}

bool InlineCodeHandler::canHandle(Parser * lex) {
	return (lex->lastToken == tokSym) &&
		(lex->lastString[0] == '`') &&
		(lex->peektok() != tokSpace) && (lex->peektok() != tokNewline);
}

std::tuple<std::unique_ptr<_ASTInlineElement>, bool> InlineCodeHandler::handle(Parser * lex) {
	
	// If there are no indicators left open, e.g. **** -> *<firstContent>**<secondContent>*
	if (lex->lastInt % 2 == 0)
		return std::make_tuple(nullptr, true);

	lex->gettok(); // Consume opening indicator
	std::unique_ptr<ASTInlineText> content;
	bool endOfLine;
	std::tie(content, endOfLine) = lex->parseText(false, true, false, '`');
	
	if (!endOfLine) {
		// Ended on indicator
		lex->gettok(); // Consume closing indicator
		if (content != nullptr)
			return std::make_tuple(std::make_unique<ASTTextModification>('`', std::move(content)), true);
		return std::make_tuple(nullptr, true);
	}
	if (content != nullptr)
		content->prependElement(std::make_unique<ASTPlainText>('`'));

	return std::make_tuple(std::move(content), true);
}

#pragma endregion

#pragma region InlineModifierHandler
// ----- InlineModifierHandler ----- \\ 

std::unique_ptr<InlineHandler> InlineModifierHandler::createNew() {
	return std::make_unique<InlineModifierHandler>();
}

std::string InlineModifierHandler::triggerChars() {
	return "[](){}<>\"!^#%";
}

bool InlineModifierHandler::canHandle(Parser * lex) {
	return (lex->lastToken == tokSym) &&
		(lex->lastString[0] == '[') &&
		(lex->peektok() != tokSpace) && (lex->peektok() != tokNewline);
}

std::tuple<std::unique_ptr<_ASTInlineElement>, bool> InlineModifierHandler::handle(Parser * lex) {
	
	if (lex->lastInt == 1)
		lex->gettok(); // Consume opening indicator
	else
		lex->lastInt--;
	std::unique_ptr<ASTInlineText> content;
	bool endOfLine;
	std::tie(content, endOfLine) = lex->parseText(false, true, true, ']');
	
	if (!endOfLine) {
		// Ended on indicator
		if (lex->lastInt != 1) {
			// Invalid, multiple closing chars
			content->prependElement(std::make_unique<ASTPlainText>('['));
			return std::make_tuple(std::move(content), true);
		}
		lex->gettok(); // Consume closing indicator

		std::string command;
		std::string url;
		int type = 0;
		bool eol;
		bool inQuote = false;
		bool success = false;
		bool succ;

		if (lex->lastToken != tokSym || lex->lastInt != 1) {
			// Can not possibly be valid
			content->prependElement(std::make_unique<ASTPlainText>('['));
			content->addElement(std::make_unique<ASTPlainText>(']'));
			return std::make_tuple(std::move(content), true);
		}

		switch (lex->lastString[0]) {
			case '(':
				type = '(';
				lex->gettok(); // Consume (
				std::tie(url, eol) = lex->readUntil([](Parser * lex) {
					return (lex->lastToken == tokSpace) || (lex->lastToken == tokSym && lex->lastString[0] == ')');
				});
				if (eol) {
					// Invalid
					break;
				}
				if (lex->lastToken == tokSpace) {
					lex->gettok(); // Consume Space
				}
				std::tie(command, eol) = lex->readUntil([&inQuote](Parser * lex) {
					if (lex->lastToken == tokSym && lex->lastString[0] == '"') {
						inQuote = !inQuote;
					}
					return (!inQuote && lex->lastToken == tokSym && lex->lastString[0] == ')');
				});
				if (eol) {
					// Invalid
					break;
				}
				if (lex->lastInt > 1)
					lex->lastInt--;
				else
					lex->gettok();
				// Valid, URL, Command and Type populated
				success = true;
				break;
			case '!':
				type = '!';
				lex->gettok(); // Consume !
				if (lex->lastToken != tokSym || lex->lastString[0] != '(' || lex->lastInt != 1) {
					// Invalid
					break;
				}
				lex->gettok(); // Consume (
				std::tie(url, eol) = lex->readUntil([](Parser * lex) {
					return (lex->lastToken == tokSpace) || (lex->lastToken == tokSym && lex->lastString[0] == ')');
				});
				if (eol) {
					// Invalid
					break;
				}
				if (lex->lastToken == tokSpace) {
					lex->gettok(); // Consume Space
				}
				std::tie(command, eol) = lex->readUntil([&inQuote](Parser * lex) {
					if (lex->lastToken == tokSym && lex->lastString[0] == '"') {
						inQuote = !inQuote;
					}
					return (!inQuote && lex->lastToken == tokSym && lex->lastString[0] == ')');
				});
				if (eol) {
					// Invalid
					break;
				}
				if (lex->lastInt > 1)
					lex->lastInt--;
				else
					lex->gettok();
				// Valid, URL, Command and Type populated
				success = true;
				break;
			case '^':
				type = '^';
				lex->gettok(); // Consume ^
				if (lex->lastToken != tokSym || lex->lastString[0] != '(' || lex->lastInt != 1) {
					// Invalid
					break;
				}
				lex->gettok(); // Consume (
				std::tie(url, eol) = lex->readUntil([](Parser * lex) {
					return (lex->lastToken == tokSpace) || (lex->lastToken == tokSym && lex->lastString[0] == ')');
				});
				if (eol) {
					// Invalid
					break;
				}
				if (lex->lastToken == tokSpace) {
					lex->gettok(); // Consume Space
				}
				std::tie(command, eol) = lex->readUntil([&inQuote](Parser * lex) {
					if (lex->lastToken == tokSym && lex->lastString[0] == '"') {
						inQuote = !inQuote;
					}
					return (!inQuote && lex->lastToken == tokSym && lex->lastString[0] == ')');
				});
				if (eol) {
					// Invalid
					break;
				}
				if (lex->lastInt > 1)
					lex->lastInt--;
				else
					lex->gettok();
				// Valid, URL, Command and Type populated
				success = true;
				break;
			case '#':
				type = '#';
				std::tie(url, succ) = lex->make_id(content->literalText());
				if (!succ) {
					// No valid ID
					break;
				}
				lex->gettok(); // Consume #
				if (lex->lastToken != tokSym || lex->lastString[0] != '(' || lex->lastInt != 1) {
					// Valid but no other specification
				}
				lex->gettok(); // Consume (
				std::tie(command, eol) = lex->readUntil([&inQuote](Parser * lex) {
					if (lex->lastToken == tokSym && lex->lastString[0] == '"') {
						inQuote = !inQuote;
					}
					return (!inQuote && lex->lastToken == tokSym && lex->lastString[0] == ')');
				});
				if (eol) {
					// Invalid
					break;
				}
				if (lex->lastInt > 1)
					lex->lastInt--;
				else
					lex->gettok();
				// Valid, URL, Command and Type populated
				success = true;
				break;
			case '<':
				type = '<';
				lex->gettok(); // Consume <
				if (lex->lastToken != tokSym || lex->lastString[0] != '%' || lex->lastInt != 1) {
					// Invalid
					break;
				}
				lex->gettok(); // Consume %
				std::tie(url, eol) = lex->readUntil([](Parser * lex) {
					return (lex->lastToken == tokSpace) || (lex->lastToken == tokSym && lex->lastString[0] == '>');
				});
				if (eol) {
					// Invalid
					break;
				}
				if (lex->lastToken == tokSpace) {
					lex->gettok(); // Consume Space
				}
				std::tie(command, eol) = lex->readUntil([&inQuote](Parser * lex) {
					if (lex->lastToken == tokSym && lex->lastString[0] == '"') {
						inQuote = !inQuote;
					}
					return (!inQuote && lex->lastToken == tokSym && lex->lastString[0] == '>');
				});
				if (eol) {
					// Invalid
					break;
				}
				if (lex->lastInt > 1)
					lex->lastInt--;
				else
					lex->gettok();
				// Valid, URL, Command and Type populated
				success = true;
				break;
			case '{':
				type = '{';
				url = "";
				lex->gettok(); // Consume {
				std::tie(command, eol) = lex->readUntil([&inQuote](Parser * lex) {
					if (lex->lastToken == tokSym && lex->lastString[0] == '"') {
						inQuote = !inQuote;
					}
					return (!inQuote && lex->lastToken == tokSym && lex->lastString[0] == '}');
				});
				if (eol) {
					// Invalid
					break;
				}
				if (lex->lastInt > 1)
					lex->lastInt--;
				else
					lex->gettok();
				// Valid, URL, Command and Type populated
				success = true;
				break;
			default:
				// Error, not valid
				break;
		}

		if (!success) {
			content->prependElement(std::make_unique<ASTPlainText>('['));
			content->addElement(std::make_unique<ASTPlainText>(']'));
			content->addElement(std::make_unique<ASTPlainText>(type));
			content->addElement(std::make_unique<ASTPlainText>(url));
			content->addElement(std::make_unique<ASTPlainText>(command));
			return std::make_tuple(std::move(content), true);
		}

		return std::make_tuple(std::make_unique<ASTModifier>(type, url, command, std::move(content)), true);
	}

	content->prependElement(std::make_unique<ASTPlainText>('['));

	return std::make_tuple(std::move(content), true);
}

#pragma endregion

#pragma region InlineSmileyHandler
// ----- InlineSmileyHandler ----- \\ 

std::unique_ptr<InlineHandler> InlineSmileyHandler::createNew() {
	return std::make_unique<InlineSmileyHandler>();
}

std::string InlineSmileyHandler::triggerChars() {
	return ":";
}

bool InlineSmileyHandler::canHandle(Parser * lex) {
	return (lex->lastToken == tokSym) &&
		(lex->lastString[0] == ':') &&
		(lex->peektok() != tokSpace) && (lex->peektok() != tokNewline);
}

std::tuple<std::unique_ptr<_ASTInlineElement>, bool> InlineSmileyHandler::handle(Parser * lex) {
	
	// If there are no indicators left open, e.g. **** -> *<firstContent>**<secondContent>*
	if (lex->lastInt % 2 == 0)
		return std::make_tuple(nullptr, true);

	lex->gettok(); // Consume opening indicator
	std::unique_ptr<ASTInlineText> content;
	bool endOfLine;
	std::tie(content, endOfLine) = lex->parseText(false, true, false, ':');
	
	if (!endOfLine) {
		// Ended on indicator
		lex->gettok(); // Consume closing indicator
		if (content != nullptr)
			return std::make_tuple(std::make_unique<ASTEmoji>(content->literalText()), true);
		return std::make_tuple(nullptr, true);
	}

	content->prependElement(std::make_unique<ASTPlainText>(':'));

	return std::make_tuple(std::move(content), true);
}

#pragma endregion
