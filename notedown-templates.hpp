#pragma once

class Parser;

/*
	Template Class for Inline Parser Hooks
*/
class InlineHandler {
protected:

public:

	InlineHandler() {}

	virtual ~InlineHandler() {}

	virtual std::unique_ptr<InlineHandler> createNew();

	virtual std::string triggerChars();

	virtual bool canHandle(Parser * lex);

	virtual std::tuple<std::unique_ptr<_ASTInlineElement>, bool> handle(Parser * lex);

	virtual std::unique_ptr<_ASTElement> finish(Parser * lex);
};

/*
	Template Class for Parser Hooks
*/
class ParserHandler {
protected:

public:

	ParserHandler() {}

	virtual ~ParserHandler() {}

	virtual std::unique_ptr<ParserHandler> createNew();

	virtual std::string triggerChars();

	virtual bool canHandle(Parser * lex);

	virtual std::tuple<std::unique_ptr<_ASTElement>, bool> handle(Parser * lex);

	virtual std::unique_ptr<_ASTElement> finish(Parser * lex);
};
