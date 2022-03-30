#include <iostream>
#include <fstream>

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

int main(int argc, char *argv[]) {
	NotedownCompiler compiler;
	compiler.addDefaultHandlers();
	compiler.addFile({ "example.nd", "example2.nd" });
	compiler.prepareAST();

	// std::ofstream json("AST.json");
	// json << compiler.getDocument(0)->toJson();
	// json.close();

	std::ofstream html("example.html");
	
	// html << "<!DOCTYPE html>\n";
	// html << "<html>\n";
	// html << "<head>\n";
	// html << "<meta charset=\"utf-8\">\n";
	// html << "</head>\n";
	// html << "<body>\n";
	html << compiler.getRawHtml();
	// html << "</body>\n";
	// html << "</html>\n";
	html.close();
	

	return 0;
}