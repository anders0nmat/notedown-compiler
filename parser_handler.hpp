#pragma once
#include "lexer.hpp"

class ParagraphHandler : public ParserHandler {
protected:
	
	std::unique_ptr<ASTParagraph> content;

public:

	ParagraphHandler() {}

	std::unique_ptr<ParserHandler> createNew() override;

	bool canHandle(Parser * lex) override;

	std::tuple<std::unique_ptr<_ASTElement>, bool> handle(Parser * lex) override;

	std::unique_ptr<_ASTElement> finish(Parser * lex) override;
};

class HeadingHandler : public ParserHandler {
public:

	HeadingHandler() {}

	std::unique_ptr<ParserHandler> createNew() override;

	std::string triggerChars() override;

	bool canHandle(Parser * lex) override;

	std::tuple<std::unique_ptr<_ASTElement>, bool> handle(Parser * lex) override;

	std::unique_ptr<_ASTElement> finish(Parser * lex) override;
};

class HLineHandler : public ParserHandler {
public:

	HLineHandler() {}

	std::unique_ptr<ParserHandler> createNew() override;

	std::string triggerChars() override;

	bool canHandle(Parser * lex) override;

	std::tuple<std::unique_ptr<_ASTElement>, bool> handle(Parser * lex) override;

	std::unique_ptr<_ASTElement> finish(Parser * lex) override;
};

class BlockquoteHandler : public ParserHandler {
protected:
	
	std::unique_ptr<ASTBlockquote> content = nullptr;
	std::unique_ptr<ParserHandler> handler = nullptr;
	bool centered;

public:

	BlockquoteHandler() {}

	std::unique_ptr<ParserHandler> createNew() override;

	std::string triggerChars() override;

	bool canHandle(Parser * lex) override;

	std::tuple<std::unique_ptr<_ASTElement>, bool> handle(Parser * lex) override;

	std::unique_ptr<_ASTElement> finish(Parser * lex) override;
};

