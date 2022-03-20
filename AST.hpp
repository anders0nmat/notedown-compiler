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
	Holds all elements of a file
*/
class ASTDocument : public _ASTElement {
protected:

	std::vector<std::unique_ptr<_ASTElement>> elements;

	std::string className() {return "ASTDocument";}

public:

	ASTDocument() {}

	ASTDocument(std::vector<std::unique_ptr<_ASTElement>> & elements) {
		addElements(elements);
	}

	void addElement(std::unique_ptr<_ASTElement> & element) {
		if (element != nullptr)
			elements.push_back(std::move(element));
	}

	void addElements(std::vector<std::unique_ptr<_ASTElement>> & list) {
		elements.clear();
		elements.reserve(list.size());
		for (auto & e : list) {
			addElement(e);
		}
	}

	size_t size() {
		return elements.size();
	}

	std::unique_ptr<_ASTElement> & front() {
		return elements.front();
	}

	void prependElement(std::unique_ptr<_ASTElement> & element) {
		elements.insert(elements.begin(), std::move(element));
	}

	std::string toString(std::string prefix) {
		std::string result = prefix + className() + "\n" +
			prefix + "  -elements (" + std::to_string(elements.size()) + "):";
		for (auto & e : elements) {
			result += "\n" + e->toString(prefix + "    ");
		}
		return result;
	}

	std::string toJson() {
		std::string obj = "{\"class\": \"" + className() + "\",";
		obj += "\"elements\": [";

		for (auto & e : elements) {
			obj += e->toJson() + ",";
		}

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
class _ASTInlineElement : public _ASTElement {

	std::string className() {return "_ASTInlineElement";}

public:

};

/*
	Represents Inline Text. Combines multiple ASTInlineElements to allow inline-styling
*/
class ASTInlineText : public _ASTInlineElement {
protected:

	std::vector<std::unique_ptr<_ASTInlineElement>> elements;

	std::string className() {return "ASTInlineText";}

public:

	ASTInlineText() {}

	ASTInlineText(std::vector<std::unique_ptr<_ASTInlineElement>> & elements) {
		addElements(elements);
	}

	ASTInlineText(std::vector<std::unique_ptr<ASTInlineText>> & elements) {
		addElements(elements);
	}

	void addElement(std::unique_ptr<_ASTInlineElement> && element) {
		if (element != nullptr)
			elements.push_back(std::move(element));
	}

	void addElements(std::vector<std::unique_ptr<_ASTInlineElement>> & list) {
		elements.clear();
		elements.reserve(list.size());
		for (auto & e : list) {
			if (e != nullptr)
				elements.push_back(std::move(e));
		}
	}

	void addElements(std::vector<std::unique_ptr<ASTInlineText>> & list) {
		elements.clear();
		elements.reserve(list.size());
		for (auto & e : list) {
			if (e != nullptr)
				elements.push_back(std::move(e));
		}
	}

	size_t size() {
		return elements.size();
	}

	std::unique_ptr<_ASTInlineElement> & front() {
		return elements.front();
	}

	void prependElement(std::unique_ptr<_ASTInlineElement>  element) {
		elements.insert(elements.begin(), std::move(element));
	}

	std::string toString(std::string prefix) {
		std::string result = prefix + className() + "\n" +
			prefix + "  -elements (" + std::to_string(elements.size()) + "):";
		for (auto & e : elements) {
			result += "\n" + e->toString(prefix + "    ");
		}
		return result;
	}

	std::string toJson() {
		std::string obj = "{\"class\": \"" + className() + "\",";
		obj += "\"elements\": [";

		for (auto & e : elements) {
			obj += e->toJson() + ",";
		}

		obj.erase(std::prev(obj.end()));

		obj += "]}";
		return obj;
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


// -------------------------------------- \\ 
// --------- MULTILINE ELEMENTS --------- \\ 
// -------------------------------------- \\ 


/*
	Multiline text template. Everything that can span multiple lines should inherit from this class
*/
class _ASTMultilineElement : public _ASTElement {
protected:

	std::string className() {return "_ASTInlineElement";}

	std::vector<std::unique_ptr<ASTInlineText>> elements;

public:

	_ASTMultilineElement() {}

	_ASTMultilineElement(std::vector<std::unique_ptr<ASTInlineText>> & elements) {
		addElements(elements);
	}

	void addElement(std::unique_ptr<ASTInlineText> && element) {
		if (element != nullptr)
			elements.push_back(std::move(element));
	}

	void addElements(std::vector<std::unique_ptr<ASTInlineText>> & list) {
		elements.clear();
		elements.reserve(list.size());
		for (auto & e : list) {
			addElement(std::move(e));
		}
	}

	size_t size() {
		return elements.size();
	}

	std::string toString(std::string prefix) {
		std::string result = prefix + className() + "\n" +
			prefix + "  -elements (" + std::to_string(elements.size()) + "):";
		for (auto & e : elements) {
			result += "\n" + e->toString(prefix + "    ");
		}
		return result;
	}

	std::string toJson() {
		std::string obj = "{\"class\": \"" + className() + "\",";
		obj += "\"elements\": [";

		for (auto & e : elements) {
			obj += e->toJson() + ",";
		}

		obj.erase(std::prev(obj.end()));

		obj += "]}";
		return obj;
	}
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

	std::string toString(std::string prefix) override {
		return prefix + className() + "\n" +
			prefix + "  -text:" + "\n" + 
			content->toString(prefix + "  ");
	}

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

	ASTHLine() {}

	std::string toString(std::string prefix) override {
		return prefix + className();
	}

	std::string toJson() {
		return "{\"class\": \"" + className() + "\"}";
	}

};


/*
	Represents a Paragraph
*/
class ASTParagraph : public _ASTMultilineElement {
protected:

	std::string className() {return "ASTParagraph";}

public:


};

/*
	Represents a Blockquote. Everything should be blockquotable
	TODO : To make everything blockquotable, files must be parsable by line
*/
class ASTBlockquote : public ASTDocument {
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

		obj.erase(std::prev(obj.end()));

		obj += "]}";
		return obj;
	}
};
