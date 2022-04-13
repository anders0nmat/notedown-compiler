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

	/**
	 * Creates new Instance of this class.
	 * Should be overwritten in every child
	 * @return unique_ptr with new instance of class
	 */
	virtual std::unique_ptr<InlineHandler> createNew();

	/**
	 * @return String with chars that have to be treated specially (tokenized as tokSym)
	 */
	virtual std::string triggerChars();

	/**
	 * @param lex Pointer to Parser with current situation
	 * @return Whether this handler can handle the current situation
	 */
	virtual bool canHandle(Parser * lex);

	/**
	 * @param lex Pointer to Parser
	 * @return Tuple containing object to insert (nullptr valid) and whether the handler finished (should always be true)
	 */
	virtual std::tuple<std::unique_ptr<_ASTInlineElement>, bool> handle(Parser * lex);

	/**
	 * @param lex Pointer to Parser
	 * @return Object to insert (nullptr valid)
	 */
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

	/**
	 * Creates new Instance of this class.
	 * Should be overwritten in every child
	 * @return unique_ptr with new instance of class
	 */
	virtual std::unique_ptr<ParserHandler> createNew();

	/**
	 * @return String with chars that have to be treated specially (tokenized as tokSym)
	 */
	virtual std::string triggerChars();

	/**
	 * @param lex Pointer to Parser with current situation
	 * @return Whether this handler can handle the current situation
	 */
	virtual bool canHandle(Parser * lex);

	/**
	 * @param lex Pointer to Parser
	 * @return Tuple containing object to insert (nullptr valid) and whether the handler finished
	 */
	virtual std::tuple<std::unique_ptr<_ASTElement>, bool> handle(Parser * lex);

	/**
	 * @param lex Pointer to Parser
	 * @return Object to insert (nullptr valid)
	 */
	virtual std::unique_ptr<_ASTElement> finish(Parser * lex);
};
