#pragma once

#include <vector>
#include <string>
#include <regex>
#include <unordered_map>
#include <memory>
#include <set>

struct SyntaxGroup {
    std::vector<std::string> groups;
    std::unordered_map<std::string, std::vector<std::regex>> matches;

    void addGroup(std::string group_name);
    void removeGroup(std::string group_name);

    void addMatch(std::string group_name, std::string match);
    void addMatch(std::string group_name, std::initializer_list<std::string> match);
    void addMatch(std::string group_name, std::vector<std::string> match);
};

class HighlighterEngine {
private:
    
    std::unordered_map<std::string, std::shared_ptr<SyntaxGroup>> languages;

public:
    HighlighterEngine(){}

    void addLanguage(std::string language);
    void removeLanguage(std::string language);
    bool hasLanguage(std::string language);

    void addGroup(std::string language, std::string group_name);
    void removeGroup(std::string language, std::string group_name);

    void addMatch(std::string language, std::string group_name, std::string match);
    void addMatch(std::string language, std::string group_name, std::initializer_list<std::string> match);
    void addMatch(std::string language, std::string group_name, std::vector<std::string> match);

    void addMatchFromFile(std::string filename);


    std::string highlight(std::string input, std::string language, std::function<void(std::string&, const std::string&, const std::string&)> processor);
    
    void highlight_callback(std::string input, std::string language, std::function<void(std::string, const std::string&)> writer);
};
