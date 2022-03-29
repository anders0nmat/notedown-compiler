#include <fstream>
#include <thread>

#include "notedown-compiler.hpp"
#include "lexer.hpp"

namespace Notedown {
	std::string makeId(std::string str) {
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
				if (e == '_' || e == '-')
					id += e;

				// Space convert
				if (e == ' ')
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

void NotedownCompiler::addSymbols(std::string str) {
	for (auto e : str)
		symbols.insert(e);
}

void NotedownCompiler::addFile(std::string filename) {
	std::ifstream file(filename, std::ios::in);
	if (!file.is_open())
		throw "File could not be opened";
	
	Parser p(this, &file);

	p.parseDocument();

	documents.push_back(std::move(p.getDocument()));
}

void NotedownCompiler::addFromFile(std::string filename, size_t order) {
	std::ifstream file(filename, std::ios::in);
	if (!file.is_open())
		throw "File could not be opened";
	
	Parser p(this, &file);

	p.parseDocument();

	mtx_documents.lock();
	documents[order] = std::move(p.getDocument());
	mtx_documents.unlock();
}

void NotedownCompiler::addFile(std::initializer_list<std::string> filenames, bool multithread) {
	std::vector<std::thread> threads;
	size_t order = documents.size();
	documents.resize(documents.size() + filenames.size());
	for (auto e : filenames) {
		if (multithread)
			threads.push_back(std::thread(&NotedownCompiler::addFromFile, this, e, order));
		else
			addFile(e);
		order++;
	}

	for(auto & th : threads)
		th.join();
}

void NotedownCompiler::prepareAST() {
	getIdDef();
}

void NotedownCompiler::getIdDef() {
	for (auto & doc : documents) {
		doc->registerNow();
		for (auto & p : doc->iddef)
			iddef[p.first] = p.second;
	}
}

_ASTElement * NotedownCompiler::handleRequest(std::string request) {
	auto it = iddef.find(request);
	if (it == iddef.end())
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
	for (auto & e : documents)
		os << e->getHtml(std::bind(&NotedownCompiler::handleRequest, this, std::placeholders::_1)) << '\n';
	return &buf;
}

std::stringbuf * NotedownCompiler::getRawHtml(size_t index) {
	if (index >= documents.size())
		throw "Index out of bounds";
	std::ostream os(&buf);
	os << documents[index]->getHtml(std::bind(&NotedownCompiler::handleRequest, this, std::placeholders::_1));
	return &buf;
}