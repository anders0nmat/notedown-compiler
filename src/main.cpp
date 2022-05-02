#include <iostream>
#include <fstream>
#include <filesystem>

#include "notedown-compiler.hpp"

/*
*	--- Adding handlers ---

	To create a new Handler, you create a new class inheriting from ParserHandler or InlineHandler:

	- ParserHandler is for Elements that span over an entire line 
	  and thus start on the beginning of a line.
	- InlineHandler is for Text Elements that can appear everywhere in 
	  Plain text.
!	  Dont extend over multiple lines.

	There you overwrite the following functions as specified:

?	- createNew() : To return a new unique_ptr instance of your class. Typically:
	  > return std::make_unique<clasName>();
?	- triggerChars() : To return a std::string of chars to listen for. If none are specified,
	  they are treated as text and thus parsed as (part of) a paragraph
?	- canHandle(Parser * lex) : Is called to determine whether your Handler applies to the 
	  current situation. Returns a bool. Shall always return fals on tokEOF. You shall not modify 
	  the parser object although you can for functionality reasons.
	  Non-Modifying elements to use: peektok(), lastToken, lastString, lastInt
?	- handle(Parser * lex) : To handle the current situation. Shall return on linebreak 
	  even if the current element is not finished parsing. Always has to leave Parser on next Token,
	  e.g. consuming Newline. Returns a tuple of { unique_ptr, bool },
	  where:
	  - unique_ptr : a unique pointer holding an ASTElement to append to the document,
	    nullptr if nothing should be inserted (yet)
	  - bool : Whether the current element is finished parsing, useful for one-line-objects.
	    Ignored for InlineHandler
?	- finish(Parser * lex) : Is called if next Line is not parsable (canHandle() == false),
	  but handler is not finished (last handle() was { - , false }). Returns unique pointer 
	  holding ASTElement to insert.
	
?	Useful members for Parsing (all regarding Parser object):
?	Functions:
	- peektok() : Looks for the next token
	- peekchar() : Returns next character
	- gettok() : Returns next token and advances input. Populates lastToken, lastString and lastInt 
	  with information.
	- parseText(bool, bool, int) : 
	  Returns a tuple { unique_ptr , bool } with the text line, applying inline-styling.
	  See its documentation for more information about return values and parameters.
	- parseLine(ParserHandler &) : Parses a line of input, looks for ParserHandler to apply
	  and appliey them, returns result. Useful for blocks that should otherwise behave like normal
	  syntax

?	Variables:
	- lastToken : Holds the current token
	- lastString : Holds information for the current Token. See enum Token for more information
	- lastInt : Holds counting information for the current token. See enum Token for more information

*	--- Applying Handlers ---

!	Handlers are checked in the order they are added in. 
!	As soon as a Handler returns true on canHandle() no further Handlers are checked.
	Add a handler with the addHandler() function, it has to have a unique name
	Naming conventions are: 
	H_<lowercase name> for ParserHandlers
	I_<lowercase name> for InlineHandlers

?	If no valid Handler is found but EOF is not reached, H_default is called.

	addDefaultHandlers() adds all sorts of handlers that are considered default:
	(Work in Progress)
	H_paragraph (H_default) : Parses paragraphs of text, ending on a empty line
	H_heading : Parses Headings of Syntax "#"(1x-6x) <Space> <Text> <Newline>

	I_bold : Prints text of Syntax "*" <Text> "*" as bold text
	I_italic : Prints text of Syntax "/" <Text> "/" as italic text
*/

std::string fmtNow(const char * fmt = "%c") {
	static char buf[256];
	time_t t;
	struct tm * timeinfo;

	time(&t);
	timeinfo = localtime(&t);
	strftime(buf, 256, fmt, timeinfo);
	return buf;
}

std::unique_ptr<ASTUnorderedList> tocLevel(std::vector<ASTHeading *> & outline, size_t & index) {
	int level = outline[index]->level;
	std::unique_ptr<ASTUnorderedList> list = std::make_unique<ASTUnorderedList>();
	while (index < outline.size() && outline[index]->level >= level) {
		if (outline[index]->level == level) {
			// New Entry
			std::string name = outline[index]->content->literalText();
			std::string id = outline[index]->commands.id;
			std::unique_ptr<ASTListElement> elem = std::make_unique<ASTListElement>();
			std::unique_ptr<ASTInlineText> text = std::make_unique<ASTInlineText>();
			text->addElement(std::make_unique<ASTPlainText>(name));
			elem->addElement(std::make_unique<ASTHeadingLink>(id, text));
			list->addElement(elem);
			index++;
		}
		else {
			std::unique_ptr<_ASTElement> elem = std::move(tocLevel(outline, index));
			list->back()->addElement(elem);
		}
	}
	return std::move(list);
}

const std::string COMPILER_VERSION = "v1.2";
const std::string DEFAULT_STYLE_PATH = "default.css";
const std::string CONSOLE_HELP = 
"\t-?\n"
"\t-h\n"
"\t-help\t: Prints this help\n"
"\t-nom\n"
"\t-nomultithread\t: Disables the multithread compiling of files\n"
"\t-sd\n"
"\t-styledoc\n"
"\t-styledocument\t: Include styling in html instead of reflinks\n" 
"\t-o\n"
"\t-out\t: Defines the output file\n"
"\t-s\n"
"\t-style\t: Defines css style sheets to use\n"
"\t-nods\n"
"\t-nodefaultstyle\t: Disables the default stylesheet\n"
"\t-node\n"
"\t-nodefaultemoji\t: Disables the default emoji LUTs\n"
"\t-nod\n"
"\t-nodefault\t: Disable default css and emoji LUT\n"
"\t-q\n"
"\t-quiet\t: Disables debug CLI Output\n"
"\t-oc\n"
"\t-outconsole\t: Result will be outputted to console\n"
;

std::vector<std::string> files, styles, emoji;
std::unordered_map<std::string, std::string> args;
std::string ofile;
std::string exe_path;

bool flagSet(std::string str) {
	return args.count(str) > 0;
}

bool flagSet(std::initializer_list<std::string> strs) {
	for (auto & s : strs)
		if (args.count(s) > 0)
			return true;
	return false;
}

int main(int argc, const char *argv[]) {
	exe_path = std::filesystem::path(argv[0]).parent_path().u8string();
	bool ignoreNext;
	for (int i = 1; i < argc; i++) {
		if (ignoreNext){
			ignoreNext = false;
			continue;
		}
		std::string arg = argv[i];
		

		if (arg == "-style" || arg == "-s") {
			if (i + 1 < argc) {
				styles.push_back(argv[i + 1]);
				ignoreNext = true;
			}
			continue;
		}

		if (arg == "-emoji" || arg == "-e") {
			if (i + 1 < argc) {
				emoji.push_back(argv[i + 1]);
				ignoreNext = true;
			}
			continue;
		}

		if (arg == "-out" || arg == "-o") {
			if (i + 1 < argc) {
				ofile = argv[i + 1];
				ignoreNext = true;
			}
			continue;
		}


		if (arg[0] == '-') {
			args[arg] = "";
			continue;
		}

		files.push_back(arg);
	}

	if (files.empty())
		args["-?"] = "";

	bool notQuiet = !flagSet({"-q", "-quiet"});

	if (notQuiet)
		std::cout << "\nNotedown Compiler " + COMPILER_VERSION << std::endl;

	// Print Help
	if (flagSet({ "-?", "-h", "-help" })) {
		std::cout << CONSOLE_HELP << std::endl;
		return 0;
	}


	if (flagSet({"-nod", "-nodefault"})) {
		args["-nods"] = "";
		args["-node"] = "";
	}

	if (ofile.empty()) {
		std::string filename = files[0].substr(0, files[0].find_last_of('.'));
		ofile = filename + ".html";
	}

	if (!flagSet({"-nods", "-nodefaultstyle"}))
		styles.push_back(exe_path + "\\" + DEFAULT_STYLE_PATH);

	if (!flagSet({"-node", "-nodefaultemoji"}))
		emoji.insert(emoji.end(), {
			exe_path + "/emoji_lut/unicode-14/activities",
			exe_path + "/emoji_lut/unicode-14/animals-nature",
			exe_path + "/emoji_lut/unicode-14/flags",
			exe_path + "/emoji_lut/unicode-14/food-drink",
			exe_path + "/emoji_lut/unicode-14/objects",
			exe_path + "/emoji_lut/unicode-14/people-body",
			exe_path + "/emoji_lut/unicode-14/smileys-emotion",
			exe_path + "/emoji_lut/unicode-14/symbols",
			exe_path + "/emoji_lut/unicode-14/travel-places"
		});


	args["-nom"] = "";
	

	// ------------------------------ //
	// ------- COMPILER START ------- //
	// ------------------------------ //


	NotedownCompiler compiler;

	compiler.addEmojiLUT(emoji);
	compiler.addDefaultHandlers();

	compiler.addGeneratorFunc("now", [](_ASTElement * e, ASTProcess step, std::vector<std::string> & args) {
		if (step != procExecuteMain) return;
		ASTCommandContainer * container = dynamic_cast<ASTCommandContainer *>(e);
		if (container == nullptr) return;

		std::string str;
		if (args.size() > 0)
			str = fmtNow(args[0].c_str());
		else
			str = fmtNow();

		container->setContent(std::make_unique<ASTPlainText>(str));
	});

	compiler.addModifierFunc("notoc", [](_ASTElement * e, ASTProcess step, std::vector<std::string> & args) {
		if (step != procExecutePrep) return;
		ASTHeading * container = dynamic_cast<ASTHeading *>(e);
		if (container == nullptr) return;

		container->commands.flags["notoc"] = "true";
	});

	compiler.addGeneratorFunc("inserttoc", [](NotedownCompiler * comp, _ASTElement * e, ASTProcess step, std::vector<std::string> & args) {
		if (step != procExecuteMain) return;
		ASTCommandContainer * container = dynamic_cast<ASTCommandContainer *>(e);
		if (container == nullptr) return;

		// Go through all headings on main Level and collect all relevant headings
		std::vector<ASTHeading *> outline;

		for (size_t index = 0; index < comp->documentCount(); index++) {
			std::unique_ptr<ASTDocument> & doc = comp->getDocument(index);
			for (auto & e : doc->elements) {
				ASTHeading * h = dynamic_cast<ASTHeading *>(e.get());
				if (h != nullptr && !h->isEmpty() && h->commands.flags["notoc"] != "true")
					outline.push_back(h);
			}
		}
		if (outline.empty()) return;

		std::unique_ptr<ASTUnorderedList> list = std::make_unique<ASTUnorderedList>();

		int currLevel;
		size_t index = 0;

		while (index < outline.size()) {
			currLevel = outline[index]->level;
			std::unique_ptr<ASTUnorderedList> l = tocLevel(outline, index);
			list->addElements(l->elements);
		}

		container->setContent(std::move(list));
	});

	compiler.addModifierFunc("usenum", [](_ASTElement * e, ASTProcess step, std::vector<std::string> & args) {
		if (step != procExecuteMain) return;
		ASTListElement * container = dynamic_cast<ASTListElement *>(e->parent);
		if (container == nullptr) return;

		unsigned long num = container->index;
		if (args.size() > 0)
			if (isdigit(args[0][0]))
				num = std::stoul(args[0]);
		
		container->commands.attributes["value"] = std::to_string(num);
	});


	compiler.addFile(files, !flagSet({ "-nomultithread", "-nom" }));
	compiler.prepareAST();


	std::ofstream html;
	std::ostream * outBuf = nullptr;

	if (flagSet({"-oc", "-outconsole"})) {
		outBuf = &std::cout;
	}
	else {
		html.open(ofile);
		outBuf = &html;
	}
	

	*outBuf << "<!DOCTYPE html>\n" << "<html>\n" << "<head>\n";
	*outBuf << "<meta charset=\"utf-8\">\n";

	for (auto & style : styles) {
		if (flagSet({ "-sd", "-styledoc", "-styledocument" })) {
			std::ifstream css(style);
			if (!css.is_open()) {
				std::cerr << "Style could not be opened: " << style << std::endl;
				continue;
			}
			*outBuf << "<!-- " << style << " -->\n";
			*outBuf << "<style>\n";
			*outBuf << css.rdbuf();
			*outBuf << "</style>\n";

		}
		else
			*outBuf << "<link rel=\"stylesheet\" href=\"" << style << "\">\n";
	}

	*outBuf << "</head>\n" << "<body>\n";
	*outBuf << compiler.getRawHtml();
	*outBuf << "</body>\n" << "</html>\n";
	
	html.close();

	if (notQuiet)
		std::cout << "Success!" << "\n" << "New File: " << ofile << std::endl;
	
	return 0;
}
