#pragma once

#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <functional>

enum ASTProcess {
	procRegister, // Elements get the chance to register themselves to their parent
	procResolve, // Elements resolve their commands, which processes dependencies
	procConsume, // Elements search for empty Childs and aquire their commands (if possible)
	procExecutePrep, // Elements execute their command functions
	procExecuteMain, // Elements execute their command functions
	procExecutePost, // Elements execute their command functions
};

class _ASTElement;
class ASTIdDefinition;
class ASTCommandContainer;

typedef std::function<void(_ASTElement *, ASTProcess, std::vector<std::string> &)> ASTModFunc;
typedef std::function<_ASTElement*(std::string)> ASTRequestFunc;
typedef std::function<ASTModFunc(std::string)> ASTRequestModFunc;

/*
	Holding information of HTML attributes:
	- Id
	- Class
	- Title
	- Style
	- Other Attributes
	Additionally, it holds references to other Commands which are included
	at translation time.
	It also manages functions to execute before translation
	and flags for operation of these function
*/
class ASTCommand {
protected:
	virtual std::string className() {return "ASTCommand";}

	std::vector<std::string> splitSnippets(std::string str, char c);
	std::vector<std::string> splitAt(std::string str, char c);
	std::pair<std::string, std::string> apartAt(std::string str, char c);

	bool strisalnum(std::string str);
	bool strisiden(std::string str);
public:
	std::string refName;
	std::string id;
	std::unordered_set<std::string> classes;
	std::string title;
	std::unordered_map<std::string, std::string> attributes;
	std::unordered_map<std::string, std::string> css;
	std::pair<std::string, std::vector<std::string>> genFunction;
	std::vector<std::pair<std::string, std::vector<std::string>>> modFunctions;
	std::vector<std::string> refCommands;

	std::unordered_map<std::string, std::string> flags;

	ASTCommand() {}
	ASTCommand(std::string command);

	void addClass(std::string className);

	virtual std::string toJson();

	/*
		Merges other into this element without consuming it
	*/
	virtual void merge(ASTCommand & other);
	virtual void merge(ASTCommand && other);

	/*
		Same as merge but other has lower precedence
	*/
	virtual void integrate(ASTCommand && other);
	virtual void integrate(ASTCommand & other);

	virtual void resolve(ASTRequestFunc request);
	virtual std::string constructHeader(ASTRequestFunc request);

	virtual void execute(_ASTElement * caller, ASTProcess step, ASTRequestModFunc modFunc);
};


// -------------------------------------- //
// ------------- TEMPLATES -------------- //
// -------------------------------------- //


/*
	Base Class for entire AST.
	Introduces:
		getHTML()
		~ASTElement()
		toString()
		className()
*/
class _ASTElement {
protected:
	virtual std::string className() {return "_ASTElement";}

	std::string cmdJson();

	virtual void _consume(ASTProcess step, ASTRequestFunc request, ASTRequestModFunc modFunc);
	virtual void _register(ASTProcess step, ASTRequestFunc request, ASTRequestModFunc modFunc);
	virtual void _resolve(ASTProcess step, ASTRequestFunc request, ASTRequestModFunc modFunc);

	virtual void _execute(ASTProcess step, ASTRequestFunc request, ASTRequestModFunc modFunc);
public:
	ASTCommand commands;
	_ASTElement * parent = nullptr;

	virtual _ASTElement * getDocument();
	virtual _ASTElement * containingElement();
	virtual void registerNow() {}

	virtual void process(ASTProcess step, ASTRequestFunc request, ASTRequestModFunc modFunc);
	
	virtual ~_ASTElement() {}

	virtual std::string toJson();

	virtual bool isEmpty();
	virtual bool canConsume();

	virtual void addCommand(ASTCommand && command);
	virtual void addCommand(ASTCommand & command);

	virtual std::string getHtml(ASTRequestFunc request);
};

/*
	Container for list-based objects
*/
template<class cl>
class _ASTListElement : virtual public _ASTElement {
protected:
	std::string className() {return "_ASTListElement";}

	void _consume(ASTProcess step, ASTRequestFunc request, ASTRequestModFunc modFunc) override {
		for (auto & e : elements) {
			if (e->canConsume())
				commands.integrate(e->commands);
		}
	}

public:
	std::vector<std::unique_ptr<cl>> elements;

	void process(ASTProcess step, ASTRequestFunc request, ASTRequestModFunc modFunc) override {
		for (auto & e : elements)
			e->process(step, request, modFunc);
		_ASTElement::process(step, request, modFunc);
	}

	void registerNow() override {
		for (auto & e : elements)
			e->registerNow();
	}
	
	bool isEmpty() override {
		for (auto & e : elements)
			if (!e->isEmpty())
				return false;
		return true;
	}

	virtual void addElement(std::unique_ptr<cl> & element) {
		if (element != nullptr) {
			element->parent = this;
			elements.push_back(std::move(element));
		}
	}

	virtual void addElement(std::unique_ptr<cl> && element) {
		if (element != nullptr) {
			element->parent = this;
			elements.push_back(std::move(element));
		}
	}

	virtual void addElements(std::vector<std::unique_ptr<cl>> & elements) {
		for (auto & e : elements)
			addElement(e);
	}

	virtual void addElements(std::vector<std::unique_ptr<cl>> && elements) {
		for (auto & e : elements)
			addElement(e);
	}

	virtual void prependElement(std::unique_ptr<cl> & element) {
		if (element != nullptr) {
			element->parent = this;
			elements.insert(elements.begin(), std::move(element));
		}
	}

	virtual void prependElement(std::unique_ptr<cl> && element) {
		if (element != nullptr) {
			element->parent = this;
			elements.insert(elements.begin(), std::move(element));
		}
	}


	virtual size_t size() {
		return elements.size();
	}

	virtual std::unique_ptr<cl> & front() {
		return elements.front();
	}

	virtual std::unique_ptr<cl> & back() {
		return elements.back();
	}

	std::string toJson() override {
		std::string obj = "{\"class\": \"" + className() + "\",";
		obj += "\"elements\": [";

		for (auto & e : elements) {
			obj += e->toJson() + ",";
		}

		if (elements.size() != 0)
			obj.erase(std::prev(obj.end()));

		obj += "],";
		obj += cmdJson();
		obj += "}";
		return obj;
	}

	std::string getHtml(ASTRequestFunc request) override {
		std::string html;
		for (auto & e : elements)
			if (!e->isEmpty())
				html += e->getHtml(request) + "\n";
		return html;
	}
};

/*
	Base for all document-like elements, so multiline, multi-element structures
*/
typedef _ASTListElement<_ASTElement> _ASTBlockElement;

/*
	Inline text template. Everything that can occure in plain Text should inherit from this class
*/
class _ASTInlineElement : virtual public _ASTElement {
protected:
	std::string className() {return "_ASTInlineElement";}

	void _consume(ASTProcess step, ASTRequestFunc request, ASTRequestModFunc modFunc) override;
public:
	_ASTElement * containingElement() override;

	virtual std::string literalText();
};


// -------------------------------------- //
// ---------- INLINE ELEMENTS ----------- //
// -------------------------------------- //


class ASTCommandContainer : public _ASTInlineElement {
protected:
	std::string className() {return "ASTCommandContainer";}

	std::unique_ptr<_ASTElement> content;

	void _register(ASTProcess step, ASTRequestFunc request, ASTRequestModFunc modFunc) override;
public:
	void setContent(std::unique_ptr<_ASTElement> & element);
	void setContent(std::unique_ptr<_ASTElement> && element);

	bool isEmpty() override;

	std::string getHtml(ASTRequestFunc request) override;
};

/*
	Represents Inline Text. Combines multiple ASTInlineElements to allow inline-styling
*/
class ASTInlineText : public _ASTInlineElement, public _ASTListElement<_ASTInlineElement> {
protected:
	std::string className() {return "ASTInlineText";}

	void _consume(ASTProcess step, ASTRequestFunc request, ASTRequestModFunc modFunc) override;
public:
	_ASTElement * containingElement() override;

	std::string literalText() override;

	std::string toJson() override;

	std::string getHtml(std::function<_ASTElement*(std::string)> request) override;
};

/*
	Plain Text element. Text will be rendered as-is
*/
class ASTPlainText : public _ASTInlineElement {
protected:
	std::string className() {return "ASTPlainText";}
public:
	std::string content;

	ASTPlainText() {}
	ASTPlainText(const std::string & content) : content(content) {}
	ASTPlainText(int chr) : content(1, chr) {}
	ASTPlainText(int count, int chr) : content(count, chr) {}

	std::string literalText() override;

	bool isEmpty() override;

	std::string toJson() override;

	std::string getHtml(ASTRequestFunc request) override;
};

/*
	Represents a forced Linebreak, indicated by <Space><Space><Linebreak>
*/
class ASTLinebreak : public _ASTInlineElement {
protected:
	std::string className() {return "ASTLinebreak";}
public:
	ASTLinebreak() {}

	bool isEmpty() override;

	std::string getHtml(ASTRequestFunc request) override;
};

/*
	Represents all text modifications with syntax <symbol><text><symbol>
*/
class ASTTextModification : public _ASTInlineElement {
protected:
	std::string className() {return "ASTTextModification";}

	char symbol = 0;
	std::unique_ptr<_ASTInlineElement> content;
public:
	ASTTextModification(char symbol) : symbol(symbol) {}
	ASTTextModification(char symbol, std::unique_ptr<_ASTInlineElement> element) 
	: symbol(symbol), content(std::move(element)) {
		content->parent = this;
	}

	std::string literalText() override;

	bool isEmpty() override;

	std::string toJson();

	std::string getHtml(ASTRequestFunc request) override;
};

/*
	Represents an inline emoji. Contains shortcode and converts it via conversion table
*/
class ASTEmoji : public _ASTInlineElement {
protected:
	std::string className() {return "ASTEmoji";}

	std::string shortcode;
public:

	ASTEmoji(const std::string & shortcode) : shortcode(shortcode) {}

	bool isEmpty() override;

	std::string toJson();

	std::string getHtml(ASTRequestFunc request) override;
};

/*
	Represents everything that is enclosed in square brackets
*/
class ASTModifier : public _ASTInlineElement {
protected:
	std::string className() {return "ASTModifier";}
public:
	std::string url;
	std::unique_ptr<ASTInlineText> content;

	ASTModifier(std::string & url, std::unique_ptr<ASTInlineText> & content)
	: url(url), content(std::move(content)) {
		this->content->parent = this;
	}

	std::string literalText() override;

	bool isEmpty() override;

	std::string toJson();
};

/*
	Represents Links with <a> element
*/
class ASTLink : public ASTModifier {
protected:
	std::string className() {return "ASTLink";}

	void _resolve(ASTProcess step, ASTRequestFunc request, ASTRequestModFunc modFunc) override;
public:
	using ASTModifier::ASTModifier;

	std::string getHtml(ASTRequestFunc request) override;
};

/*
	Represents Images
*/
class ASTImage : public ASTModifier {
protected:	
	std::string className() {return "ASTImage";}

	void _resolve(ASTProcess step, ASTRequestFunc request, ASTRequestModFunc modFunc) override;
public:
	using ASTModifier::ASTModifier;

	bool isEmpty() override;

	std::string getHtml(ASTRequestFunc request) override;
};

/*
	Represents Footnotes
*/
class ASTFootnote : public ASTModifier {
protected:
	std::string className() {return "ASTFootnote";}

	void _resolve(ASTProcess step, ASTRequestFunc request, ASTRequestModFunc modFunc) override;
public:
	using ASTModifier::ASTModifier;

	std::string getHtml(ASTRequestFunc request) override;
};

/*
	Represents Heading Links
*/
class ASTHeadingLink : public ASTModifier {
protected:
	std::string className() {return "ASTHeadingLink";}
public:
	using ASTModifier::ASTModifier;

	std::string getHtml(ASTRequestFunc request) override;
};

/*
	Represents Replaced Content
*/
class ASTReplace : public ASTModifier {
protected:
	std::string className() {return "ASTReplace";}
public:
	using ASTModifier::ASTModifier;

	bool isEmpty() override;

	std::string getHtml(ASTRequestFunc request) override;
};

/*
	Represents styled Content
*/
class ASTStyled : public ASTModifier {
protected:
	std::string className() {return "ASTStyled";}
public:
	using ASTModifier::ASTModifier;

	std::string getHtml(ASTRequestFunc request) override;
};


// -------------------------------------- //
// --------- MULTILINE ELEMENTS --------- //
// -------------------------------------- //


/*
	Holds all elements of a file
*/
class ASTDocument : public _ASTBlockElement {
protected:
	std::string className() {return "ASTDocument";}
public:
	std::unordered_map<std::string, _ASTElement *> iddef;

	_ASTElement * getDocument() override;
};

/*
	Holds definitions for ids
*/
class ASTIdDefinition : public _ASTBlockElement {
protected:
	std::string className() {return "ASTIdDefinition";}

	void _register(ASTProcess step, ASTRequestFunc request, ASTRequestModFunc modFunc) override;
public:
	std::string id;
	std::string url;
	char type;

	ASTIdDefinition(std::string id, std::string url, char type) : id(id), url(url), type(type) {}

	bool isEmpty() override;
	bool canConsume() override;

	void registerNow();

	std::string toJson() override;

	std::string getHtml(ASTRequestFunc request) override;
};

/*
	Represents Headings
*/
class ASTHeading : public _ASTElement {
protected:
	std::string className() override {return "ASTHeading";}

	void _consume(ASTProcess step, ASTRequestFunc request, ASTRequestModFunc modFunc) override;
public:
	int level = 0;
	std::unique_ptr<ASTInlineText> content;

	ASTHeading(int level, std::unique_ptr<ASTInlineText> content) : level(level), content(std::move(content)) {
		this->content->parent = this;
	}

	void process(ASTProcess step, ASTRequestFunc request, ASTRequestModFunc modFunc) override;

	bool isEmpty() override;

	std::string toJson();

	std::string getHtml(ASTRequestFunc request) override;
};

/*
	Represents HLine
*/
class ASTHLine : public _ASTElement {
protected:
	std::string className() override {return "ASTHLine";}
public:
	bool isEmpty() override;

	std::string getHtml(ASTRequestFunc request) override;
};

/*
	Represents a Paragraph
*/
class ASTParagraph : public _ASTBlockElement {
protected:
	std::string className() {return "ASTParagraph";}
public:
	std::string getHtml(ASTRequestFunc request) override;
};

/*
	Represents a Blockquote. Everything should be blockquotable
*/
class ASTBlockquote : public _ASTBlockElement {
protected:
	std::string className() {return "ASTBlockquote";}
public:
	bool centered;

	ASTBlockquote() {}
	ASTBlockquote(bool centered) : centered(centered) {}

	std::string toJson() override;

	std::string getHtml(ASTRequestFunc request) override;
};

/*
	Represents Unordered and Ordered List Element
*/
class ASTListElement : public _ASTBlockElement {
protected:
	std::string className() {return "ASTListElement";}
public:
	unsigned long index;

	ASTListElement(unsigned long index = 0) : index(index) {}

	std::string toJson() override;

	std::string getHtml(ASTRequestFunc request) override;
};

/*
	Represents Unordered Lists
*/
class ASTUnorderedList : public _ASTListElement<ASTListElement> {
protected:
	std::string className() {return "ASTUnorderedList";}
public:
	std::string getHtml(ASTRequestFunc request) override;
};

/*
	Represents Ordered Lists
*/
class ASTOrderedList : public _ASTListElement<ASTListElement> {
protected:
	std::string className() {return "ASTOrderedList";}
public:
	std::string getHtml(ASTRequestFunc request) override;
};

/*
	Holds code text
*/
class ASTCodeBlock : public _ASTBlockElement {
protected:
	std::string className() {return "ASTCodeBlock";}
public:
	std::string lang;
	std::unique_ptr<ASTInlineText> command;

	ASTCodeBlock(std::string lang) : lang(lang) {}

	void addCommand(std::unique_ptr<ASTInlineText> & e);

	std::string toJson() override;

	std::string getHtml(ASTRequestFunc request) override;
};

/*
	Represents a Info Block. Everything should be able to fit in this object
*/
class ASTInfoBlock : public _ASTBlockElement {
protected:
	std::string className() {return "ASTInfoBlock";}
public:
	std::string type;
	bool sym;

	ASTInfoBlock() {}
	ASTInfoBlock(std::string type, bool sym = false) : type(type), sym(sym) {}

	std::string toJson() override;

	std::string getHtml(ASTRequestFunc request) override;
};

/*
	Represents footnote definition
*/
class ASTFootnoteBlock : public _ASTBlockElement {
protected:
	std::string className() {return "ASTFootnoteBlock";}

	
	void _register(ASTProcess step, ASTRequestFunc request, ASTRequestModFunc modFunc) override;
public:
	std::string id;

	ASTFootnoteBlock(std::string id) : id(id) {}

	std::string toJson() override;

	std::string getHtml(ASTRequestFunc request) override;
};

/*
	Contains a collapsible section
*/
class ASTCollapseBlock : public _ASTBlockElement {
protected:
	std::string className() {return "ASTCollapseBlock";}
public:
	std::unique_ptr<ASTInlineText> summary;
	bool isOpen;

	ASTCollapseBlock() {}
	ASTCollapseBlock(bool isOpen, std::unique_ptr<ASTInlineText> summary) : isOpen(isOpen), summary(std::move(summary)) {
		this->summary->parent = this;
	}

	void process(ASTProcess step, ASTRequestFunc request, ASTRequestModFunc modFunc) override;

	std::string toJson() override;

	std::string getHtml(ASTRequestFunc request) override;
};
