#include "highlighter.hpp"

#include <set>
#include <fstream>

void SyntaxGroup::addGroup(std::string group_name) {
	for (auto & g : groups)
		if (g == group_name) return;
	
	groups.push_back(group_name);
}

void SyntaxGroup::removeGroup(std::string group_name) {
	for (auto it = groups.begin(); it != groups.end(); it++)
		if (*it == group_name)
			groups.erase(it);

	matches.erase(group_name);	
}

void SyntaxGroup::addMatch(std::string group_name, std::string match) {
	addGroup(group_name);
	matches[group_name].emplace_back(match);
}

void SyntaxGroup::addMatch(std::string group_name, std::initializer_list<std::string> match) {
	addGroup(group_name);
	auto & list = matches[group_name];
	for (auto & e : match)
		list.emplace_back(e);
}

void SyntaxGroup::addMatch(std::string group_name, std::vector<std::string> match) {
	addGroup(group_name);
	auto & list = matches[group_name];
	for (auto & e : match)
		list.emplace_back(e);
}




std::vector<std::string> splitStr(const std::string & str, std::string delimiters) {

	std::vector<std::string> result;
	std::string tok;

	for (auto & c : str) {
		if (delimiters.find_first_of(c) != std::string::npos) {
			if (!tok.empty()) {
				result.push_back(tok);
				tok = "";
			}
			continue;
		}
		tok += c;
	}

	if (!tok.empty()) {
		result.push_back(tok);
	}
	return result;
}



void HighlighterEngine::addLanguage(std::string language) {
	if (languages.count(language) == 0)
		languages[language] = std::make_shared<SyntaxGroup>();
}

void HighlighterEngine::removeLanguage(std::string language) {
	languages.erase(language);
}

bool HighlighterEngine::hasLanguage(std::string language) {
	return languages.count(language) != 0;
}

void HighlighterEngine::addGroup(std::string language, std::string group_name) {
	addLanguage(language);
	languages[language]->addGroup(group_name);
}

void HighlighterEngine::removeGroup(std::string language, std::string group_name) {
	addLanguage(language);
	languages[language]->removeGroup(group_name);
}

void HighlighterEngine::addMatch(std::string language, std::string group_name, std::string match) {
	addLanguage(language);
	languages[language]->addMatch(group_name, match);
}

void HighlighterEngine::addMatch(std::string language, std::string group_name, std::initializer_list<std::string> match) {
	addLanguage(language);
	languages[language]->addMatch(group_name, match);
}

void HighlighterEngine::addMatch(std::string language, std::string group_name, std::vector<std::string> match) {
	addLanguage(language);
	languages[language]->addMatch(group_name, match);
}

void HighlighterEngine::addMatchFromFile(std::string filename) {
	std::ifstream file(filename);

	if (!file.is_open()) return;

	std::shared_ptr<SyntaxGroup> group = std::make_shared<SyntaxGroup>();

	std::string line;
	std::string first_word;
	std::string curr_group;
	while (std::getline(file, line)) {
		if (line.empty() || line[0] == '#') continue; // Comments an empty Lines

		first_word = line.substr(0, line.find_first_of(" \t"));

		if (first_word == "matches") {
			// Language names defined
			std::vector<std::string> toks = splitStr(line, " \t");
			if (toks.empty()) continue;
			for (auto it = toks.begin() + 1; it != toks.end(); it++) {
				languages.emplace(*it, group);
			}
			continue;
		}
		if (first_word == "extends") {
			// Build upon another lang
			// TODO
			continue;
		}

		if (!first_word.empty()) {
			// New Group
			curr_group = first_word;
			continue;
		}

		if (curr_group.empty()) continue;
		// Possibly new Match

		size_t start = line.find_first_not_of(" \t");
		if (start == std::string::npos) continue;
		size_t end = line.find_last_not_of(" \t");
		first_word = line.substr(start, end == std::string::npos ? end : end - start + 1);
		group->addMatch(curr_group, first_word);
	}
}

typedef std::string::const_iterator string_iterator;
typedef std::regex_iterator<string_iterator> regex_iterator;
typedef std::pair<std::string, regex_iterator> group_regex_iterator;

std::string HighlighterEngine::highlight(std::string input, std::string language, std::function<void(std::string&, const std::string&, const std::string&)> processor) {
	std::string result;
	string_iterator input_begin = input.begin();
	std::set<group_regex_iterator, std::function<bool(const group_regex_iterator &, const group_regex_iterator &)>>
		matches([](const group_regex_iterator & l, const group_regex_iterator & r) {
		return l.second->position() < r.second->position();
	});
	regex_iterator rend;

	addLanguage(language);
	auto & lang = languages[language];

	for (auto & group : lang->groups) {
		for (auto & match : lang->matches[group]) {
			regex_iterator rit(input_begin, input.end(), match);
			while (rit != rend) {
				matches.emplace(group, rit);
				rit++;
			}
		}
	}

	for (auto & match : matches) {
		// Translate Match into Iterators
		string_iterator match_begin = input.begin() + match.second->position();
		string_iterator match_end = match_begin + match.second->length();

		// If we are already over this match, continue
		if (std::distance(input_begin, match_begin) < 0) continue;

		// Add everything that didnt matched
		result.append(input_begin, match_begin);

		// Then call the first Match
		processor(result, match.first, match.second->str());
		// Advance input
		input_begin = match_end;
	}

	result.append(input_begin, input.cend());

	return result;
}

void HighlighterEngine::highlight_callback(std::string input, std::string language, std::function<void(std::string, const std::string&)> writer) {
	std::string result;
	string_iterator input_begin = input.begin();
	std::set<group_regex_iterator, std::function<bool(const group_regex_iterator &, const group_regex_iterator &)>>
		matches([](const group_regex_iterator & l, const group_regex_iterator & r) {
		return l.second->position() < r.second->position();
	});
	regex_iterator rend;

	addLanguage(language);
	auto & lang = languages[language];

	for (auto & group : lang->groups) {
		for (auto & match : lang->matches[group]) {
			regex_iterator rit(input_begin, input.end(), match);
			while (rit != rend) {
				matches.emplace(group, rit);
				rit++;
			}
		}
	}

	for (auto & match : matches) {
		// Translate Match into Iterators
		string_iterator match_begin = input.begin() + match.second->position();
		string_iterator match_end = match_begin + match.second->length();

		// If we are already over this match, continue
		if (std::distance(input_begin, match_begin) < 0) continue;

		// Add everything that didnt matched
		result.append(input_begin, match_begin);

		writer(std::string(input_begin, match_begin), "");

		// Then call the first Match
		writer(match.second->str(), match.first);
		// Advance input
		input_begin = match_end;
	}

	writer(std::string(input_begin, input.cend()), "");
}