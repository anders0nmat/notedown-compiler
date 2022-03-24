#pragma once
#include "lexer.hpp"

template<char indicator>
class InlineTemplateHandler : public InlineHandler {
protected:

public:

	InlineTemplateHandler() {}

	std::unique_ptr<InlineHandler> createNew() {
		return std::make_unique<InlineTemplateHandler<indicator>>();
	}

	std::string triggerChars() {
		return std::string(1, indicator);
	}

	bool canHandle(Parser * lex) {
		return (lex->lastToken == tokInlineSym) &&
			(lex->lastString.front() == indicator) &&
			(lex->peektok() != tokSpace) && (lex->peektok() != tokNewline);
	}

	std::tuple<std::unique_ptr<_ASTInlineElement>, bool> handle(Parser * lex) {
		
		// If there are no indicators left open, e.g. **** -> *<firstContent>**<secondContent>*
		if (lex->lastInt % 2 == 0)
			return std::make_tuple(nullptr, true);

		lex->gettok(); // Consume opening indicator
		std::unique_ptr<ASTInlineText> content;
		bool endOfLine;
		std::tie(content, endOfLine) = lex->parseText(false, true, true, indicator);
		
		if (!endOfLine) {
			// Ended on indicator
			lex->gettok(); // Consume closing indicator
			if (content != nullptr)
				return std::make_tuple(std::make_unique<ASTTextModification>(indicator, std::move(content)), true);
			return std::make_tuple(nullptr, true);
		}

		content->prependElement(std::make_unique<ASTPlainText>(1, indicator));

		return std::make_tuple(std::move(content), true);
	}

	std::unique_ptr<_ASTElement> finish(Parser * lex) {
		return nullptr;
	}
};

class InlineCodeHandler : public InlineHandler {
protected:

public:

	InlineCodeHandler() {}

	std::unique_ptr<InlineHandler> createNew() {
		return std::make_unique<InlineCodeHandler>();
	}

	std::string triggerChars() {
		return "`";
	}

	bool canHandle(Parser * lex) {
		return (lex->lastToken == tokInlineSym) &&
			(lex->lastString.front() == '`') &&
			(lex->peektok() != tokSpace) && (lex->peektok() != tokNewline);
	}

	std::tuple<std::unique_ptr<_ASTInlineElement>, bool> handle(Parser * lex) {
		
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

		content->prependElement(std::make_unique<ASTPlainText>('`'));

		return std::make_tuple(std::move(content), true);
	}

	std::unique_ptr<_ASTElement> finish(Parser * lex) {
		return nullptr;
	}
};

// class InlineModifierHandler : public InlineHandler {
// protected:

// public:

// 	InlineModifierHandler() {}

// 	std::unique_ptr<InlineHandler> createNew() {
// 		return std::make_unique<InlineModifierHandler>();
// 	}

// 	std::string triggerChars() {
// 		return "[](){}<>";
// 	}

// 	bool canHandle(Parser * lex) {
// 		return (lex->lastToken == tokInlineSym) &&
// 			(lex->lastString.front() == '[') &&
// 			(lex->peektok() != tokSpace) && (lex->peektok() != tokNewline);
// 	}

// 	std::tuple<std::unique_ptr<_ASTInlineElement>, bool> handle(Parser * lex) {
		
// 		// If there are no indicators left open, e.g. **** -> *<firstContent>**<secondContent>*
// 		if (lex->lastInt == 1)
// 			lex->gettok(); // Consume opening indicator
// 		else
// 			lex->lastInt--;
// 		std::unique_ptr<ASTInlineText> content;
// 		bool endOfLine;
// 		std::tie(content, endOfLine) = lex->parseText(false, true, true, ']');
		
// 		if (!endOfLine) {
// 			// Ended on indicator
// 			if (lex->lastInt != 1) {
// 				// Invalid, multiple closing chars

// 			}
// 			lex->gettok(); // Consume closing indicator

// 			std::string command;
// 			bool success;

// 			if (content != nullptr)
// 				return std::make_tuple(std::make_unique<ASTTextModification>('`', std::move(content)), true);
// 			return std::make_tuple(nullptr, true);
// 		}

// 		content->prependElement(std::make_unique<ASTPlainText>('['));

// 		return std::make_tuple(std::move(content), true);
// 	}

// 	std::unique_ptr<_ASTElement> finish(Parser * lex) {
// 		return nullptr;
// 	}
// };

/*

gettext()
	if "
		blockBegin
		consume "
	
	while 
.	!blockbegin && space
	||	blockbegin && "	
		text <- current
	
	return text

link()
	gettext()
	space?
	while
		#?
			gettext()
		.?
			gettext()
		:?
			gettext()
		+?
			gettext()
			=?
			gettext()
		$?
			gettext()
			while
				,?
				gettext()
		>?
			gettext()
			:?
			gettext()
		error?

		consume space?



*/

