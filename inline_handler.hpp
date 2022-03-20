#pragma once
#include "lexer.hpp"

template<char indicator>
class InlineTemplateHandler : public InlineHandler {
protected:

public:

	InlineTemplateHandler() {}

	std::unique_ptr<InlineHandler> createNew() override;

	std::string triggerChars() override;

	bool canHandle(Parser * lex) override;

	std::tuple<std::unique_ptr<_ASTInlineElement>, bool> handle(Parser * lex) override;

	std::unique_ptr<_ASTElement> finish(Parser * lex) override;
};

