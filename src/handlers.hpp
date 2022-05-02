#pragma once
#include "lexer.hpp"


// ------------------------------------ //
// ---------- Block Handlers ---------- //
// ------------------------------------ //


template<class cl>
class BlockHandler : public ParserHandler {
protected:

	std::unique_ptr<cl> content = nullptr;
	std::unique_ptr<ParserHandler> handler = nullptr;
	int indentLevel = 0;

	bool canHandleBlock(Parser * lex) {
		return 
			(content != nullptr) &&
			(
				(lex->lastToken == tokNewline) ||
				(
					(lex->lastToken == tokSpace) &&
					(indentLevel == 0 || lex->lastInt >= indentLevel)
				)
			);
	}

	void handleBlock(Parser * lex) {
		if (indentLevel == 0)
			indentLevel = lex->lastInt;

		if (lex->lastToken == tokSpace) {
			if (indentLevel == lex->lastInt)
				lex->gettok();
			else
				lex->lastInt -= indentLevel;
		}

		std::unique_ptr<_ASTElement> e;
		bool redo;
		do {
			std::tie(e, redo) = lex->parseLine(handler);
			content->addElement(e);
		} while (redo);
	}

	void finishBlock(Parser * lex) {
		if (handler != nullptr && content != nullptr) {
			std::unique_ptr<_ASTElement> e = std::move(handler->finish(lex));
			content->addElement(e);
		}
	}

public:

};

class ParagraphHandler : public ParserHandler {
protected:
	
	std::unique_ptr<ASTParagraph> content;

public:

	ParagraphHandler() {}

	std::unique_ptr<ParserHandler> createNew() override;

	bool canHandle(Parser * lex) override;

	Notedown::Handler::HandlerReturn handle(Parser * lex) override;

	std::unique_ptr<_ASTElement> finish(Parser * lex) override;
};

class HeadingHandler : public ParserHandler {
public:

	HeadingHandler() {}

	std::unique_ptr<ParserHandler> createNew() override;

	std::string triggerChars() override;

	bool canHandle(Parser * lex) override;

	Notedown::Handler::HandlerReturn handle(Parser * lex) override;

	std::unique_ptr<_ASTElement> finish(Parser * lex) override;
};

class HLineHandler : public ParserHandler {
public:

	HLineHandler() {}

	std::unique_ptr<ParserHandler> createNew() override;

	std::string triggerChars() override;

	bool canHandle(Parser * lex) override;

	Notedown::Handler::HandlerReturn handle(Parser * lex) override;

	std::unique_ptr<_ASTElement> finish(Parser * lex) override;
};

class BlockquoteHandler : public BlockHandler<ASTBlockquote> {
protected:
	
	bool centered;
	char indentStyle = 0;

public:

	BlockquoteHandler() {}

	std::unique_ptr<ParserHandler> createNew() override;

	std::string triggerChars() override;

	bool canHandle(Parser * lex) override;

	Notedown::Handler::HandlerReturn handle(Parser * lex) override;

	std::unique_ptr<_ASTElement> finish(Parser * lex) override;
};

class UnorderedListHandler : public BlockHandler<ASTListElement> {
protected:

	std::unique_ptr<ASTUnorderedList> list;

public:

	std::unique_ptr<ParserHandler> createNew() override;

	std::string triggerChars() override;

	bool canHandle(Parser * lex) override;

	Notedown::Handler::HandlerReturn handle(Parser * lex) override;

	std::unique_ptr<_ASTElement> finish(Parser * lex) override;
};

class OrderedListHandler : public BlockHandler<ASTListElement> {
protected:

	std::unique_ptr<ASTOrderedList> list;

public:

	std::unique_ptr<ParserHandler> createNew() override;

	std::string triggerChars() override;

	bool canHandle(Parser * lex) override;

	Notedown::Handler::HandlerReturn handle(Parser * lex) override;

	std::unique_ptr<_ASTElement> finish(Parser * lex) override;
};

class CodeHandler : public ParserHandler {
protected:
	
	std::vector<std::unique_ptr<_ASTInlineElement>> content;
	int fenceCount = 0;
	std::string lang;
	std::unique_ptr<ASTInlineText> firstLine;

public:

	CodeHandler() {}

	std::unique_ptr<ParserHandler> createNew() override;

	std::string triggerChars() override;

	bool canHandle(Parser * lex) override;

	Notedown::Handler::HandlerReturn handle(Parser * lex) override;

	std::unique_ptr<_ASTElement> finish(Parser * lex) override;
};

class InfoBlockHandler : public BlockHandler<ASTInfoBlock> {
protected:

public:

	InfoBlockHandler() {}

	std::unique_ptr<ParserHandler> createNew() override;

	std::string triggerChars() override;

	bool canHandle(Parser * lex) override;

	Notedown::Handler::HandlerReturn handle(Parser * lex) override;

	std::unique_ptr<_ASTElement> finish(Parser * lex) override;
};

class IdDefinitionHandler : public BlockHandler<ASTIdDefinition> {
protected:

	char type;

	char invType();

public:

	IdDefinitionHandler() {}

	std::unique_ptr<ParserHandler> createNew() override;

	std::string triggerChars() override;

	bool canHandle(Parser * lex) override;

	Notedown::Handler::HandlerReturn handle(Parser * lex) override;

	std::unique_ptr<_ASTElement> finish(Parser * lex) override;
};

class FootnoteHandler : public BlockHandler<ASTFootnoteBlock> {
public:
	FootnoteHandler() {}

	std::unique_ptr<ParserHandler> createNew() override;

	std::string triggerChars() override;

	bool canHandle(Parser * lex) override;

	Notedown::Handler::HandlerReturn handle(Parser * lex) override;

	std::unique_ptr<_ASTElement> finish(Parser * lex) override;
};

class CollapseHandler : public BlockHandler<ASTCollapseBlock> {
protected:
	// std::unique_ptr<ASTCollapseBlock> content;
	// std::unique_ptr<ParserHandler> handler;
public:

	CollapseHandler() {}

	std::unique_ptr<ParserHandler> createNew() override;
	
	std::string triggerChars() override;

	bool canHandle(Parser * lex) override;

	Notedown::Handler::HandlerReturn handle(Parser * lex) override;

	std::unique_ptr<_ASTElement> finish(Parser * lex) override;
};


// ------------------------------------- //
// ---------- Inline Handlers ---------- //
// ------------------------------------- //


template<char indicator>
class InlineTemplateHandler : public InlineHandler {
protected:

public:

	InlineTemplateHandler() {}

	std::unique_ptr<InlineHandler> createNew() override {
		return std::make_unique<InlineTemplateHandler<indicator>>();
	}

	std::string triggerChars() override {
		return std::string(1, indicator);
	}

	bool canHandle(Parser * lex) override {
		return (lex->lastToken == tokSym) &&
			(lex->lastString[0] == indicator) &&
			(lex->lastInt % 2 != 0) && // At least one Indicator left open, e.g. *** -> *<firstContent>**<secondContent>
			(lex->peektok() != tokSpace) && (lex->peektok() != tokNewline);
	}

	Notedown::Handler::InlineHandlerReturn handle(Parser * lex) override {
		lex->gettok(); // Consume opening indicator
		std::unique_ptr<ASTInlineText> content;
		bool endOfLine;
		std::tie(content, endOfLine) = lex->parseText(false, true, true, indicator);
		
		if (!endOfLine) {
			// Ended on indicator
			lex->gettok(); // Consume closing indicator
			if (content != nullptr)
				return Notedown::Handler::make_result(std::make_unique<ASTTextModification>(indicator, std::move(content)), true);
			return Notedown::Handler::make_result(nullptr, true);
		}

		content->prependElement(std::make_unique<ASTPlainText>(1, indicator));

		return Notedown::Handler::make_result(content, true);
	}

	std::unique_ptr<_ASTElement> finish(Parser * lex) override {
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

	Notedown::Handler::InlineHandlerReturn handle(Parser * lex) override;
};

class InlineModifierHandler : public InlineHandler {
protected:

	/*
		parses everything of syntax <Keyword>, <Space>, <Commands>, <delim>
		@return { Keyword, Commands, Success }
	*/
	// std::tuple<std::string, std::string, bool> parseLink(Parser * lex, char delim);

public:

	InlineModifierHandler() {}

	std::unique_ptr<InlineHandler> createNew() override;

	std::string triggerChars() override;

	bool canHandle(Parser * lex) override;

	Notedown::Handler::InlineHandlerReturn handle(Parser * lex) override;
};

class InlineSmileyHandler : public InlineHandler {
protected:

public:

	InlineSmileyHandler() {}

	std::unique_ptr<InlineHandler> createNew() override;

	std::string triggerChars() override;

	bool canHandle(Parser * lex) override;

	Notedown::Handler::InlineHandlerReturn handle(Parser * lex) override;
};

class InlineCommandHandler : public InlineHandler {
protected:

public:
	InlineCommandHandler() {}

	std::unique_ptr<InlineHandler> createNew() override;

	std::string triggerChars() override;

	bool canHandle(Parser * lex) override;

	Notedown::Handler::InlineHandlerReturn handle(Parser * lex) override;
};

class InlineTaskHandler : public InlineHandler {
protected:

public:

	InlineTaskHandler() {}

	std::unique_ptr<InlineHandler> createNew() override;

	std::string triggerChars() override;

	bool canHandle(Parser * lex) override;

	Notedown::Handler::InlineHandlerReturn handle(Parser * lex) override;
};
