#pragma once
#include "lexer.hpp"

template<class cl>
class BlockHandler : public ParserHandler {
protected:

	std::unique_ptr<cl> content = nullptr;
	std::unique_ptr<ParserHandler> handler = nullptr;
	int indentLevel = 0;

	bool canHandleBlock(Parser * lex) {
		return (content != nullptr) &&
			(lex->lastToken == tokSpace) &&
			(indentLevel == 0 || lex->lastInt >= indentLevel);
	}

	void handleBlock(Parser * lex) {
		if (indentLevel == 0)
			indentLevel = lex->lastInt;

		if (indentLevel == lex->lastInt)
			lex->gettok();
		else
			lex->lastInt -= indentLevel;

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

class BlockquoteHandler : public BlockHandler<ASTBlockquote> {
protected:
	
	bool centered;
	char indentStyle = 0;

public:

	BlockquoteHandler() {}

	std::unique_ptr<ParserHandler> createNew() override;

	std::string triggerChars() override;

	bool canHandle(Parser * lex) override;

	std::tuple<std::unique_ptr<_ASTElement>, bool> handle(Parser * lex) override;

	std::unique_ptr<_ASTElement> finish(Parser * lex) override;
};

class UnorderedListHandler : public BlockHandler<ASTListElement> {
protected:

	std::unique_ptr<ASTUnorderedList> list;

public:

	std::unique_ptr<ParserHandler> createNew() override;

	std::string triggerChars() override;

	bool canHandle(Parser * lex) override;

	std::tuple<std::unique_ptr<_ASTElement>, bool> handle(Parser * lex) override;

	std::unique_ptr<_ASTElement> finish(Parser * lex) override;
};

// class BlockquoteHandler : public ParserHandler {
// protected:
	
// 	std::unique_ptr<ASTBlockquote> content = nullptr;
// 	std::unique_ptr<ParserHandler> handler = nullptr;
// 	bool centered;

// public:

// 	BlockquoteHandler() {}

// 	std::unique_ptr<ParserHandler> createNew() override;

// 	std::string triggerChars() override;

// 	bool canHandle(Parser * lex) override;

// 	std::tuple<std::unique_ptr<_ASTElement>, bool> handle(Parser * lex) override;

// 	std::unique_ptr<_ASTElement> finish(Parser * lex) override;
// };

