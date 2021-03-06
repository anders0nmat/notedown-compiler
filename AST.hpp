#include <string>
#include <vector>
#include <memory>

// -------------------------------------- \\ 
// ------------- TEMPLATES -------------- \\ 
// -------------------------------------- \\ 

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

public:

	virtual ~_ASTElement() {}

	virtual std::string toString(std::string prefix) {
		return prefix + className();
	}

	virtual std::string toJson() {
		return "{\"class\": \"" + className() + "\"}";
	}
};

/*
	Container for list-based objects
*/
template<class cl>
class _ASTListElement : virtual public _ASTElement {
protected:

	std::vector<std::unique_ptr<cl>> elements;

	std::string className() {return "_ASTListElement";}

public:

	virtual void addElement(std::unique_ptr<cl> & element) {
		if (element != nullptr)
			elements.push_back(std::move(element));
	}

	virtual void prependElement(std::unique_ptr<cl> & element) {
		if (element != nullptr)
			elements.insert(elements.begin(), std::move(element));
	}

	virtual void addElement(std::unique_ptr<cl> && element) {
		if (element != nullptr)
			elements.push_back(std::move(element));
	}

	virtual void addElements(std::vector<std::unique_ptr<cl>> && elements) {
		for (auto & e : elements) {
			addElement(e);
		}
	}

	virtual void prependElement(std::unique_ptr<cl> && element) {
		if (element != nullptr)
			elements.insert(elements.begin(), std::move(element));
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

		obj += "]}";
		return obj;
	}

};


// -------------------------------------- \\ 
// ---------- INLINE ELEMENTS ----------- \\ 
// -------------------------------------- \\ 


/*
	Inline text template. Everything that can occure in plain Text should inherit from this class
*/
class _ASTInlineElement : virtual public _ASTElement {

	std::string className() {return "_ASTInlineElement";}

public:

	virtual std::string literalText() {
		return "";
	}

};

/*
	Represents Inline Text. Combines multiple ASTInlineElements to allow inline-styling
*/
class ASTInlineText : public _ASTInlineElement, public _ASTListElement<_ASTInlineElement> {
protected:

	std::string className() {return "ASTInlineText";}

public:

	std::string literalText() override {
		std::string res;
		for (auto & e : elements)
			res += e->literalText();
		return res;
	}

	std::string toJson() override {
		return _ASTListElement<_ASTInlineElement>::toJson();
	}
};

/*
	Plain Text element. Text will be rendered as-is
*/
class ASTPlainText : public _ASTInlineElement {
protected:

	std::string content;

	std::string className() {return "ASTPlainText";}

public:

	ASTPlainText(const std::string & content) : content(content) {}

	ASTPlainText(int chr) : content(1, chr) {}

	ASTPlainText(int count, int chr) : content(count, chr) {}

	std::string literalText() override {
		return content;
	}

	std::string toString(std::string prefix) {
		return prefix + className() + "\n" + 
			prefix + "  -content: \"" + content + "\"";
	}

	std::string toJson() {
		std::string obj = "{\"class\": \"" + className() + "\",";
		obj += "\"content\": \"";

		obj += content;

		obj += "\"}";
		return obj;
	}
};

/*
	Represents a forced Linebreak, indicated by <Space><Space><Linebreak>
*/
class ASTLinebreak : public _ASTInlineElement {
protected:

	std::string className() {return "ASTLinebreak";}

public:

	ASTLinebreak() {}

};

/*
	Represents all text modifications with syntax <symbol><text><symbol>
*/
class ASTTextModification : public _ASTInlineElement {
protected:

	char symbol = 0;

	std::unique_ptr<_ASTInlineElement> content;

	std::string className() {return "ASTTextModification";}

public:

	ASTTextModification(char symbol) : symbol(symbol) {}

	ASTTextModification(char symbol, std::unique_ptr<_ASTInlineElement> element) 
	: symbol(symbol), content(std::move(element)) {}

	std::string literalText() override {
		return content->literalText();
	}

	std::string toString(std::string prefix) {
		return prefix + className() + "\n" +
			prefix + "  -symbol: " + symbol + "\n" +
			content->toString(prefix + "  ");
	}

	std::string toJson() {
		std::string obj = "{\"class\": \"" + className() + "\",";
		obj += "\"symbol\": \"";
		obj += symbol;
		obj += "\","; 
		obj += "\"content\": ";

		obj += content->toJson();
		
		obj += "}";
		return obj;
	}

};

/*
	Represents an inline emoji. Contains shortcode and converts it via conversion table
*/
class ASTEmoji : public _ASTInlineElement {
protected:

	std::string shortcode;

	std::string className() {return "ASTEmoji";}

public:

	ASTEmoji(const std::string & shortcode) : shortcode(shortcode) {}

	std::string toJson() {
		std::string obj = "{\"class\": \"" + className() + "\",";
		obj += "\"shortcode\": \"";

		obj += shortcode;

		obj += "\"}";
		return obj;
	}

};

/*
	Represents everything that is enclosed in square brackets
*/
class ASTModifier : public _ASTInlineElement {
protected:

	std::string command;

	std::string url;

	int type = 0;
	
	std::unique_ptr<ASTInlineText> content;

	std::string className() {return "ASTModifier";}

public:

	ASTModifier(int type, std::string url, std::string command, std::unique_ptr<ASTInlineText> content)
		: type(type), url(url), command(command), content(std::move(content)) {}

	std::string literalText() override {
		return content->literalText();
	}

	std::string toJson() {
		std::string obj = "{\"class\": \"" + className() + "\",";
		obj += "\"type\": \"";
		obj += type;
		obj += "\",";

		obj += "\"url\": \"" + url + "\",";
		obj += "\"command\": \"" + command + "\",";

		obj += "\"content\":";
		obj += content->toJson();

		obj += "}";
		return obj;
	}

};


// -------------------------------------- \\ 
// --------- MULTILINE ELEMENTS --------- \\ 
// -------------------------------------- \\ 


/*
	Base for all document-like elements, so multiline, multi-element structures
*/
typedef _ASTListElement<_ASTElement> _ASTBlockElement;

/*
	Holds all elements of a file
*/
class ASTDocument : public _ASTBlockElement {
protected:

	std::string className() {return "ASTDocument";}

public:

};

/*
	Represents Headings
*/
class ASTHeading : public _ASTElement {
protected:

	int level = 0;

	std::unique_ptr<ASTInlineText> content;

	std::string className() override {return "ASTHeading";}

public:

	ASTHeading(int level, std::unique_ptr<ASTInlineText> content) : level(level), content(std::move(content)) {}

	std::string toJson() {
		std::string obj = "{\"class\": \"" + className() + "\",";
		obj += "\"level\": " + std::to_string(level) + ",";
		obj += "\"text\": ";

		obj += content->toJson();

		obj += "}";
		return obj;
	}

};

/*
	Represents HLine
*/
class ASTHLine : public _ASTElement {
protected:

	std::string className() override {return "ASTHLine";}

public:

};

/*
	Represents a Paragraph
*/
class ASTParagraph : public _ASTBlockElement {
protected:

	std::string className() {return "ASTParagraph";}

public:

};

/*
	Represents a Blockquote. Everything should be blockquotable
*/
class ASTBlockquote : public _ASTBlockElement {
protected:

	std::string className() {return "ASTBlockquote";}

	bool centered;

public:

	ASTBlockquote() {}

	ASTBlockquote(bool centered) : centered(centered) {}

	std::string toJson() override {
		std::string obj = "{\"class\": \"" + className() + "\",";
		obj += "\"centered\": " + std::to_string(centered) + ",";
		obj += "\"elements\": [";

		for (auto & e : elements) {
			obj += e->toJson() + ",";
		}

		if (elements.size() != 0)
			obj.erase(std::prev(obj.end()));

		obj += "]}";
		return obj;
	}
};

/*
	Represents Unordered and Ordered List Element
*/
class ASTListElement : public _ASTBlockElement {
protected:

	std::string className() {return "ASTListElement";}

	unsigned long index;

public:

	ASTListElement(unsigned long index = 0) : index(index) {}

	std::string toJson() override {
		std::string obj = "{\"class\": \"" + className() + "\",";
		obj += "\"index\": " + std::to_string(index) + ",";
		obj += "\"elements\": [";

		for (auto & e : elements) {
			obj += e->toJson() + ",";
		}
		if (elements.size() != 0)
			obj.erase(std::prev(obj.end()));

		obj += "]}";
		return obj;
	}
};

/*
	Represents Unordered Lists
*/
class ASTUnorderedList : public _ASTListElement<ASTListElement> {
protected:

	std::string className() {return "ASTUnorderedList";}

public:

};

/*
	Represents Ordered Lists
*/
class ASTOrderedList : public _ASTListElement<ASTListElement> {
protected:

	std::string className() {return "ASTOrderedList";}

public:

};

/*
	Holds code text
*/
class ASTCodeBlock : public _ASTBlockElement {
protected:

	std::string lang;

	std::unique_ptr<ASTInlineText> command;

	std::string className() {return "ASTCodeBlock";}

public:

	ASTCodeBlock(std::string lang) : lang(lang) {}

	void addCommand(std::unique_ptr<ASTInlineText> & e) {
		command = std::move(e);
	}

	std::string toJson() override {
		std::string obj = "{\"class\": \"" + className() + "\",";
		obj += "\"lang\": \"" + lang + "\",";
		obj += "\"elements\": [";

		for (auto & e : elements) {
			obj += e->toJson() + ",";
		}

		if (elements.size() != 0)
			obj.erase(std::prev(obj.end()));

		obj += "]}";
		return obj;
	}

};
