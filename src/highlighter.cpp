#include "highlighter.hpp"

#include <set>
#include <fstream>
#include <stack>

void SyntaxLanguage::addGroup(std::string group_name) {
	for (auto & g : groups)
		if (g == group_name) return;
	
	groups.push_back(group_name);
	matches[group_name] = std::make_unique<SyntaxGroup>();
}

void SyntaxLanguage::removeGroup(std::string group_name) {
	for (auto it = groups.begin(); it != groups.end(); it++)
		if (*it == group_name)
			groups.erase(it);

	matches.erase(group_name);	
}

void SyntaxLanguage::addMatch(std::string group_name, std::string match) {
	addGroup(group_name);
	matches[group_name]->matches.emplace_back(match);
}

void SyntaxLanguage::addMatch(std::string group_name, std::vector<std::string> match) {
	addGroup(group_name);
	auto & list = matches[group_name]->matches;
	for (auto & e : match)
		list.emplace_back(e);
}

void SyntaxLanguage::addMatch(std::string group_name, SyntaxMatch match) {
	addGroup(group_name);
	matches[group_name]->matches.push_back(match);
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
		languages[language] = std::make_shared<SyntaxLanguage>();
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

void HighlighterEngine::addMatch(std::string language, std::string group_name, SyntaxMatch match) {
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

	std::shared_ptr<SyntaxLanguage> group = std::make_shared<SyntaxLanguage>();

	std::string line;
	std::string first_word;
	std::string curr_group;
	bool isOpening, isClosing, isDefault;
	std::shared_ptr<std::unordered_set<std::string>> activeGroups;
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
			std::vector<std::string> toks = splitStr(line, " \t");
			isOpening = false;
			isClosing = false;
			isDefault = false;
			activeGroups = nullptr;
			for (size_t idx = 1; idx < toks.size(); idx++) {
				if (activeGroups != nullptr) {
					activeGroups->insert(toks[idx]);
					continue;
				}
				if (toks[idx] == "open-default") {
					isOpening = true;
					isDefault = true;
					continue;
				}
				if (toks[idx] == "open") {
					isOpening = true;
					continue;
				}
				if (toks[idx] == "close") {
					isClosing = true;
					continue;
				}
				if (toks[idx] == "when") {
					activeGroups = std::make_shared<std::unordered_set<std::string>>();
					continue;
				}
			}
			continue;
		}

		if (curr_group.empty()) continue;
		// Possibly new Match

		size_t start = line.find_first_not_of(" \t");
		if (start == std::string::npos) continue;
		size_t end = line.find_last_not_of(" \t");
		first_word = line.substr(start, end == std::string::npos ? end : end - start + 1);
		group->addMatch(curr_group, SyntaxMatch(first_word, isOpening, isClosing, isDefault, activeGroups));
	}
}

typedef std::string::const_iterator string_iterator;
typedef std::regex_iterator<string_iterator> regex_iterator;
typedef std::tuple<std::string, regex_iterator, SyntaxMatch &> group_regex_iterator;

template<typename T>
bool isSubset(std::unordered_set<T> & set, std::unordered_set<T> & subset) {
	for (const T & e : subset)
		if (set.count(e) == 0)
			return false;
	return true;
}

// std::string HighlighterEngine::highlight(std::string input, std::string language, std::function<void(std::string&, const std::string&, const std::string&)> processor) {
// 	std::string result;
// 	string_iterator input_begin = input.begin();
// 	std::set<group_regex_iterator, std::function<bool(const group_regex_iterator &, const group_regex_iterator &)>>
// 		matches([](const group_regex_iterator & l, const group_regex_iterator & r) {
// 		return l.second->position() < r.second->position();
// 	});
// 	regex_iterator rend;

// 	addLanguage(language);
// 	auto & lang = languages[language];

// 	for (auto & group : lang->groups) {
// 		for (auto & match : lang->matches[group]->matches) {
// 			regex_iterator rit(input_begin, input.end(), match);
// 			while (rit != rend) {
// 				matches.emplace(group, rit);
// 				rit++;
// 			}
// 		}
// 	}

// 	for (auto & match : matches) {
// 		// Translate Match into Iterators
// 		string_iterator match_begin = input.begin() + match.second->position();
// 		string_iterator match_end = match_begin + match.second->length();

// 		// If we are already over this match, continue
// 		if (std::distance(input_begin, match_begin) < 0) continue;

// 		// Add everything that didnt matched
// 		result.append(input_begin, match_begin);

// 		// Then call the first Match
// 		processor(result, match.first, match.second->str());
// 		// Advance input
// 		input_begin = match_end;
// 	}

// 	result.append(input_begin, input.cend());

// 	return result;
// }

void HighlighterEngine::highlight_callback(std::string input, std::string language, std::function<void(std::string, const std::string&)> writer) {
	string_iterator input_begin = input.begin();
	std::set<group_regex_iterator, std::function<bool(const group_regex_iterator &, const group_regex_iterator &)>>
		matches([](const group_regex_iterator & l, const group_regex_iterator & r) {
		return std::get<1>(l)->position() < std::get<1>(r)->position();
	});
	regex_iterator rend;

	addLanguage(language);
	auto & lang = languages[language];

	for (auto & group : lang->groups) {
		for (auto & match : lang->matches[group]->matches) {
			regex_iterator rit(input_begin, input.end(), match.match);
			while (rit != rend) {
				matches.emplace(group, rit, match);
				rit++;
			}
		}
	}


	std::stack<std::string> default_group;
	std::unordered_set<std::string> group_stack;
	for (auto & match : matches) {
		// Translate Match into Iterators
		string_iterator match_begin = input.begin() + std::get<1>(match)->position();
		string_iterator match_end = match_begin + std::get<1>(match)->length();

		// If we are already over this match, continue
		if (std::distance(input_begin, match_begin) < 0) continue;

		auto & matchObj = std::get<2>(match);

		if (matchObj.activeWhen != nullptr && !isSubset(group_stack, *matchObj.activeWhen)) {
			// This match does not apply because the needed groups are not active
			continue;
		}
		if (matchObj.isClosing && group_stack.count(std::get<0>(match)) == 0) {
			// Closing match but not open
			continue;
		}

		writer(std::string(input_begin, match_begin), default_group.top());

		if (matchObj.isClosing) {
			default_group.pop();
			group_stack.erase(std::get<0>(match));
		}
		else if (matchObj.isOpening && matchObj.isDefault) {
			default_group.push(std::get<0>(match));
			group_stack.insert(std::get<0>(match));
		}

		// Then call the first Match
		writer(std::get<1>(match)->str(), std::get<0>(match));
		// Advance input
		input_begin = match_end;
	}

	writer(std::string(input_begin, input.cend()), "");
}

Highlighter HighlighterEngine::getHighlighter(std::string language) {
	addLanguage(language);
	return Highlighter(languages[language]);
}

void Highlighter::operator()(std::string input, std::function<void(std::string, const std::string&)> writer) {
	string_iterator input_begin = input.begin();
	std::set<group_regex_iterator, std::function<bool(const group_regex_iterator &, const group_regex_iterator &)>>
		matches([](const group_regex_iterator & l, const group_regex_iterator & r) {
		return std::get<1>(l)->position() < std::get<1>(r)->position();
	});
	regex_iterator rend;

	for (auto & group : lang->groups) {
		for (auto & match : lang->matches[group]->matches) {
			regex_iterator rit(input_begin, input.end(), match.match);
			while (rit != rend) {
				matches.emplace(group, rit, match);
				rit++;
			}
		}
	}

	for (auto & match : matches) {
		// Translate Match into Iterators
		string_iterator match_begin = input.begin() + std::get<1>(match)->position();
		string_iterator match_end = match_begin + std::get<1>(match)->length();

		// If we are already over this match, continue
		if (std::distance(input_begin, match_begin) < 0) continue;

		auto & matchObj = std::get<2>(match);

		if (matchObj.activeWhen != nullptr && !isSubset(group_stack, *matchObj.activeWhen)) {
			// This match does not apply because the needed groups are not active
			continue;
		}
		if (matchObj.isClosing && group_stack.count(std::get<0>(match)) == 0) {
			// Closing match but not open
			continue;
		}
		std::string debug = std::string(input_begin, match_begin);
		writer(std::string(input_begin, match_begin), default_group.empty() ? "" : default_group.top());

		if (matchObj.isClosing) {
			default_group.pop();
			group_stack.erase(std::get<0>(match));
		}
		else if (matchObj.isOpening && matchObj.isDefault) {
			default_group.push(std::get<0>(match));
			group_stack.insert(std::get<0>(match));
		}

		// Then call the first Match
		writer(std::get<1>(match)->str(), std::get<0>(match));
		// Advance input
		input_begin = match_end;
	}

	writer(std::string(input_begin, input.cend()), default_group.empty() ? "" : default_group.top());
}