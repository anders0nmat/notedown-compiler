#pragma once

#include <vector>
#include <string>
#include <regex>
#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <set>

struct SyntaxMatch {
    std::regex match;
    bool isOpening;
    bool isClosing;
    bool isDefault;

    std::shared_ptr<std::unordered_set<std::string>> activeWhen;

    SyntaxMatch(std::string match) : match(match) {}
    SyntaxMatch(std::string match, bool isOpening, bool isClosing, 
        bool isDefault, std::shared_ptr<std::unordered_set<std::string>> & activeWhen) 
        : match(match), isOpening(isOpening), isClosing(isClosing), 
        isDefault(isDefault), activeWhen(activeWhen) {}
};

struct SyntaxGroup {
    std::string groupName;
    std::vector<SyntaxMatch> matches;
};

struct SyntaxLanguage {
    std::vector<std::string> groups;
    std::unordered_map<std::string, std::unique_ptr<SyntaxGroup>> matches;

    void addGroup(std::string group_name);
    void removeGroup(std::string group_name);

    void addMatch(std::string group_name, std::string match);
    void addMatch(std::string group_name, std::vector<std::string> match);
    void addMatch(std::string group_name, SyntaxMatch match);
};

class Highlighter {
private:
    std::shared_ptr<SyntaxLanguage> lang;
    std::stack<std::string> default_group;
	std::unordered_set<std::string> group_stack;
public:
    Highlighter(std::shared_ptr<SyntaxLanguage> lang) : lang(lang) {}

    void operator()(std::string input, std::function<void(std::string, const std::string&)> writer);
};

class HighlighterEngine {
private:
    
    std::unordered_map<std::string, std::shared_ptr<SyntaxLanguage>> languages;

    void addLangFromFileRecursive(std::string filename, std::shared_ptr<SyntaxLanguage> & lang, std::unordered_set<std::string> & included_files, bool inheritFileType = false);
public:
    HighlighterEngine(){}

    void addLanguage(std::string language);
    void removeLanguage(std::string language);
    bool hasLanguage(std::string language);

    void addGroup(std::string language, std::string group_name);
    void removeGroup(std::string language, std::string group_name);

    void addMatch(std::string language, std::string group_name, std::string match);
    void addMatch(std::string language, std::string group_name, std::vector<std::string> match);
    void addMatch(std::string language, std::string group_name, SyntaxMatch match);

    void addMatchFromFile(std::string filename);


    // std::string highlight(std::string input, std::string language, std::function<void(std::string&, const std::string&, const std::string&)> processor);
    
    void highlight_callback(std::string input, std::string language, std::function<void(std::string, const std::string&)> writer);

    Highlighter getHighlighter(std::string language);
};
