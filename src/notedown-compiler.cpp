#include <fstream>
#include <thread>
#include <algorithm>
#include <iostream>

#include "notedown-compiler.hpp"
#include "lexer.hpp"

namespace Notedown {
	// trim from start (in place)
	static inline void ltrim(std::string &s) {
		s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) {
			return !std::isspace(ch);
		}));
	}

	// trim from end (in place)
	static inline void rtrim(std::string &s) {
		s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) {
			return !std::isspace(ch);
		}).base(), s.end());
	}

	// trim from both ends (in place)
	static inline void trim(std::string &s) {
		ltrim(s);
		rtrim(s);
	}

	std::string makeId(std::string str) {
		trim(str);
		std::string id;
		for (auto e : str) {
			// Letters
			if (65 <= e && e <= 90)
				// Uppercase letter
				id += e + 32;
			if (97 <= e && e <= 122)
				id += e;

			if (!id.empty()) {
				// Numbers
				if (48 <= e && e <= 57)
					id += e;

				// Allowed for html compatibility
				if ((e == '_' || e == '-') && id.back() != e)
					id += e;

				// Space convert
				if (e == ' ' && id.back() != '-')
					id += '-';
			}
		}
		return id;
	}

	bool isId(std::string id) {
		bool invalidChars = id.find_first_not_of("abcdefghijklmnopqrstuvwxyz0123456789_-") == std::string::npos,
			startValid = islower(id[0]);
		return startValid && !invalidChars;
	}
}

bool NotedownCompiler::addHandler(std::string name, std::unique_ptr<ParserHandler> handler) {
	if (handlerAlias.count(name) == 1)
		return false;
	
	addSymbols(handler->triggerChars());
	handlerList.push_back(std::move(handler));
	return handlerAlias.emplace(name, handlerList.size() - 1).second;
}

bool NotedownCompiler::addHandlerAlias(std::string alias, std::string name) {
	if (handlerAlias.count(alias) == 1 || handlerAlias.count(name) == 0)
		return false;
	
	return handlerAlias.emplace(alias, handlerAlias[name]).second;
}

bool NotedownCompiler::addInlineHandler(std::string name, std::unique_ptr<InlineHandler> handler) {
	if (inlineHandlerAlias.count(name) == 1)
		return 0;
	
	addSymbols(handler->triggerChars());
	inlineHandlerList.push_back(std::move(handler));
	return inlineHandlerAlias.emplace(name, inlineHandlerList.size() - 1).second;
}

bool NotedownCompiler::addInlineHandlerAlias(std::string alias, std::string name) {
	if (inlineHandlerAlias.count(alias) == 1 || inlineHandlerAlias.count(name) == 0)
		return false;
	
	return inlineHandlerAlias.emplace(alias, inlineHandlerAlias[name]).second;
}

void NotedownCompiler::addEmojiLUT(std::string filename) {
	std::ifstream file(filename, std::ios::in);
	if (!file.is_open()) {
		std::cerr << "Emoji LUT could not be opened: " << filename << std::endl;
		return;
	}

	while (!file.eof()) {
		std::string line;
		std::getline(file, line);
		std::string emoji;
		std::string shortcode;
		size_t oldPos = 0, pos = line.find_first_of(' ');
		if (pos != std::string::npos && pos != oldPos) {
			emoji = line.substr(0, pos);
			if (!emoji.empty()) {
				oldPos = pos;
				pos = line.find_first_of(' ', oldPos + 1);
				while (pos != std::string::npos) {
					shortcode = line.substr(oldPos + 1, pos - oldPos - 1);
					if (!shortcode.empty())
						emojis.emplace(shortcode, emoji);
					oldPos = pos;
					pos = line.find_first_of(' ', oldPos + 1);
				}
				shortcode = line.substr(oldPos + 1, pos - oldPos - 1);
				if (!shortcode.empty())
					emojis.emplace(shortcode, emoji);
			}
		}
	}
}

void NotedownCompiler::addEmojiLUT(std::vector<std::string> filenames) {
	for (auto & e : filenames)
			addEmojiLUT(e);
}

void NotedownCompiler::addSymbols(std::string str) {
	for (auto e : str)
		symbols.insert(e);
}

void NotedownCompiler::addFile(std::string filename) {
	documents.emplace_back(nullptr); // Create one empty element
	addFromFile(filename, documents.size() - 1);
}

void NotedownCompiler::addFromFile(std::string filename, size_t order) {
	std::ifstream file(filename, std::ios::in);
	if (!file.is_open()) {
		std::cerr << "File could not be opened: " << filename << std::endl;
		return;
	}
	
	Parser p(this, &file);

	p.parseDocument();
	p.getDocument()->commands.flags["_filename"] = filename;

	mtx_documents.lock();
	documents[order] = std::move(p.getDocument());
	mtx_documents.unlock();
}

void NotedownCompiler::addFile(std::vector<std::string> & filenames, bool multithread) {
	std::vector<std::thread> threads;
	size_t order = documents.size();
	documents.resize(documents.size() + filenames.size());
	for (auto e : filenames) {
		if (multithread)
			threads.push_back(std::thread(&NotedownCompiler::addFromFile, this, e, order));
		else
			addFromFile(e, order);
		order++;
	}

	for(auto & th : threads)
		th.join();
}

void NotedownCompiler::prepareAST() {
	for (auto & e : documents) {
		e->process(procRegister, 
			std::bind(&NotedownCompiler::handleRequest, this, std::placeholders::_1), 
			std::bind(&NotedownCompiler::handleModRequest, this, std::placeholders::_1));
		for (auto & p : e->iddef)
			iddef[p.first] = p.second;	
	}
	
	for (auto & e : documents) {
		e->process(procResolve, 
			std::bind(&NotedownCompiler::handleRequest, this, std::placeholders::_1), 
			std::bind(&NotedownCompiler::handleModRequest, this, std::placeholders::_1));
		for (auto & p : e->iddef)
			iddef[p.first] = p.second;
	}

	for (auto & e : documents) {
		e->process(procConsume, 
			std::bind(&NotedownCompiler::handleRequest, this, std::placeholders::_1), 
			std::bind(&NotedownCompiler::handleModRequest, this, std::placeholders::_1));
		for (auto & p : e->iddef)
			iddef[p.first] = p.second;
	}

	// Consume all registered components
	// for (auto & e : documents)
	// 	for (auto & p : e->iddef)
	// 		iddef[p.first] = p.second;

	for (auto & e : documents) {
		e->process(procExecutePrep, 
			std::bind(&NotedownCompiler::handleRequest, this, std::placeholders::_1), 
			std::bind(&NotedownCompiler::handleModRequest, this, std::placeholders::_1));
	}
	
	for (auto & e : documents) {
		e->process(procExecuteMain, 
			std::bind(&NotedownCompiler::handleRequest, this, std::placeholders::_1), 
			std::bind(&NotedownCompiler::handleModRequest, this, std::placeholders::_1));
	}

	for (auto & e : documents) {
		e->process(procExecutePost, 
			std::bind(&NotedownCompiler::handleRequest, this, std::placeholders::_1), 
			std::bind(&NotedownCompiler::handleModRequest, this, std::placeholders::_1));
	}
}

void NotedownCompiler::getIdDef() {
	for (auto & doc : documents) {
		doc->registerNow();
		for (auto & p : doc->iddef)
			iddef[p.first] = p.second;
	}
}

_ASTElement * NotedownCompiler::handleRequest(std::string request) {
	if (request[0] == ':') {
		auto it = emojis.find(request.substr(1));
		if (it == emojis.end())
			return nullptr;
		temp_emoji.content = it->second;
		return &temp_emoji;
	}
	
	auto it = iddef.find(request);
	if (it == iddef.end())
		return nullptr;
	return it->second;
}

ASTModFunc NotedownCompiler::handleModRequest(std::string request) {
	auto it = modFuncs.find(request);
	if (it == modFuncs.end())
		return nullptr;
	return it->second;
}

std::unique_ptr<ASTDocument> & NotedownCompiler::getDocument(size_t index) {
	if (documents.size() <= index)
		throw "Index out of range";
	return documents[index];
}

std::stringbuf * NotedownCompiler::getRawHtml() {
	std::ostream os(&buf);
	for (auto & e : documents) {
		os << "<!-- " << e->commands.flags["_filename"] << " -->\n";
		os << e->getHtml(std::bind(&NotedownCompiler::handleRequest, this, std::placeholders::_1)) << '\n';
	}
	return &buf;
}

std::stringbuf * NotedownCompiler::getRawHtml(size_t index) {
	if (index >= documents.size())
		throw "Index out of bounds";
	std::ostream os(&buf);
	os << documents[index]->getHtml(std::bind(&NotedownCompiler::handleRequest, this, std::placeholders::_1));
	return &buf;
}

void NotedownCompiler::addModifierFunc(std::string name, ASTModFunc func) {
	modFuncs.emplace("&" + name, func);
}

void NotedownCompiler::addModifierFunc(std::string name, std::function<void(NotedownCompiler *, _ASTElement *, ASTProcess, std::vector<std::string> &)> func) {
	addModifierFunc(name, std::bind(func, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
}

void NotedownCompiler::addGeneratorFunc(std::string name, ASTModFunc func) {
	modFuncs.emplace("$" + name, func);
}

void NotedownCompiler::addGeneratorFunc(std::string name, std::function<void(NotedownCompiler *, _ASTElement *, ASTProcess, std::vector<std::string> &)> func) {
	addGeneratorFunc(name, std::bind(func, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
}

size_t NotedownCompiler::documentCount() {
	return documents.size();
}
