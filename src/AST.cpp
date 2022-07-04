#include <unordered_set>

#include "notedown-compiler.hpp"
#include "AST.hpp"
#include "highlighter.hpp"

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
			auto cls(splitAt(e, '.'));
			for (auto & c : cls) {
				if (strisiden(c))
					classes.insert(c);
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
				}
				genFunction = f;
			}
			
			continue;
		}
		if (e[0] == '&' && e.length() > 1) {
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
				}
				modFunctions.push_back(f);
			}
			
			continue;
		}
		if (e[0] == '>' && e.length() > 4) {
			auto funcarg = apartAt(e.substr(1), ':');
			if (!funcarg.second.empty() && strisiden(funcarg.first)) {
				if (funcarg.second[0] == '"') {
					funcarg.second = funcarg.second.substr(1, funcarg.second.length() - 2);
				}
				css.emplace(funcarg.first, funcarg.second);
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
	classes.insert(className);
}

std::string ASTCommand::toJson() {
	std::string obj = "{\"class\": \"" + className() + "\",";
	obj += "\"id\": \"" + id + "\",";
	obj += "\"classes\": \"";
	for (auto & e : classes) {
		obj += " " + e;
	}
	obj += "\",";
	obj += "\"title\": \"" + title + "\",";
	obj += "\"css\": \"";
	for (auto & p : css) {
		obj += " " + p.first + ":" + p.second + ";";
	}
	obj += "\",";

	obj += "\"attributes\": {";

	for (auto & e : attributes) {
		obj += "\"" + e.first + "\": \"" + e.second + "\",";
	}		
	
	if (attributes.size() != 0)
		obj.erase(std::prev(obj.end()));

	obj += "},";

	obj += "\"genFunction\": {";	
	obj += "\"" + genFunction.first + "\": [";
	for (auto arg : genFunction.second) {
		obj += "\"" + arg + "\",";
	}
	if (genFunction.second.size() != 0)
		obj.erase(std::prev(obj.end()));
	obj += "],";	
	obj += "},";

	obj += "\"modFunctions\": {";

	for (auto & e : modFunctions) {
		obj += "\"" + e.first + "\": [";
		for (auto arg : e.second) {
			obj += "\"" + arg + "\",";
		}
		if (e.second.size() != 0)
			obj.erase(std::prev(obj.end()));
		obj += "],";
	}		
	
	if (modFunctions.size() != 0)
		obj.erase(std::prev(obj.end()));

	obj += "}";
	
	
	obj += "}";
	return obj;
}

void ASTCommand::merge(ASTCommand & other) {
	// id = other.id;
	// classes.insert(other.classes.begin(), other.classes.end());
	// title = other.title;
	// for (auto & p : other.attributes)
	// 	attributes[p.first] = p.second;
	// for (auto & p : other.css)
	// 	css[p.first] = p.second;
	// genFunction = other.genFunction;
	// for (auto & e : other.modFunctions)
	// 	modFunctions.push_back(e);
	// for (auto & p : other.flags)
	// 	flags[p.first] = p.second;
	// for (auto & e : other.refCommands)
	// 	refCommands.push_back(e);
	merge(std::move(other));
}

void ASTCommand::merge(ASTCommand && other) {
	id = other.id;
	classes.insert(other.classes.begin(), other.classes.end());
	title = other.title;
	for (auto & p : other.attributes)
		attributes[p.first] = p.second;
	for (auto & p : other.css)
		css[p.first] = p.second;
	genFunction = other.genFunction;
	for (auto & e : other.modFunctions)
		modFunctions.push_back(e);
	for (auto & p : other.flags)
		flags[p.first] = p.second;
	for (auto & e : other.refCommands)
		refCommands.push_back(e);
}

void ASTCommand::integrate(ASTCommand && other) {
	if (id.empty())
		id = other.id;
	classes.insert(other.classes.begin(), other.classes.end());
	if (title.empty())
		title = other.title;
	for (auto & p : other.attributes)
		attributes.insert(p);
	css.insert(other.css.begin(), other.css.end());
	if (genFunction.first.empty())
		genFunction = other.genFunction;
	for (auto & e : other.modFunctions)
		modFunctions.push_back(e);
	for (auto & p : other.flags)
		flags.insert(p);
	for (auto & e : other.refCommands)
		refCommands.push_back(e);
}

void ASTCommand::integrate(ASTCommand & other) {
	integrate(std::move(other));
	// if (id.empty())
	// 	id = other.id;
	// classes.insert(other.classes.begin(), other.classes.end());
	// if (title.empty())
	// 	title = other.title;
	// for (auto & p : other.attributes)
	// 	attributes.insert(p);
	// css.insert(other.css.begin(), other.css.end());
	// if (genFunction.first.empty())
	// 	genFunction = other.genFunction;
	// for (auto & e : other.modFunctions)
	// 	modFunctions.push_back(e);
	// for (auto & p : other.flags)
	// 	flags.insert(p);
	// for (auto & e : other.refCommands)
	// 	refCommands.push_back(e);
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
	if (!classes.empty()) {
		std::string clsAtr;
		for (auto & e : classes) {
			clsAtr += (clsAtr.empty() ? "" : " ") + e;
		}
		header += " class=\"" + clsAtr + "\"";
	}
	if (title != "")
		header += " title=\"" + title + "\"";
	if (!css.empty()) {
		std::string cssAtr;
		for (auto & p : css) {
			cssAtr += (cssAtr.empty() ? "" : " ") + p.first + ": " + p.second + ";";
		}
		header += " style=\"" + cssAtr + "\"";
	}
	for (auto & p : attributes)
		header += " " + p.first + "=\"" + p.second + "\"";
	return header;
}

void ASTCommand::execute(_ASTElement * caller, ASTProcess step, ASTRequestModFunc modFunc) {
	ASTModFunc func;
	if (!genFunction.first.empty()) {
		func = modFunc("$" + genFunction.first);
		if (func) {
			func(caller, step, genFunction.second);
		}
	}
	for (auto & e : modFunctions) {
		if (!e.first.empty()) {
			func = modFunc("&" + e.first);
			if (func)
				func(caller, step, e.second);
		}
	}
}


// -------------------------------------- //
// ------------- TEMPLATES -------------- //
// -------------------------------------- //


// ----- _ASTElement ----- //

std::string _ASTElement::cmdJson() {
	return "\"command\": " + commands.toJson();
}

void _ASTElement::_consume(ASTProcess step, ASTRequestFunc request, ASTRequestModFunc modFunc) {}
void _ASTElement::_register(ASTProcess step, ASTRequestFunc request, ASTRequestModFunc modFunc) {}

void _ASTElement::_resolve(ASTProcess step, ASTRequestFunc request, ASTRequestModFunc modFunc) {
	commands.resolve(request);
}

void _ASTElement::_identify(ASTProcess step, ASTRequestFunc request, ASTRequestModFunc modFunc) {}

void _ASTElement::_execute(ASTProcess step, ASTRequestFunc request, ASTRequestModFunc modFunc) {
	commands.execute(this, step, modFunc);
}

_ASTElement * _ASTElement::getDocument() {
	return parent->getDocument();
}

_ASTElement * _ASTElement::containingElement() {
	return this;
}

void _ASTElement::process(ASTProcess step, ASTRequestFunc request, ASTRequestModFunc modFunc) {
	switch (step) {
	case procConsume:
		_consume(step, request, modFunc);
		break;
	case procRegister:
		_register(step, request, modFunc);
		break;
	case procResolve:
		_resolve(step, request, modFunc);
		break;
	case procIdentify:
		_identify(step, request, modFunc);
		break;
	case procExecutePrep:
	case procExecuteMain:
	case procExecutePost:
		_execute(step, request, modFunc);
		break;
	default:
		break;
	}
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

bool _ASTElement::canConsume() {
	return isEmpty();
}

void _ASTElement::addCommand(ASTCommand && command) {
	commands.merge(command);
}

void _ASTElement::addCommand(ASTCommand & command) {
	commands.merge(command);
}

std::string _ASTElement::getHtml(ASTRequestFunc request) {
	return "";
}

// ----- _ASTInlineElement ----- //

void _ASTInlineElement::_consume(ASTProcess step, ASTRequestFunc request, ASTRequestModFunc modFunc) {}

_ASTElement * _ASTInlineElement::containingElement() {
	return parent->containingElement();
}

std::string _ASTInlineElement::literalText() {
	return "";
}


// -------------------------------------- //
// ---------- INLINE ELEMENTS ----------- //
// -------------------------------------- //


// ----- ASTCommandContainer ----- //

void ASTCommandContainer::_register(ASTProcess step, ASTRequestFunc request, ASTRequestModFunc modFunc) {
	if (!commands.genFunction.first.empty())
		return;
	// Does not create content, can cascade up
	_ASTElement * container = containingElement();
	if (container == nullptr)
		return;
	container->commands.integrate(commands);
	commands = ASTCommand(); // Empty this command
}

void ASTCommandContainer::setContent(std::unique_ptr<_ASTElement> & element) {
	content = std::move(element);
}

void ASTCommandContainer::setContent(std::unique_ptr<_ASTElement> && element) {
	content = std::move(element);
}

bool ASTCommandContainer::isEmpty() {
	return content == nullptr;
}

std::string ASTCommandContainer::getHtml(ASTRequestFunc request) {
	if (content == nullptr)
		return "";

	std::string html = "<span";
	html += commands.constructHeader(request);
	html += ">";
	html += content->getHtml(request);
	html += "</span>";
	return html;
}

// ----- ASTInlineText ----- //

void ASTInlineText::_consume(ASTProcess step, ASTRequestFunc request, ASTRequestModFunc modFunc) {
	_ASTInlineElement::_consume(step, request, modFunc);
}

_ASTElement * ASTInlineText::containingElement() {
	return _ASTInlineElement::containingElement();
}

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

bool ASTPlainText::isEmpty() {
	return content.empty();
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

bool ASTLinebreak::isEmpty() {
	return false;
}

std::string ASTLinebreak::getHtml(ASTRequestFunc request) {
	return "<br>";
}

// ----- ASTTextModification ----- //

std::string ASTTextModification::literalText() {
	return content->literalText();
}

bool ASTTextModification::isEmpty() {
	return content->isEmpty();
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

bool ASTEmoji::isEmpty() {
	return shortcode.empty();
}

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
	ASTPlainText * text = dynamic_cast<ASTPlainText *>(request(":" + shortcode));
	if (text == nullptr)
		return ":" + shortcode + ":";
	return text->getHtml(request);
}

// ----- ASTModifier ----- //

bool ASTModifier::isEmpty() {
	return content->isEmpty() && url.empty();
}

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

void ASTLink::_resolve(ASTProcess step, ASTRequestFunc request, ASTRequestModFunc modFunc) {
	if (url[0] == '%') {
		std::string id = Notedown::makeId(url.substr(1));
		if (id.empty() && !content->isEmpty())
			id = Notedown::makeId(content->literalText());
		ASTIdDefinition * def = dynamic_cast<ASTIdDefinition *>(request("(" + id));
		if (def != nullptr) {
			url = def->url;
			commands.integrate(def->commands);
		}
	}

	commands.resolve(request);
}

std::string ASTLink::getHtml(ASTRequestFunc request) {
	std::string html = "<a";
	commands.attributes["href"] = url;
	html += commands.constructHeader(request);
	html += ">";
	if (content->isEmpty())
		html += url;
	else
		html += content->getHtml(request);
	html += "</a>";
	return html;
}

// ----- ASTImage ----- //

bool ASTImage::isEmpty() {
	return false;
}

void ASTImage::_resolve(ASTProcess step, ASTRequestFunc request, ASTRequestModFunc modFunc) {
	if (url[0] == '%') {
		std::string id = Notedown::makeId(url.substr(1));
		if (id.empty() && !content->isEmpty())
			id = Notedown::makeId(content->literalText());
		ASTIdDefinition * def = dynamic_cast<ASTIdDefinition *>(request("(" + id));
		if (def != nullptr) {
			url = def->url;
			commands.integrate(def->commands);
		}
	}

	commands.resolve(request);
}

std::string ASTImage::getHtml(ASTRequestFunc request) {
	std::string html = "<img";
	commands.attributes["src"] = url;
	if (content->isEmpty())
		commands.attributes["alt"] = url;
	else
		commands.attributes["alt"] = content->literalText();
	html += commands.constructHeader(request);
	html += ">";
	return html;
}

// ----- ASTFootnote ----- //

void ASTFootnote::_resolve(ASTProcess step, ASTRequestFunc request, ASTRequestModFunc modFunc) {
	if (url[0] == '%') {
		std::string id = Notedown::makeId(url.substr(1));
		if (id.empty() && !content->isEmpty())
			id = Notedown::makeId(content->literalText());
		ASTIdDefinition * def = dynamic_cast<ASTIdDefinition *>(request("(" + id));
		if (def != nullptr) {
			url = def->url;
			commands.integrate(def->commands);
		}
	}

	commands.resolve(request);
}

std::string ASTFootnote::getHtml(ASTRequestFunc request) {
	std::string html = "<a";
	commands.attributes["href"] = "#" + url;
	commands.addClass("nd-footnote");
	if (request("^" + url) == nullptr)
		commands.addClass("nd-missing");
	html += commands.constructHeader(request);
	html += ">";
	if (content->isEmpty())
		html += url;
	else
		html += content->getHtml(request);
	html += "</a>";
	return html;
}

// ----- ASTHeadingLink ----- //

std::string ASTHeadingLink::getHtml(ASTRequestFunc request) {
	std::string html = "<a";
	commands.attributes["href"] = "#" + url;
	commands.addClass("nd-h-link");
	if (request("#" + url) == nullptr)
		commands.addClass("nd-missing");
	html += commands.constructHeader(request);
	html += ">";
	html += content->getHtml(request);
	html += "</a>";
	return html;
}

// ----- ASTReplace ----- //

bool ASTReplace::isEmpty() {
	return false;
}

std::string ASTReplace::getHtml(ASTRequestFunc request) {
	_ASTBlockElement * replContent = dynamic_cast<_ASTBlockElement *>(request("<" + url));

	std::string html = "<div";
	if (replContent == nullptr)
		commands.addClass("nd-missing");
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

// ----- ASTStyled ----- //

std::string ASTStyled::getHtml(ASTRequestFunc request) {
	std::string html = "<span";
	html += commands.constructHeader(request);
	html += ">";
	html += content->getHtml(request);
	html += "</span>";
	return html;
}

// ----- ASTTask ----- //

void ASTTask::_resolve(ASTProcess step, ASTRequestFunc request, ASTRequestModFunc modFunc) {
	// Structure has to be the following:
	// ASTUnorderedList
	//   ASTListElement
	//     ASTParagraph
	//       ASTInlineText
	//         [ASTTask]
	valid = false;

	ASTInlineText * text = dynamic_cast<ASTInlineText *>(parent);
	if (text == nullptr) return;
	if (text->elements[0].get() != this) return;

	ASTParagraph * par = dynamic_cast<ASTParagraph *>(text->parent);
	if (par == nullptr) return;
	if (par->elements[0].get() != text) return;
	
	ASTListElement * elem = dynamic_cast<ASTListElement *>(par->parent);
	if (elem == nullptr) return;
	if (elem->elements[0].get() != par) return;

	ASTUnorderedList * list = dynamic_cast<ASTUnorderedList *>(elem->parent);
	if (list == nullptr) return;

	valid = true;
	elem->commands.addClass("nd-task-list-item");
}

bool ASTTask::isEmpty() {
	return false;
}

std::string ASTTask::getHtml(ASTRequestFunc request) {
	if (!valid)
		return "[" + std::string(1, checked) + "]";

	std::string html = "<input";
	commands.attributes["type"] = "checkbox";
	if (checked == 'X' || checked == 'x')
		commands.attributes["checked"] = "";
	html += commands.constructHeader(request);
	html += ">";
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

void ASTIdDefinition::_register(ASTProcess step, ASTRequestFunc request, ASTRequestModFunc modFunc) {
	ASTDocument * doc = dynamic_cast<ASTDocument *>(getDocument());
	if (doc == nullptr)
		return;
	std::string _id(1, type);
	_id += id;
	doc->iddef[_id] = this;
	if (type == '{')
		commands.refName = id;
}

bool ASTIdDefinition::isEmpty() {
	return true;
}

bool ASTIdDefinition::canConsume() {
	return false;
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

void ASTHeading::_consume(ASTProcess step, ASTRequestFunc request, ASTRequestModFunc modFunc) {
	if (content != nullptr && content->isEmpty())
		commands.integrate(content->commands);

	if (commands.id.empty())
		commands.id = Notedown::makeId(content->literalText());
}

void ASTHeading::_identify(ASTProcess step, ASTRequestFunc request, ASTRequestModFunc modFunc) {
	ASTDocument * doc = dynamic_cast<ASTDocument *>(getDocument());
	if (doc == nullptr)
		return;
	doc->iddef["#" + commands.id] = this;
}

void ASTHeading::process(ASTProcess step, ASTRequestFunc request, ASTRequestModFunc modFunc) {
	if (content != nullptr)
		content->process(step, request, modFunc);
	_ASTElement::process(step, request, modFunc);
}

bool ASTHeading::isEmpty() {
	return false;
}

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
	if (content != nullptr)
		html += content->getHtml(request);
	html += "</h" + std::to_string(level) + ">";
	return html;
}

// ----- ASTHLine ----- //

bool ASTHLine::isEmpty() {
	return false;
}

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
		commands.addClass("nd-center");
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

void ASTCodeBlock::_register(ASTProcess step, ASTRequestFunc request, ASTRequestModFunc modFunc) {
	// if (command != nullptr) {
	// 	commands = command->commands;
	// 	command->commands = ASTCommand();
	// }
}

void ASTCodeBlock::process(ASTProcess step, ASTRequestFunc request, ASTRequestModFunc modFunc) {
	if (command != nullptr)
		command->process(step, request, modFunc);
	_ASTBlockElement::process(step, request, modFunc);
}

void ASTCodeBlock::addCommand(std::unique_ptr<ASTInlineText> & e) {
	command = std::move(e);
	if (this->command != nullptr)
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
	html += ">";

	ASTContainerSyntaxHighlight * lang_handler = dynamic_cast<ASTContainerSyntaxHighlight*>(request("~" + lang));
	if (lang_handler != nullptr) {
		std::string raw;
		_ASTInlineElement * text;
		ASTInlineText * container;
		std::string total_text;
		Highlighter highlighter = lang_handler->engine->getHighlighter(lang);
		for (size_t i = 0; i < elements.size(); i++) {
			text = dynamic_cast<_ASTInlineElement*>(elements[i].get());
			if (text == nullptr) continue;
			
			raw = text->literalText();
			elements[i] = std::make_unique<ASTInlineText>();
			container = dynamic_cast<ASTInlineText*>(elements[i].get());
			highlighter(raw, [container](std::string output, const std::string & type){
				if (output.empty() || type.empty()) {
					container->addElement(std::make_unique<ASTPlainText>(output));
				}
				else {
					std::unique_ptr<ASTStyled> style = std::make_unique<ASTStyled>();
					style->commands.addClass("nd-syntax-" + type);
					style->content = std::make_unique<ASTInlineText>();
					style->content->parent = style->content.get();
					style->content->addElement(std::make_unique<ASTPlainText>(output));
					container->addElement(std::move(style));
				}
			});
			// lang_handler->engine->highlight_callback(raw, lang, [container](std::string output, const std::string & type){
			// 	if (type.empty()) {
			// 		container->addElement(std::make_unique<ASTPlainText>(output));
			// 	}
			// 	else {
			// 		std::unique_ptr<ASTStyled> style = std::make_unique<ASTStyled>();
			// 		style->commands.addClass("nd-syntax-" + type);
			// 		style->content = std::make_unique<ASTInlineText>();
			// 		style->content->parent = style->content.get();
			// 		style->content->addElement(std::make_unique<ASTPlainText>(output));
			// 		container->addElement(std::move(style));
			// 	}
			// });
		}
	}

	for (auto & e : elements)
		html += e->getHtml(request) + "\n";
	if (elements.size() > 0)
		html.erase(std::prev(html.end()));
	
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
		commands.addClass("nd-sym");
	html += commands.constructHeader(request);
	html += ">\n";
	html += _ASTBlockElement::getHtml(request);
	html += "</blockquote>";
	return html;
}

// ----- ASTFootnoteBlock ----- //

void ASTFootnoteBlock::_register(ASTProcess step, ASTRequestFunc request, ASTRequestModFunc modFunc) {
	ASTDocument * doc = dynamic_cast<ASTDocument *>(getDocument());
	if (doc == nullptr)
		return;
	doc->iddef["^" + id] = this;
}

std::string ASTFootnoteBlock::toJson() {
	std::string obj = "{\"class\": \"" + className() + "\",";
	obj += "\"id\": " + id + ",";
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

std::string ASTFootnoteBlock::getHtml(ASTRequestFunc request) {
	std::string html = "<div";
	commands.addClass("nd-footnote");
	if (!id.empty())
		commands.id = id;

	html += commands.constructHeader(request);
	html += ">\n";
	html += _ASTBlockElement::getHtml(request);
	html += "</div>";
	return html;
}

// ----- ASTCollapseBlock ----- //

void ASTCollapseBlock::process(ASTProcess step, ASTRequestFunc request, ASTRequestModFunc modFunc) {
	if (summary != nullptr)
		summary->process(step, request, modFunc);
	_ASTBlockElement::process(step, request, modFunc);
}

std::string ASTCollapseBlock::toJson() {
	std::string obj = "{\"class\": \"" + className() + "\",";
	obj += "\"summary\": " + summary->toJson() + ",";
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

std::string ASTCollapseBlock::getHtml(ASTRequestFunc request) {
	std::string html = "<details";
	if (isOpen)
		commands.attributes["open"] = "open";
	html += commands.constructHeader(request);
	html += ">\n";
	html += "<summary>";
	if (summary != nullptr && !summary->isEmpty())
		html += summary->getHtml(request);
	html += "</summary>\n";
	html += _ASTBlockElement::getHtml(request);
	html += "</details>";
	return html;
}