#include "inline_handler.hpp"
#include "parser_handler.hpp"

void Parser::addDefaultHandlers() {
	
}

// ----- ParagraphHandler ----- \\ 

std::unique_ptr<ParserHandler> ParagraphHandler::createNew() {
	return std::make_unique<ParagraphHandler>();
}

bool ParagraphHandler::canHandle(Parser * lex) {
	return 
		(lex->lastToken == tokSpace) || 
		(lex->lastToken == tokText) || 
		(lex->lastToken == tokInlineSym);
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

// ----- HeadingHandler ----- \\ 

std::unique_ptr<ParserHandler> HeadingHandler::createNew() {
	return std::make_unique<HeadingHandler>();
}

std::string HeadingHandler::triggerChars() {
	return "#";
}

bool HeadingHandler::canHandle(Parser * lex) {
	return (lex->lastToken == tokSym) && 
		(lex->lastString.front() == '#') &&
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

// ----- HLineHandler ----- \\ 

std::unique_ptr<ParserHandler> HLineHandler::createNew() {
	return std::make_unique<HLineHandler>();
}

std::string HLineHandler::triggerChars() {
	return "-";
}

bool HLineHandler::canHandle(Parser * lex) {
	return (lex->lastToken == tokSym) && 
		(lex->lastString.front() == '-') &&
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
			(lex->lastString.front() == '>') && 
			(lex->lastInt == (centered ? 2 : 1)) && 
			(lex->peektok() == tokSpace || lex->peektok() == tokNewline));

	return (lex->lastToken == tokSym) &&
		(lex->lastString.front() == '>') && 
		(lex->lastInt <= 2) && 
		(lex->peektok() == tokSpace || lex->peektok() == tokNewline);
}

std::tuple<std::unique_ptr<_ASTElement>, bool> BlockquoteHandler::handle(Parser * lex) {
	if (content != nullptr && indentStyle == 0)
		indentStyle = lex->lastString.front();
	
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
		(lex->lastString.front() == '-') && 
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


// ----- InlineTemplateHandler ----- \\ 

// template<char indicator>
// std::unique_ptr<InlineHandler> InlineTemplateHandler<indicator>::createNew() {
// 	return std::make_unique<InlineTemplateHandler<indicator>>();
// }

// template<char indicator>
// std::string InlineTemplateHandler<indicator>::triggerChars() {
// 	return std::string(1, indicator);
// }

// template<char indicator>
// bool InlineTemplateHandler<indicator>::canHandle(Parser * lex) {
// 	return (lex->lastToken == tokInlineSym) &&
// 		(lex->lastString.front() == indicator) &&
// 		(lex->peektok() != tokSpace) && (lex->peektok() != tokNewline);
// }

// template<char indicator>
// std::tuple<std::unique_ptr<_ASTInlineElement>, bool> InlineTemplateHandler<indicator>::handle(Parser * lex) {
	
// 	// If there are no indicators left open, e.g. **** -> *<firstContent>**<secondContent>*
// 	if (lex->lastInt % 2 == 0)
// 		return std::make_tuple(nullptr, true);

// 	lex->gettok(); // Consume opening indicator
// 	std::unique_ptr<ASTInlineText> content;
// 	bool endOfLine;
// 	std::tie(content, endOfLine) = lex->parseText(false, true, indicator);
	
// 	if (!endOfLine) {
// 		// Ended on indicator
// 		lex->gettok(); // Consume closing indicator
// 		if (content != nullptr)
// 			return std::make_tuple(std::make_unique<ASTTextModification>(indicator, std::move(content)), true);
// 		return std::make_tuple(nullptr, true);
// 	}

// 	content->prependElement(std::make_unique<ASTPlainText>(1, indicator));

// 	return std::make_tuple(std::move(content), true);
// }

// template<char indicator>
// std::unique_ptr<_ASTElement> InlineTemplateHandler<indicator>::finish(Parser * lex) {
// 	return nullptr;
// }

// -----  ----- \\ 

