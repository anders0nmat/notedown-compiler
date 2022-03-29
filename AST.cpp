#include <unordered_set>

#include "AST.hpp"

std::string ASTCommand::constructHeader(std::function<_ASTElement*(std::string)> request) {
	std::unordered_set<std::string> history = { refName };
	for (size_t i = 0; i < refCommands.size(); i++) {
		std::string & s = refCommands[i];
		if (!std::get<1>(history.insert(s)))
			continue;
		ASTIdDefinition * def = dynamic_cast<ASTIdDefinition *>(request("{" + s));
		if (def != nullptr)
			integrate(def->commands);
	}

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

std::string ASTModifierLink::getHtml(std::function<_ASTElement*(std::string)> request) {
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

std::string ASTModifierImage::getHtml(std::function<_ASTElement*(std::string)> request) {
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

std::string ASTModifierFootnote::getHtml(std::function<_ASTElement*(std::string)> request) {
	if (id[0] == '%') {
		ASTIdDefinition * def = dynamic_cast<ASTIdDefinition *>(request("(" + id.substr(1)));
		if (def != nullptr) {
			id = def->url;
			commands.integrate(def->commands);
		}
	}

	std::string html = "<a";
	commands.attributes["href"] = "#" + id;
	commands.addClass("footnote");
	if (request("^" + id))
		commands.addClass("missing");
	html += commands.constructHeader(request);
	html += ">";
	html += content->getHtml(request);
	html += "</a>";
	return html;
}


