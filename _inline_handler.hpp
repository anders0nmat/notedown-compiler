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
		return (lex->lastToken == tokSym) &&
			(lex->lastString[0] == indicator) &&
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

	std::unique_ptr<InlineHandler> createNew() override;

	std::string triggerChars() override;

	bool canHandle(Parser * lex) override;

	std::tuple<std::unique_ptr<_ASTInlineElement>, bool> handle(Parser * lex) override;
};

class InlineModifierHandler : public InlineHandler {
protected:

	/*
		parses everything of syntax <Keyword>, <Space>, <Commands>, <delim>
		@return { Keyword, Commands, Success }
	*/
	std::tuple<std::string, std::string, bool> parseLink(Parser * lex, char delim);

public:

	InlineModifierHandler() {}

	std::unique_ptr<InlineHandler> createNew() override;

	std::string triggerChars() override;

	bool canHandle(Parser * lex) override;

	std::tuple<std::unique_ptr<_ASTInlineElement>, bool> handle(Parser * lex) override;
};

class InlineSmileyHandler : public InlineHandler {
protected:

public:

	InlineSmileyHandler() {}

	std::unique_ptr<InlineHandler> createNew() override;

	std::string triggerChars() override;

	bool canHandle(Parser * lex) override;

	std::tuple<std::unique_ptr<_ASTInlineElement>, bool> handle(Parser * lex) override;
};

class InlineCommandHandler : public InlineHandler {
protected:

public:

};