#pragma once

class Parser;

enum Token : int {
	
	tokEOF = -1, // End of File
	tokText = -2, // lastString contains text
	tokNumber = -3, // lastInt contains number, lastString contains string read (reads <Number>.)
	tokSpace = -4, // lastString contains ' ', lastInt contains amount of spaces
	tokNewline = -5, // Newline Character read (\n)
	tokSym = -6, // Indicates formatting Symbol, lastString contains it, lastInt contains amount
	_tokEscape = -7, // Used internally for escape sequences. Will never be the state of lastToken after gettok()
};

struct TimeSnap {
	long long int pos = -1;

	int _lastChar = -1;

	std::string lastString = "";
	int lastInt = -1;
	Token lastToken = tokEOF;
};

namespace Notedown::Handler {
	typedef std::tuple<std::unique_ptr<_ASTElement>, bool, bool> HandlerReturn;
	typedef std::tuple<std::unique_ptr<_ASTInlineElement>, bool, bool> InlineHandlerReturn;

	template<typename T>
	constexpr std::tuple<T, bool, bool> make_result(T & e, bool s) {
		return std::make_tuple(std::move(e), s, false);
	}

	template<typename T>
	constexpr std::tuple<T, bool, bool> make_result(T && e, bool s) {
		return std::make_tuple(std::move(e), s, false);
	}

	/*
		Returned by a Handler.
		Indicates that the Handler was not able to construct a valid AST from input.
		The Caller is supposed to roll back and continue with an other handler.
	*/
	constexpr auto handler_error = std::make_tuple(nullptr, false, true);

	/*
		Returned by a Handler.
		Indicates that the Handler is not finished and has nothing to insert into the AST right now.
	*/
	constexpr auto handler_continue = make_result(nullptr, false);
}

/*
	Template Class for Inline Parser Hooks
*/
class InlineHandler {
private:
	friend class Parser;
	friend class NotedownCompiler;
	TimeSnap snap;
	int id = -1;
protected:

	void inheritId(std::unique_ptr<InlineHandler> & other);

	template<class T>
	std::unique_ptr<InlineHandler> newSelf() {
		std::unique_ptr<InlineHandler> p = std::make_unique<T>();
		inheritId(p);
		return p;
	}

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
	virtual Notedown::Handler::InlineHandlerReturn handle(Parser * lex);

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
private:
	friend class Parser;
	friend class NotedownCompiler;
	TimeSnap snap;
	int id = -1;
protected:

	void inheritId(std::unique_ptr<ParserHandler> & other);

	template<class T>
	std::unique_ptr<ParserHandler> newSelf() {
		std::unique_ptr<ParserHandler> p = std::make_unique<T>();
		inheritId(p);
		return p;
	}

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
	virtual Notedown::Handler::HandlerReturn handle(Parser * lex);

	/**
	 * @param lex Pointer to Parser
	 * @return Object to insert (nullptr valid)
	 */
	virtual std::unique_ptr<_ASTElement> finish(Parser * lex);
};
