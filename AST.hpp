#include <string>
#include <vector>
#include <memory>
#include <unordered_map>

/*
	Class holding all kinds of commands
*/
class ASTCommand {
protected:

	std::string id;
	std::string classes;
	std::string title;
	std::vector<std::pair<std::string, std::string>> attributes;
	std::string css;
	std::vector<std::pair<std::string, std::vector<std::string>>> functions;

	std::unordered_map<std::string, std::string> flags;

	std::vector<std::string> splitSnippets(std::string str, char c) {
		std::vector<std::string> result;
		std::string s;
		bool block = false;
		for (auto e : str) {
			if (!block && e == c) {
				if (!s.empty())
					result.push_back(s);
				s = "";
			}
			else if (e == '"') {
				block = !block;
			}
			else
				s += e;
		}
		if (!s.empty())
			result.push_back(s);
		return result;
	}

	std::vector<std::string> splitAt(std::string str, char c) {
		std::vector<std::string> result;
		std::string s;
		for (auto e : str) {
			if (e == c) {
				if (!s.empty())
					result.push_back(s);
				s = "";
			}
			else
				s += e;
		}
		if (!s.empty())
			result.push_back(s);
		return result;
	}

	std::pair<std::string, std::string> apartAt(std::string str, char c) {
		size_t pos = str.find_first_of(c);
		if (pos == std::string::npos)
			return std::make_pair(str, "");
		return std::make_pair(str.substr(0, pos), str.substr(pos + 1));
	}

	bool strisalnum(std::string str) {
		for (auto e : str)
			if (!isalnum(e))
				return false;
		return true;
	}

	bool strisiden(std::string str) {
		for (auto e : str)
			if (!isalnum(e) && e != '-' && e != '_')
				return false;
		return true;
	}

	virtual std::string className() {return "ASTCommand";}

public:

	ASTCommand() {}

	ASTCommand(std::string command) {
		std::vector<std::string> snippets(splitSnippets(command, ' '));
		for (auto & e : snippets) {
			if (e[0] == '#' && e.length() > 1) {
				// ID Setter
				std::string newId = e.substr(1);
				if (strisiden(newId))
					id = newId;
				continue;
			}
			if (e[0] == '.' && e.length() > 1) {
				std::vector cls(splitAt(e, '.'));
				for (auto & c : cls) {
					if (strisiden(c))
						classes += (classes.empty() ? "" : " ") + c;
				}
				continue;
			}
			if (e[0] == ':' && e.length() > 1) {
				std::string desc = e.substr(1);
				if (desc[0] == '"')
					title = desc.substr(1, desc.length() - 2);
				else
					title = desc;
				continue;
			}
			if (e[0] == '+' && e.length() > 1) {
				auto fields = apartAt(e.substr(1), '=');
				if (strisiden(fields.first))
					attributes.push_back(fields);
				continue;
			}
			if (e[0] == '$' && e.length() > 1) {
				auto funcarg = apartAt(e.substr(1), ':');
				if (strisiden(funcarg.first)) {
					std::pair<std::string, std::vector<std::string>> f;
					f.first = funcarg.first;
					if (!funcarg.second.empty()) {
						auto args = splitSnippets(funcarg.second, ',');
						for (auto & arg : args) {
							if (arg[0] == '"')
								f.second.push_back(arg.substr(1, arg.length() - 2));
							else
								f.second.push_back(arg);
						}
						functions.push_back(f);
					}
				}
				
				continue;
			}
			if (e[0] == '>' && e.length() > 4) {
				auto funcarg = apartAt(e.substr(1), ':');
				if (!funcarg.second.empty() && strisiden(funcarg.first)) {
					if (funcarg.second[0] == '"') {
						funcarg.second = funcarg.second.substr(1, funcarg.second.length() - 2);
					}
					css.append(funcarg.first + ": " + funcarg.second + "; ");
				}
				continue;
			}
		}
	}

	virtual std::string toJson() {
		std::string obj = "{\"class\": \"" + className() + "\",";
		obj += "\"id\": \"" + id + "\",";
		obj += "\"classes\": \"" + classes + "\",";
		obj += "\"title\": \"" + title + "\",";
		obj += "\"css\": \"" + css + "\",";

		obj += "\"attributes\": {";

		for (auto & e : attributes) {
			obj += "\"" + e.first + "\": \"" + e.second + "\",";
		}		
		
		if (attributes.size() != 0)
			obj.erase(std::prev(obj.end()));

		obj += "},";

		obj += "\"functions\": {";

		for (auto & e : functions) {
			obj += "\"" + e.first + "\": [";
			for (auto arg : e.second) {
				obj += "\"" + arg + "\",";
			}
			if (e.second.size() != 0)
				obj.erase(std::prev(obj.end()));
			obj += "],";
		}		
		
		if (attributes.size() != 0)
			obj.erase(std::prev(obj.end()));

		obj += "}";
		
		
		obj += "}";
		return obj;
	}

	void execute() {}
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

	std::unique_ptr<ASTCommand> commands;

	std::unordered_map<std::string, std::string> attributes;


	virtual std::string className() {return "_ASTElement";}

	std::string cmdJson() {
		return commands == nullptr ? "\"command\": null" : "\"command\": " + commands->toJson();
	}

public:

	_ASTElement * parent = nullptr;
	
	virtual ~_ASTElement() {}

	virtual std::string toJson() {
		std::string obj = "{\"class\": \"" + className() + "\",";
		obj += cmdJson();
		obj += "}";
		return obj;
	}

	virtual bool isEmpty() {
		return true;
	}

	virtual void addCommand(std::unique_ptr<ASTCommand> && command) {
		if (command != nullptr)
			commands = std::move(command);
	}

	virtual void addCommand(std::unique_ptr<ASTCommand> & command) {
		if (command != nullptr)
			commands = std::move(command);
	}

	virtual void executeCommands() {
		if (commands != nullptr)
			commands->execute();
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
		if (element != nullptr) {
			element->parent = this;
			elements.push_back(std::move(element));
		}
	}

	virtual void prependElement(std::unique_ptr<cl> & element) {
		if (element != nullptr) {
			element->parent = this;
			elements.insert(elements.begin(), std::move(element));
		}
	}

	virtual void addElement(std::unique_ptr<cl> && element) {
		if (element != nullptr) {
			element->parent = this;
			elements.push_back(std::move(element));
		}
	}

	virtual void addElements(std::vector<std::unique_ptr<cl>> && elements) {
		for (auto & e : elements)
			addElement(e);
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

};


// -------------------------------------- //
// ---------- INLINE ELEMENTS ----------- //
// -------------------------------------- //


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

	std::string toJson() {
		std::string obj = "{\"class\": \"" + className() + "\",";
		obj += "\"content\": \"";

		obj += content;

		obj += "\",";
		obj += cmdJson();
		obj += "}";
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
	: symbol(symbol), content(std::move(element)) {
		content->parent = this;
	}

	std::string literalText() override {
		return content->literalText();
	}

	std::string toJson() {
		std::string obj = "{\"class\": \"" + className() + "\",";
		obj += "\"symbol\": \"";
		obj += symbol;
		obj += "\","; 
		obj += "\"content\": ";

		obj += content->toJson();
		obj += ",";

		obj += cmdJson();
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
		obj += "\",";

		obj += cmdJson();
		obj += "}";
		return obj;
	}

};

/*
	Represents everything that is enclosed in square brackets
*/
class ASTModifier : public _ASTInlineElement {
protected:

	int type;
	std::string url;
	std::string command;
	
	std::unique_ptr<ASTInlineText> content;

	std::string className() {return "ASTModifier";}

public:

	ASTModifier(int type, std::string url, std::string command, std::unique_ptr<ASTInlineText> content)
		: type(type), url(url), command(command), content(std::move(content)) {
			content->parent = this;
		}

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
		obj += ",";

		obj += cmdJson();
		obj += "}";
		return obj;
	}

};

/*
	Represents Links with <a> element
*/
class ASTModifierLink : public _ASTInlineElement {
protected:

	std::string url;

	std::unique_ptr<ASTInlineText> content;

	std::string className() {return "ASTModifierLink";}

public:

	ASTModifierLink(std::string & url, std::unique_ptr<ASTInlineText> & content) : url(url), content(std::move(content)) {
		content->parent = this;
	}

	std::string toJson() {
		std::string obj = "{\"class\": \"" + className() + "\",";
		obj += "\"url\": \"" + url + "\",";

		obj += "\"content\":";
		obj += content->toJson();
		obj += ",";

		obj += cmdJson();
		obj += "}";
		return obj;
	}
};

/*
	Represents Images
*/
class ASTModifierImage : public _ASTInlineElement {
protected:

	std::string url;

	std::unique_ptr<ASTInlineText> content;
	
	std::string className() {return "ASTModifierImage";}

public:

	ASTModifierImage(std::string & url, std::unique_ptr<ASTInlineText> & content) : url(url), content(std::move(content)) {
		content->parent = this;
	}
	
	std::string toJson() {
		std::string obj = "{\"class\": \"" + className() + "\",";
		obj += "\"url\": \"" + url + "\",";

		obj += "\"content\":";
		obj += content->toJson();
		obj += ",";

		obj += cmdJson();
		obj += "}";
		return obj;
	}
};

/*
	Represents Footnotes
*/
class ASTModifierFootnote : public _ASTInlineElement {
protected:

	std::string id;

	std::unique_ptr<ASTInlineText> content;
	
	std::string className() {return "ASTModifierFootnote";}

public:

	ASTModifierFootnote(std::string & id, std::unique_ptr<ASTInlineText> & content) : id(id), content(std::move(content)) {
		content->parent = this;
	}
	
	std::string toJson() {
		std::string obj = "{\"class\": \"" + className() + "\",";
		obj += "\"id\": \"" + id + "\",";

		obj += "\"content\":";
		obj += content->toJson();
		obj += ",";

		obj += cmdJson();
		obj += "}";
		return obj;
	}
};

/*
	Represents Heading Links
*/
class ASTModifierHeadingLink : public _ASTInlineElement {
protected:

	std::string id;

	std::unique_ptr<ASTInlineText> content;
	
	std::string className() {return "ASTModifierHeadingLink";}

public:

	ASTModifierHeadingLink(std::string & id, std::unique_ptr<ASTInlineText> & content) : id(id), content(std::move(content)) {
		content->parent = this;
	}
	
	std::string toJson() {
		std::string obj = "{\"class\": \"" + className() + "\",";
		obj += "\"id\": \"" + id + "\",";

		obj += "\"content\":";
		obj += content->toJson();
		obj += ",";

		obj += cmdJson();
		obj += "}";
		return obj;
	}
};

/*
	Represents Replaced Content
*/
class ASTModifierReplace : public _ASTInlineElement {
protected:

	std::string id;

	std::unique_ptr<ASTInlineText> content;
	
	std::string className() {return "ASTModifierReplace";}

public:

	ASTModifierReplace(std::string & id, std::unique_ptr<ASTInlineText> & content) : id(id), content(std::move(content)) {
		content->parent = this;
	}
	
	std::string toJson() {
		std::string obj = "{\"class\": \"" + className() + "\",";
		obj += "\"id\": \"" + id + "\",";

		obj += "\"content\":";
		obj += content->toJson();
		obj += ",";

		obj += cmdJson();
		obj += "}";
		return obj;
	}
};


// -------------------------------------- //
// --------- MULTILINE ELEMENTS --------- //
// -------------------------------------- //


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

	ASTHeading(int level, std::unique_ptr<ASTInlineText> content) : level(level), content(std::move(content)) {
		content->parent = this;
	}

	std::string toJson() {
		std::string obj = "{\"class\": \"" + className() + "\",";
		obj += "\"level\": " + std::to_string(level) + ",";
		obj += "\"text\": ";

		obj += content->toJson();
		obj += ",";

		obj += cmdJson();
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

		obj += "],";

		obj += cmdJson();
		obj += "}";
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

		obj += "],";

		obj += cmdJson();
		obj += "}";
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
		command->parent = this;
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

		obj += "],";

		obj += cmdJson();
		obj += "}";
		return obj;
	}

};

/*
	Represents a Info Block. Everything should be able to fit in this object
*/
class ASTInfoBlock : public _ASTBlockElement {
protected:

	std::string className() {return "ASTInfoBlock";}

	std::string type;
	bool sym;

public:

	ASTInfoBlock() {}

	ASTInfoBlock(std::string type, bool sym = false) : type(type), sym(sym) {}

	std::string toJson() override {
		std::string obj = "{\"class\": \"" + className() + "\",";
		obj += "\"type\": \"" + type + "\",";
		obj += "\"sym\": " + std::to_string(sym) + ",";
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
};
