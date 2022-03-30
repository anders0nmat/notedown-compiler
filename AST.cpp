#include <unordered_set>

#include "AST.hpp"

// ----- ASTCommand ----- //

std::vector<std::string> ASTCommand::splitSnippets(std::string str, char c) {
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

std::vector<std::string> ASTCommand::splitAt(std::string str, char c) {
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

std::pair<std::string, std::string> ASTCommand::apartAt(std::string str, char c) {
	size_t pos = str.find_first_of(c);
	if (pos == std::string::npos)
		return std::make_pair(str, "");
	return std::make_pair(str.substr(0, pos), str.substr(pos + 1));
}

bool ASTCommand::strisalnum(std::string str) {
	for (auto e : str)
		if (!isalnum(e))
			return false;
	return true;
}

bool ASTCommand::strisiden(std::string str) {
	for (auto e : str)
		if (!isalnum(e) && e != '-' && e != '_')
			return false;
	return true;
}

ASTCommand::ASTCommand(std::string command) {
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
				attributes.insert(fields);
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
				css.append((css.empty() ? "" : " ") + funcarg.first + ": " + funcarg.second + ";");
			}
			continue;
		}
		if (e[0] == '%' && e.length() > 1) {
			std::string ref = e.substr(1);
			if (strisiden(ref))
				refCommands.push_back(ref);
			continue;
		}
	}
}

void ASTCommand::addClass(std::string className) {
	classes += (classes.empty() ? "" : " ") + className;
}

std::string ASTCommand::toJson() {
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

void ASTCommand::merge(ASTCommand & other) {
	id = other.id;
	classes += (classes.empty() ? "" : " ") + other.classes;
	title = other.title;
	for (auto & p : other.attributes)
		attributes[p.first] = p.second;
	css += (css.empty() ? "" : " ") + other.css;
	for (auto & e : other.functions)
		functions.push_back(e);
	for (auto & p : other.flags)
		flags[p.first] = p.second;
	for (auto & e : other.refCommands)
		refCommands.push_back(e);
}

void ASTCommand::merge(ASTCommand && other) {
	id = other.id;
	classes += (classes.empty() ? "" : " ") + other.classes;
	title = other.title;
	for (auto & p : other.attributes)
		attributes[p.first] = p.second;
	css += (css.empty() ? "" : " ") + other.css;
	for (auto & e : other.functions)
		functions.push_back(e);
	for (auto & p : other.flags)
		flags[p.first] = p.second;
	for (auto & e : other.refCommands)
		refCommands.push_back(e);
}

void ASTCommand::integrate(ASTCommand && other) {
	if (id.empty())
		id = other.id;
	classes += (classes.empty() ? "" : " ") + other.classes;
	if (title.empty())
		title = other.title;
	for (auto & p : other.attributes)
		attributes.insert(p);
	css += (css.empty() ? "" : " ") + other.css;
	for (auto & e : other.functions)
		functions.push_back(e);
	for (auto & p : other.flags)
		flags.insert(p);
	for (auto & e : other.refCommands)
		refCommands.push_back(e);
}

void ASTCommand::integrate(ASTCommand & other) {
	if (id.empty())
		id = other.id;
	classes += (classes.empty() ? "" : " ") + other.classes;
	if (title.empty())
		title = other.title;
	for (auto & p : other.attributes)
		attributes.insert(p);
	css += (css.empty() ? "" : " ") + other.css;
	for (auto & e : other.functions)
		functions.push_back(e);
	for (auto & p : other.flags)
		flags.insert(p);
	for (auto & e : other.refCommands)
		refCommands.push_back(e);
}

void ASTCommand::resolve(ASTRequestFunc request) {
	std::unordered_set<std::string> history = { refName };
	for (size_t i = 0; i < refCommands.size(); i++) {
		std::string & s = refCommands[i];
		if (!std::get<1>(history.insert(s)))
			continue;
		ASTIdDefinition * def = dynamic_cast<ASTIdDefinition *>(request("{" + s));
		if (def != nullptr)
			integrate(def->commands);
	}
}

std::string ASTCommand::constructHeader(ASTRequestFunc request) {
	std::string header;
	if (id != "")
		header += " id=\"" + id + "\"";
	if (classes != "")
		header += " class=\"" + classes + "\"";
	if (title != "")
		header += " title=\"" + title + "\"";
	if (css != "")
		header += " style=\"" + css + "\"";
	for (auto & p : attributes)
		header += " " + p.first + "=\"" + p.second + "\"";
	return header;
}


// -------------------------------------- //
// ------------- TEMPLATES -------------- //
// -------------------------------------- //


// ----- _ASTElement ----- //

std::string _ASTElement::cmdJson() {
	return "\"command\": " + commands.toJson();
}

_ASTElement * _ASTElement::getDocument() {
	return parent;
}

_ASTElement * _ASTElement::containingElement() {
	return this;
}

std::string _ASTElement::toJson() {
	std::string obj = "{\"class\": \"" + className() + "\",";
	obj += cmdJson();
	obj += "}";
	return obj;
}

bool _ASTElement::isEmpty() {
	return true;
}

void _ASTElement::addCommand(ASTCommand && command) {
	commands.merge(command);
}

void _ASTElement::addCommand(ASTCommand & command) {
	commands.merge(command);
}

void _ASTElement::executeCommands() {
	commands.execute();
}

std::string _ASTElement::getHtml(ASTRequestFunc request) {
	return "";
}

// ----- _ASTInlineElement ----- //

_ASTElement * _ASTInlineElement::containingElement() {
	return parent;
}

std::string _ASTInlineElement::literalText() {
	return "";
}


// -------------------------------------- //
// ---------- INLINE ELEMENTS ----------- //
// -------------------------------------- //


// ----- ASTInlineText ----- //

std::string ASTInlineText::literalText() {
	std::string res;
	for (auto & e : elements)
		res += e->literalText();
	return res;
}

std::string ASTInlineText::toJson() {
	return _ASTListElement<_ASTInlineElement>::toJson();
}

std::string ASTInlineText::getHtml(std::function<_ASTElement*(std::string)> request) {
	std::string html;
	for (auto & e : elements) {
		html += e->getHtml(request);
	}
	return html;
}

// ----- ASTPlainText ----- //

std::string ASTPlainText::literalText() {
	return content;
}

std::string ASTPlainText::toJson() {
	std::string obj = "{\"class\": \"" + className() + "\",";
	obj += "\"content\": \"";

	obj += content;

	obj += "\",";
	obj += cmdJson();
	obj += "}";
	return obj;
}

std::string ASTPlainText::getHtml(ASTRequestFunc request) {
	std::string cnt;
	for (auto c : content) {
		switch (c) {
			case '&':
				cnt += "&amp;";
				break;
			case '<':
				cnt += "&lt;";
				break;
			case '>':
				cnt += "&gt;";
				break;
			default:
				cnt += c;
		}
	}

	return cnt;
}

// ----- ASTLinebreak ----- //

std::string ASTLinebreak::getHtml(ASTRequestFunc request) {
	return "<br>";
}

// ----- ASTTextModification ----- //

std::string ASTTextModification::literalText() {
	return content->literalText();
}

std::string ASTTextModification::toJson() {
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

std::string ASTTextModification::getHtml(ASTRequestFunc request) {
	std::string tag;
	switch (symbol) {
		case '*': 
			tag = "strong";
			break;
		case '/': 
			tag = "em";
			break;
		case '_': 
			tag = "u";
			break;
		case '~': 
			tag = "del";
			break;
		case '=': 
			tag = "mark";
			break;
		case '`': 
			tag = "code";
			break;
	}	
	return "<" + tag + ">" + content->getHtml(request) + "</" + tag + ">";
}

// ----- ASTEmoji ----- //

std::string ASTEmoji::toJson() {
	std::string obj = "{\"class\": \"" + className() + "\",";
	obj += "\"shortcode\": \"";
	obj += shortcode;
	obj += "\",";

	obj += cmdJson();
	obj += "}";
	return obj;
}

std::string ASTEmoji::getHtml(ASTRequestFunc request) {
	return ":EMOJI " + shortcode + ":";
}

// ----- ASTModifier ----- //

std::string ASTModifier::literalText() {
	return content->literalText();
}

std::string ASTModifier::toJson() {
	std::string obj = "{\"class\": \"" + className() + "\",";
	obj += "\"url\": \"" + url + "\",";

	obj += "\"content\":";
	obj += content->toJson();
	obj += ",";

	obj += cmdJson();
	obj += "}";
	return obj;
}

// ----- ASTLink ----- //

std::string ASTLink::getHtml(ASTRequestFunc request) {
	if (url[0] == '%') {
		ASTIdDefinition * def = dynamic_cast<ASTIdDefinition *>(request("(" + url.substr(1)));
		if (def != nullptr) {
			url = def->url;
			commands.integrate(def->commands);
		}
	}

	std::string html = "<a";
	commands.attributes["href"] = url;
	html += commands.constructHeader(request);
	html += ">";
	html += content->getHtml(request);
	html += "</a>";
	return html;
}

// ----- ASTImage ----- //

std::string ASTImage::getHtml(ASTRequestFunc request) {
	if (url[0] == '%') {
		ASTIdDefinition * def = dynamic_cast<ASTIdDefinition *>(request("(" + url.substr(1)));
		if (def != nullptr) {
			url = def->url;
			commands.integrate(def->commands);
		}
	}

	std::string html = "<img";
	commands.attributes["src"] = url;
	commands.attributes["alt"] = content->literalText();
	html += commands.constructHeader(request);
	html += ">";
	return html;
}

// ----- ASTFootnote ----- //

std::string ASTFootnote::getHtml(ASTRequestFunc request) {
	if (url[0] == '%') {
		ASTIdDefinition * def = dynamic_cast<ASTIdDefinition *>(request("(" + url.substr(1)));
		if (def != nullptr) {
			url = def->url;
			commands.integrate(def->commands);
		}
	}

	std::string html = "<a";
	commands.attributes["href"] = "#" + url;
	commands.addClass("footnote");
	if (request("^" + url))
		commands.addClass("missing");
	html += commands.constructHeader(request);
	html += ">";
	html += content->getHtml(request);
	html += "</a>";
	return html;
}

// ----- ASTHeadingLink ----- //

std::string ASTHeadingLink::getHtml(ASTRequestFunc request) {
	std::string html = "<a";
	commands.attributes["href"] = "#" + url;
	commands.addClass("h-link");
	if (request("#" + url) == nullptr)
		commands.addClass("missing");
	html += commands.constructHeader(request);
	html += ">";
	html += content->getHtml(request);
	html += "</a>";
	return html;
}

// ----- ASTReplace ----- //

std::string ASTReplace::getHtml(ASTRequestFunc request) {
	_ASTBlockElement * replContent = dynamic_cast<_ASTBlockElement *>(request("<" + url));

	std::string html = "<div";
	if (replContent == nullptr)
		commands.addClass("missing");
	html += commands.constructHeader(request);
	html += ">";

	if (replContent == nullptr)
		html += content->getHtml(request);
	else {
		for (auto & e : replContent->elements)
			html += e->getHtml(request) + "\n";
	}
	html += "</div>";
	return html;
}


// -------------------------------------- //
// --------- MULTILINE ELEMENTS --------- //
// -------------------------------------- //


// ----- ASTDocument ----- //

_ASTElement * ASTDocument::getDocument() {
	return this;
}

// ----- ASTIdDefinition ----- //

void ASTIdDefinition::registerNow() {
	ASTDocument * doc = dynamic_cast<ASTDocument *>(getDocument());
	if (doc == nullptr)
		return;
	std::string _id(1, type);
	_id += id;
	doc->iddef[_id] = this;
	if (type == '{')
		commands.refName = id;
}

std::string ASTIdDefinition::toJson() {
	std::string obj = "{\"class\": \"" + className() + "\",";
	obj += "\"url\": \"" + url + "\",";
	obj += "\"id\": \"" + id + "\",";
	obj += "\"type\": \"" + std::to_string(type) + "\",";
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

std::string ASTIdDefinition::getHtml(ASTRequestFunc request) {
	return "";
}

// ----- ASTHeading ----- //

std::string ASTHeading::toJson() {
	std::string obj = "{\"class\": \"" + className() + "\",";
	obj += "\"level\": " + std::to_string(level) + ",";
	obj += "\"text\": ";

	obj += content->toJson();
	obj += ",";

	obj += cmdJson();
	obj += "}";
	return obj;
}

std::string ASTHeading::getHtml(ASTRequestFunc request) {
	std::string html = "<h" + std::to_string(level);
	html += commands.constructHeader(request);
	html += ">";
	html += content->getHtml(request);
	html += "</h" + std::to_string(level) + ">";
	return html;
}

// ----- ASTHLine ----- //

std::string ASTHLine::getHtml(ASTRequestFunc request) {
	return "<hr>";
}

// ----- ASTParagraph ----- //

std::string ASTParagraph::getHtml(ASTRequestFunc request) {
	std::string html = "<p";
	html += commands.constructHeader(request);
	html += ">\n";
	html += _ASTBlockElement::getHtml(request);
	html += "</p>";
	return html;
}

// ----- ASTBlockquote ----- //

std::string ASTBlockquote::toJson() {
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

std::string ASTBlockquote::getHtml(ASTRequestFunc request) {
	std::string html = "<blockquote";
	if (centered)
		commands.addClass("center");
	html += commands.constructHeader(request);
	html += ">\n";
	html += _ASTBlockElement::getHtml(request);
	html += "</blockquote>";
	return html;
}

// ----- ASTListElement ----- //

std::string ASTListElement::toJson() {
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

std::string ASTListElement::getHtml(ASTRequestFunc request) {
	std::string html = "<li";
	html += commands.constructHeader(request);
	html += ">";
	html += _ASTBlockElement::getHtml(request);
	html += "</li>";
	return html;
}

// ----- ASTUnorderedList ----- //

std::string ASTUnorderedList::getHtml(ASTRequestFunc request) {
	std::string html = "<ul";
	html += commands.constructHeader(request);
	html += ">\n";
	html += _ASTListElement<ASTListElement>::getHtml(request);
	html += "</ul>";
	return html;
}

// ----- ASTOrderedList ----- //

std::string ASTOrderedList::getHtml(ASTRequestFunc request) {
	std::string html = "<ol";
	html += commands.constructHeader(request);
	html += ">\n";
	html += _ASTListElement<ASTListElement>::getHtml(request);
	html += "</ol>";
	return html;
}

// ----- ASTCodeBlock ----- //

void ASTCodeBlock::addCommand(std::unique_ptr<ASTInlineText> & e) {
	command = std::move(e);
	command->parent = this;
}

std::string ASTCodeBlock::toJson() {
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

std::string ASTCodeBlock::getHtml(ASTRequestFunc request) {
	std::string html = "<pre><code";
	html += commands.constructHeader(request);
	html += ">\n";
	html += _ASTBlockElement::getHtml(request);
	html += "</code></pre>";
	return html;
}

// ----- ASTInfoBlock ----- //

std::string ASTInfoBlock::toJson() {
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

std::string ASTInfoBlock::getHtml(ASTRequestFunc request) {
	std::string html = "<blockquote";
	commands.addClass(type);
	if (sym)
		commands.addClass("sym");
	html += commands.constructHeader(request);
	html += ">\n";
	html += _ASTBlockElement::getHtml(request);
	html += "</blockquote>";
	return html;
}
