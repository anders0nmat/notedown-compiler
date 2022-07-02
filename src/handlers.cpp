#include "handlers.hpp"
#include "notedown-templates.hpp"

using namespace Notedown::Handler;

void NotedownCompiler::addDefaultHandlers() {
	addHandler<UnorderedListHandler>("H_ulist");
	addHandler<OrderedListHandler>("H_olist");
	addHandler<HeadingHandler>("H_heading");
	addHandler<InfoBlockHandler>("H_infoblock");
	addHandler<BlockquoteHandler>("H_blockquote");
	addHandler<HLineHandler>("H_hline");
	addHandler<CodeHandler>("H_code");
	addHandler<IdDefinitionHandler>("H_iddef");
	addHandler<FootnoteHandler>("H_footnote");
	addHandler<CollapseHandler>("H_collapse");

	addInlineHandler<InlineTemplateHandler<'*'>>("I_bold");
	addInlineHandler<InlineTemplateHandler<'/'>>("I_italic");
	addInlineHandler<InlineTemplateHandler<'_'>>("I_underlined");
	addInlineHandler<InlineTemplateHandler<'~'>>("I_strikethrough");
	addInlineHandler<InlineTemplateHandler<'='>>("I_highlight");

	addInlineHandler<InlineTaskHandler>("I_task");
	addInlineHandler<InlineSmileyHandler>("I_emoji");
	addInlineHandler<InlineCodeHandler>("I_code");
	addInlineHandler<InlineModifierHandler>("I_link");
	addInlineHandler<InlineCommandHandler>("I_command");


	addHandler<ParagraphHandler>("H_paragraph");
	addHandlerAlias("H_default", "H_paragraph");
}


// ------------------------------------ //
// ---------- Block Handlers ---------- //
// ------------------------------------ //


// ----- ParagraphHandler ----- // 

std::unique_ptr<ParserHandler> ParagraphHandler::createNew() {
	return newSelf<ParagraphHandler>();
}

bool ParagraphHandler::canHandle(Parser * lex) {
	return 
		(lex->lastToken == tokSpace) || 
		(lex->lastToken == tokText);
}

HandlerReturn ParagraphHandler::handle(Parser * lex) {
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
			return make_result(nullptr, true);
		return make_result(content, true);
	}
	content->addElement(std::move(e));

	// Dont finish, dont return anything
	return handler_continue;
}

std::unique_ptr<_ASTElement> ParagraphHandler::finish(Parser * lex) {
	return std::move(content);
}


// ----- HeadingHandler ----- //

std::unique_ptr<ParserHandler> HeadingHandler::createNew() {
	return newSelf<HeadingHandler>();
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

HandlerReturn HeadingHandler::handle(Parser * lex) {
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
		return make_result(std::make_unique<ASTHeading>(level, nullptr), true);
	}

	std::unique_ptr<ASTInlineText> t;
	std::tie(t, std::ignore) = lex->parseText(false);
	lex->gettok(); // Consume newline

	return make_result(std::make_unique<ASTHeading>(level, std::move(t)), true);
}

std::unique_ptr<_ASTElement> HeadingHandler::finish(Parser * lex) {
	return nullptr;
}


// ----- HLineHandler ----- //

std::unique_ptr<ParserHandler> HLineHandler::createNew() {
	return newSelf<HLineHandler>();
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

HandlerReturn HLineHandler::handle(Parser * lex) {
	lex->gettok(); // Consume ---
	lex->gettok(); // Consume Newline
	return make_result(std::make_unique<ASTHLine>(), true);
}

std::unique_ptr<_ASTElement> HLineHandler::finish(Parser * lex) {
	return nullptr;
}


// ----- BlockquoteHandler ----- //

std::unique_ptr<ParserHandler> BlockquoteHandler::createNew() {
	return newSelf<BlockquoteHandler>();
}

std::string BlockquoteHandler::triggerChars() {
	return ">";
}

bool BlockquoteHandler::canHandle(Parser * lex) {
	if (content != nullptr)
		return canHandleBlock(lex);

	return (lex->lastToken == tokSym) &&
		(lex->lastString[0] == '>') && 
		(lex->lastInt <= 2) && 
		(lex->peektok() == tokSpace || lex->peektok() == tokNewline);
}

HandlerReturn BlockquoteHandler::handle(Parser * lex) {
	if (content == nullptr) {
		content = std::make_unique<ASTBlockquote>(lex->lastInt > 1);
		centered = lex->lastInt > 1;
	}

	if (lex->lastToken == tokSpace || lex->lastToken == tokNewline) {
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

	return handler_continue;
}

std::unique_ptr<_ASTElement> BlockquoteHandler::finish(Parser * lex) {
	finishBlock(lex);
	return std::move(content);
}


// ----- UnorderedListHandler ----- //

std::unique_ptr<ParserHandler> UnorderedListHandler::createNew() {
	return newSelf<UnorderedListHandler>();
}

std::string UnorderedListHandler::triggerChars() {
	return "-";
}

bool UnorderedListHandler::canHandle(Parser * lex) {
	return canHandleBlock(lex) ||
		(
			(lex->isLast(tokSym, '-', 1)) && 
			(lex->peektok() == tokSpace || lex->peektok() == tokNewline)
		);
}

HandlerReturn UnorderedListHandler::handle(Parser * lex) {
	if (list == nullptr) {
		// Create List
		list = std::make_unique<ASTUnorderedList>();
	}

	if (lex->lastToken == tokSpace || lex->lastToken == tokNewline) {
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

	return handler_continue;
}

std::unique_ptr<_ASTElement> UnorderedListHandler::finish(Parser * lex) {
	finishBlock(lex);
	if (list != nullptr)
		list->addElement(content);
	return std::move(list);
}


// ----- OrderedListHandler ----- //

std::unique_ptr<ParserHandler> OrderedListHandler::createNew() {
	return newSelf<OrderedListHandler>();
}

std::string OrderedListHandler::triggerChars() {
	return "";
}

bool OrderedListHandler::canHandle(Parser * lex) {
	return canHandleBlock(lex) ||
		((lex->lastToken == tokNumber) &&
		(lex->lastString[lex->lastString.length() - 1] == '.') &&
		(lex->peektok() == tokSpace || lex->peektok() == tokNewline));
}

HandlerReturn OrderedListHandler::handle(Parser * lex) {
	if (list == nullptr) {
		// Create List
		list = std::make_unique<ASTOrderedList>();
	}

	if (lex->lastToken == tokSpace || lex->lastToken == tokNewline) {
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

	return handler_continue;
}

std::unique_ptr<_ASTElement> OrderedListHandler::finish(Parser * lex) {
	finishBlock(lex);
	if (list != nullptr)
		list->addElement(content);
	return std::move(list);
}


// ----- CodeHandler ----- //

std::unique_ptr<ParserHandler> CodeHandler::createNew() {
	return newSelf<CodeHandler>();
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

HandlerReturn CodeHandler::handle(Parser * lex) {
	if (fenceCount == 0) {
		// Remove Spaces in front
		while (lex->lastToken == tokSpace) 
			lex->gettok(); // Eat Space
		fenceCount = lex->lastInt;
		lex->gettok(); // Eat ```
		bool success;
		std::tie(lang, success) = lex->readUntil([](Parser * lex){return lex->lastToken == tokSpace;});
		if (success) { // If there is more to read
			lex->gettok(); // Eat Space
			std::tie(firstLine, std::ignore) = lex->parseText(false);
			lex->gettok(); // Eat Newline
		}
		return handler_continue;
	}

	std::string currLine;
	bool success;
	int count = fenceCount;

	std::tie(currLine, success) = lex->readUntil([count](Parser * lex) {
		return (lex->lastToken == tokSym && lex->lastString[0] == '`' && lex->lastInt == count && (lex->peektok() == tokNewline || lex->peektok() == tokEOF));
	});

	// One only escapes if it is newline or end of block
	
	content.push_back(std::make_unique<ASTPlainText>(currLine));
	if (success) {
		// Finishing symbol
		lex->gettok(); // Consume ```
		lex->gettok(); // Consume newline
		std::unique_ptr<ASTCodeBlock> code = std::make_unique<ASTCodeBlock>(lang);
		code->addCommand(firstLine);
		for (auto & e : content)
			code->addElement(std::move(e));
		return make_result(std::move(code), true);
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
		// 		return make_result(std::move(code), true);
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
	return handler_continue;
}

std::unique_ptr<_ASTElement> CodeHandler::finish(Parser * lex) {
	std::unique_ptr<ASTParagraph> p = std::make_unique<ASTParagraph>();
	for (auto & e : content)
		p->addElement(std::move(e));
	return std::move(p);
}


// ----- InfoBlockHandler ----- //

std::unique_ptr<ParserHandler> InfoBlockHandler::createNew() {
	return newSelf<InfoBlockHandler>();
}

std::string InfoBlockHandler::triggerChars() {
	return ">:";
}

bool InfoBlockHandler::canHandle(Parser * lex) {
	if (content != nullptr)
		return canHandleBlock(lex);

	return (lex->isLast(tokSym, '>', 1)) &&
		((lex->peektok() == tokText) ||
		(lex->peektok() == tokSym && lex->peekchar() == ':'));
}

HandlerReturn InfoBlockHandler::handle(Parser * lex) {	
	if (content == nullptr) {
		lex->gettok(); // Consume >
		bool success, sym = false;
		std::string type;
		if (lex->lastToken == tokText) {
			std::tie(type, success) = lex->readUntil([](Parser * lex) {
				return (lex->lastToken == tokSpace) ||
					(lex->lastToken == tokSym);
			});
			if (!success || lex->lastToken == tokSpace || lex->lastString[0] != '>' || lex->peektok() != tokSpace) {
				return handler_error;
			}
		}
		else {
			if (lex->lastString[0] != ':' || lex->lastInt != 1) {
				return handler_error;
			}
			lex->gettok(); // Consume :
			std::tie(type, success) = lex->readUntil([](Parser * lex) {
				return (lex->lastToken == tokSpace) ||
					(lex->lastToken == tokSym);
			});
			if (!success || lex->lastToken == tokSpace || lex->lastString[0] != ':' || lex->lastInt != 1) {
				// Error
				return handler_error;
			}
			lex->gettok(); // Consume closing :
			if (lex->lastToken != tokSym || lex->lastString[0] != '>' || lex->lastInt != 1 || lex->peektok() != tokSpace) {
				// Error
				return handler_error;
			}
			sym = true;
		}
		content = std::make_unique<ASTInfoBlock>(type, sym);

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

		return handler_continue;
	}
	
	handleBlock(lex);

	return handler_continue;
}

std::unique_ptr<_ASTElement> InfoBlockHandler::finish(Parser * lex) {
	finishBlock(lex);
	return std::move(content);
}

// ----- IdDefinitionHandler ----- //

std::unique_ptr<ParserHandler> IdDefinitionHandler::createNew() {
	return newSelf<IdDefinitionHandler>();
}

char IdDefinitionHandler::invType() {
	switch (type) {
		case '(': return ')';
		case '{': return '}';
		case '<': return '>';
		default: return '\0';
	}
}

std::string IdDefinitionHandler::triggerChars() {
	return "%(){}<>:";
}

bool IdDefinitionHandler::canHandle(Parser * lex) {
	if (content != nullptr && type == '<')
		return canHandleBlock(lex);
	
	return (lex->isLast(tokSym, '%', 1)) &&
		(lex->peektok() == tokSym) &&
		(
			(lex->peekchar() == '(') ||
			(lex->peekchar() == '{') ||
			(lex->peekchar() == '<')
		);
}

HandlerReturn IdDefinitionHandler::handle(Parser * lex) {
	if (content == nullptr) {
		lex->gettok(); // Consume %
		type = lex->lastString[0];
		lex->gettok(); // Consume opening bracket
		std::string id;
		bool success;
		char _type = invType();
		std::tie(id, success) = lex->readUntil([_type](Parser * lex) {
			return (lex->lastToken == tokSym) &&
				(lex->lastString[0] == _type) &&
				(lex->lastInt == 1) &&
				(lex->peektok() == tokSym) &&
				(lex->peekchar() == ':');
		});

		if (!success) {
			return handler_error;
		}

		lex->gettok(); // Consume closing Bracket
		if (lex->lastInt != 1 || lex->peektok() != tokSpace) {
			// More than one : or no space following
			return handler_error;
		}

		lex->gettok(); // Consume :
		lex->gettok(); // Consume Spaces

		std::string url;
		std::string command;
		std::unique_ptr<_ASTElement> e;
		std::unique_ptr<ASTInlineText> t;
		std::unique_ptr<ASTParagraph> p;
		bool inQuote;

		switch (type) {
			case '(':
				// <Url or Id> <Commands>
				std::tie(url, command, success) = lex->parseLink('\0');
				content = std::make_unique<ASTIdDefinition>(id, url, type);
				content->addCommand(ASTCommand(command));
				return make_result(content, true);
			case '{':
				// <Commands>
				std::tie(command, success) = lex->readUntil([&inQuote](Parser * lex) {
					return false;
				});
				content = std::make_unique<ASTIdDefinition>(id, "", type);
				content->addCommand(ASTCommand(command));
				return make_result(content, true);
			case '<':
				// <Replace Content>
				content = std::make_unique<ASTIdDefinition>(id, "", type);
				bool redo;
				do {
					std::tie(e, redo) = lex->parseLine(handler);
					content->addElement(e);
				} while (redo);
				break;
			default:
				return handler_error;
		}
		return handler_continue;
	}
	
	handleBlock(lex);

	return handler_continue;
}

std::unique_ptr<_ASTElement> IdDefinitionHandler::finish(Parser * lex) {
	finishBlock(lex);
	return std::move(content);
}

// ----- FootnoteHandler ----- //

std::unique_ptr<ParserHandler> FootnoteHandler::createNew() {
	return newSelf<FootnoteHandler>();
}

std::string FootnoteHandler::triggerChars() {
	return "^:";
}

bool FootnoteHandler::canHandle(Parser * lex) {
	if (content != nullptr)
		return canHandleBlock(lex);
	return 
		(lex->isLast(tokSym, '^', 1)) &&
		(
			(lex->peektok() == tokText) ||
			(lex->peektok() == tokNumber)
		);
}

HandlerReturn FootnoteHandler::handle(Parser * lex) {
	if (content == nullptr) {
		lex->gettok(); // Consume ^
		std::string id;
		bool succ;
		std::tie(id, succ) = lex->readUntil([](Parser * lex) {
			return (lex->lastToken == tokSym) &&
				(lex->lastInt == 1) &&
				(lex->lastString[0] == ':') &&
				(lex->peektok() == tokSpace);
		});
		if (!succ) {
			return handler_error;
		}
		lex->gettok(); // Consume :
		lex->gettok(); // Consume spaces
		content = std::make_unique<ASTFootnoteBlock>(Notedown::makeId(id));
		bool redo;
		std::unique_ptr<_ASTElement> e;
		do {
			std::tie(e, redo) = lex->parseLine(handler);
			content->addElement(e);
		} while (redo);
		return handler_continue;
	}	
	handleBlock(lex);
	return handler_continue;
}

std::unique_ptr<_ASTElement> FootnoteHandler::finish(Parser * lex) {
	finishBlock(lex);
	return std::move(content);
}

// ----- CollapseHandler ----- //

std::unique_ptr<ParserHandler> CollapseHandler::createNew() {
	return newSelf<CollapseHandler>();
}
	
std::string CollapseHandler::triggerChars() {
	return "+-";
}

bool CollapseHandler::canHandle(Parser * lex) {
	if (content != nullptr)
		return canHandleBlock(lex);

	return 
		(lex->lastToken == tokSym) &&
		(lex->lastInt <= 2) &&
		(lex->lastString[0] == '+') &&
		(lex->peektok() == tokSym) &&
		(lex->peekchar() == '-')
	;
}

HandlerReturn CollapseHandler::handle(Parser * lex) {
	if (content == nullptr) {
		bool isOpen = lex->lastInt == 2;
		lex->gettok(); // Consume +

		if (lex->lastToken != tokSym || lex->lastString[0] != '-' || lex->lastInt != 1 + !isOpen || (lex->peektok() != tokSpace && lex->peektok() != tokNewline)) {
			// error
			return handler_error;
		}
		lex->gettok(); // Consume -

		if (lex->lastToken == tokSpace)
			lex->gettok(); // Consume Space

		std::unique_ptr<ASTInlineText> e;
		std::tie(e, std::ignore) = lex->parseText(false);
		lex->gettok(); // Consume newline
		content = std::make_unique<ASTCollapseBlock>(isOpen, std::move(e));
		return handler_continue;
	}

	handleBlock(lex);
	
	return handler_continue;
}

std::unique_ptr<_ASTElement> CollapseHandler::finish(Parser * lex) {
	finishBlock(lex);
	return std::move(content);
}


// ------------------------------------- //
// ---------- Inline Handlers ---------- //
// ------------------------------------- //

// ----- InlineCodeHandler ----- //

std::unique_ptr<InlineHandler> InlineCodeHandler::createNew() {
	return newSelf<InlineCodeHandler>();
}

std::string InlineCodeHandler::triggerChars() {
	return "`";
}

bool InlineCodeHandler::canHandle(Parser * lex) {
	return (lex->lastToken == tokSym) &&
		(lex->lastString[0] == '`') &&
		(lex->lastInt % 2 != 0) && // At least one Indicator left open, e.g. *** -> *<firstContent>**<secondContent>
		(lex->peektok() != tokSpace) && (lex->peektok() != tokNewline);
}

InlineHandlerReturn InlineCodeHandler::handle(Parser * lex) {
	lex->gettok(); // Consume opening indicator
	std::unique_ptr<ASTInlineText> content;
	bool endOfLine;
	std::tie(content, endOfLine) = lex->parseText(false, true, false, '`');
	
	if (!endOfLine) {
		// Ended on indicator
		lex->gettok(); // Consume closing indicator
		if (content != nullptr)
			return make_result(std::make_unique<ASTTextModification>('`', std::move(content)), true);
		return make_result(nullptr, true);
	}
	if (content != nullptr)
		return handler_error;

	return make_result(content, true);
}


// ----- InlineModifierHandler ----- //

std::unique_ptr<InlineHandler> InlineModifierHandler::createNew() {
	return newSelf<InlineModifierHandler>();
}

std::string InlineModifierHandler::triggerChars() {
	return "[](){}<>\"!^#%";
}

bool InlineModifierHandler::canHandle(Parser * lex) {
	return (lex->lastToken == tokSym) &&
		(lex->lastString[0] == '[') &&
		(lex->peektok() != tokSpace) && (lex->peektok() != tokNewline);
}

InlineHandlerReturn InlineModifierHandler::handle(Parser * lex) {
	lex->gettok(1);
	std::unique_ptr<ASTInlineText> content;
	bool endOfLine;
	std::tie(content, endOfLine) = lex->parseText(false, true, true, ']');
	
	if (endOfLine) {
		return handler_error;
	}

	// Ended on indicator
	if (lex->lastInt != 1) {
		// Invalid, multiple closing chars
		return handler_error;
	}
	lex->gettok(); // Consume closing indicator

	std::string command, keyword;
	std::unique_ptr<_ASTInlineElement> result;
	bool inQuote = false, success = false;

	if (lex->lastToken != tokSym || lex->lastInt != 1) {
		// Can not possibly be valid
		return handler_error;
	}

	switch (lex->lastString[0]) {
		case '(':
			lex->gettok(); // Consume (
			std::tie(keyword, command, success) = lex->parseLink(')');
			if (!success) {
				return handler_error;
			}
			result = std::make_unique<ASTLink>(keyword, content);
			result->addCommand(ASTCommand(command));
			return make_result(result, true);
		case '!':
			lex->gettok(); // Consume !
			if (lex->lastToken != tokSym || lex->lastString[0] != '(' || lex->lastInt != 1) {	
				return handler_error;
			}
			lex->gettok(); // Consume (
			std::tie(keyword, command, success) = lex->parseLink(')');
			if (!success) {
				return handler_error;
			}
			result = std::make_unique<ASTImage>(keyword, content);
			result->addCommand(ASTCommand(command));
			return make_result(std::move(result), true);
		case '^':
			lex->gettok(); // Consume ^
			if (lex->lastToken != tokSym || lex->lastString[0] != '(' || lex->lastInt != 1) {
				return handler_error;
			}
			lex->gettok(); // Consume (
			std::tie(keyword, command, success) = lex->parseLink(')');
			if (!success) {
				return handler_error;
			}
			result = std::make_unique<ASTFootnote>(keyword, content);
			result->addCommand(ASTCommand(command));
			return make_result(std::move(result), true);
		case '#':
			keyword = Notedown::makeId(content->literalText());
			lex->gettok(); // Consume #
			if (lex->lastToken != tokSym || lex->lastString[0] != '{' || lex->lastInt != 1) {
				// Valid but no other specification
				result = std::make_unique<ASTHeadingLink>(keyword, content);
				return make_result(std::move(result), true);
			}
			lex->gettok(); // Consume (
			std::tie(command, success) = lex->readUntil([&inQuote](Parser * lex) {
				if (lex->lastToken == tokSym && lex->lastString[0] == '"') {
					inQuote = !inQuote;
				}
				return (!inQuote && lex->lastToken == tokSym && lex->lastString[0] == '}');
			});
			if (!success) {
				return handler_error;
			}
			lex->gettok(1);
			// Valid, URL, Command and Type populated
			result = std::make_unique<ASTHeadingLink>(keyword, content);
			result->addCommand(ASTCommand(command));
			return make_result(std::move(result), true);
		case '<':
			lex->gettok(); // Consume <
			if (lex->lastToken != tokSym || lex->lastString[0] != '%' || lex->lastInt != 1) {
				return handler_error;
			}
			lex->gettok(); // Consume %
			std::tie(keyword, command, success) = lex->parseLink('>');
			if (!success) {
				return handler_error;
			}
			result = std::make_unique<ASTReplace>(keyword, content);
			result->addCommand(ASTCommand(command));
			return make_result(std::move(result), true);
		case '{':
			lex->gettok(); // Consume {
			// Read Command line literally
			std::tie(command, success) = lex->readUntil([&inQuote](Parser * lex) {
				if (lex->lastToken == tokSym && lex->lastString[0] == '"') {
					inQuote = !inQuote;
				}
				return (!inQuote && lex->lastToken == tokSym && lex->lastString[0] == '}');
			});
			if (!success) {
				// No closing bracket, surround read content with "[content]{command"
				return handler_error;
			}
			lex->gettok(1);
			result = std::make_unique<ASTStyled>(keyword, content);
			result->addCommand(ASTCommand(command));
			return make_result(std::move(result), true);
		default:
			// Error, not valid
			break;
	}

	// If we reach this point, something failed after "[text]", so we print content surrounded by square brackets
	return handler_error;
}


// ----- InlineSmileyHandler ----- //

std::unique_ptr<InlineHandler> InlineSmileyHandler::createNew() {
	return newSelf<InlineSmileyHandler>();
}

std::string InlineSmileyHandler::triggerChars() {
	return ":";
}

bool InlineSmileyHandler::canHandle(Parser * lex) {
	return (lex->lastToken == tokSym) &&
		(lex->lastString[0] == ':') &&
		(lex->peektok() != tokSpace) && (lex->peektok() != tokNewline);
}

InlineHandlerReturn InlineSmileyHandler::handle(Parser * lex) {
	
	// If there are no indicators left open, e.g. **** -> *<firstContent>**<secondContent>*
	if (lex->lastInt % 2 == 0)
		return make_result(nullptr, true);

	lex->gettok(); // Consume opening indicator
	std::unique_ptr<ASTInlineText> content;
	bool endOfLine;
	std::tie(content, endOfLine) = lex->parseText(false, true, false, ':');
	
	if (!endOfLine) {
		// Ended on indicator
		lex->gettok(); // Consume closing indicator
		if (content != nullptr)
			return make_result(std::make_unique<ASTEmoji>(content->literalText()), true);
		return make_result(nullptr, true);
	}

	return handler_error;
}

// ----- InlineCommandHandler ----- //

std::unique_ptr<InlineHandler> InlineCommandHandler::createNew() {
	return newSelf<InlineCommandHandler>();
}

std::string InlineCommandHandler::triggerChars() {
	return "{}";
}

bool InlineCommandHandler::canHandle(Parser * lex) {
	return (lex->lastToken == tokSym) &&
		(lex->lastString[0] == '{') &&
		(lex->peektok() != tokSpace) &&
		(lex->peektok() != tokNewline) &&
		(lex->peektok() != tokEOF);
}

InlineHandlerReturn InlineCommandHandler::handle(Parser * lex) {
	lex->gettok(); // Consume {

	std::string command;
	bool success, inQuote;

	// Read Command line literally
	std::tie(command, success) = lex->readUntil([&inQuote](Parser * lex) {
		if (lex->lastToken == tokSym && lex->lastString[0] == '"') {
			inQuote = !inQuote;
		}
		return (!inQuote && lex->lastToken == tokSym && lex->lastString[0] == '}');
	});
	if (!success) {
		return handler_error;
	}
	lex->gettok(1); // Consume closing char
	std::unique_ptr<ASTCommandContainer> result = std::make_unique<ASTCommandContainer>();
	result->addCommand(ASTCommand(command));
	return make_result(std::move(result), true);
}

// ----- InlineTaskHandler ----- //

std::unique_ptr<InlineHandler> InlineTaskHandler::createNew() {
	return newSelf<InlineTaskHandler>();
}

std::string InlineTaskHandler::triggerChars() {
	return "[]";
}

bool InlineTaskHandler::canHandle(Parser * lex) {
	return (lex->lastToken == tokSym) &&
		(lex->lastString[0] == '[') &&
		(lex->lastInt == 1) &&
		(
			(lex->peektok() == tokSpace) ||
			(
				(lex->peektok() == tokText) &&
				(
					(lex->peekchar() == 'X') || 
					(lex->peekchar() == 'x')
				)
			) ||
			(
				(lex->peektok() == tokSym) &&
				(lex->peekchar() == ']')
			)
		);
}

InlineHandlerReturn InlineTaskHandler::handle(Parser * lex) {
	lex->gettok(); // Consume [
	if (lex->lastToken == tokSpace && lex->lastInt == 1) {
		lex->gettok(); // Consume Space
		if (lex->lastToken == tokSym && lex->lastInt == 1 && lex->lastString[0] == ']' && lex->peektok() == tokSpace) {
			// Valid
			lex->gettok(); // Consume ]
			return make_result(std::make_unique<ASTTask>(' '), true);
		}
	}
	else if (lex->lastToken == tokSym && lex->lastInt == 1 && lex->lastString[0] == ']' && lex->peektok() == tokSpace) {
		// Valid
		lex->gettok(); // Consume ]
		return make_result(std::make_unique<ASTTask>(' '), true);
	}
	else if (lex->lastToken == tokText && (lex->lastString == "X" || lex->lastString == "x")) {
		lex->gettok(); // Consume x
		char checked = lex->lastString[0];
		if (lex->lastToken == tokSym && lex->lastInt == 1 && lex->lastString[0] == ']' && lex->peektok() == tokSpace) {
			// Valid
			lex->gettok(); // Consume ]
			return make_result(std::make_unique<ASTTask>(checked), true);
		}
	}
	return handler_error;
}
