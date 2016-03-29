#ifndef ERROR_HH
#define ERROR_HH

/* Code for managing errors in Stu.  Errors codes in Stu are represented
 * by integers between 0 and 4, as defined in the ERROR_* constants
 * below.  Zero represents no error.  These codes are used for both
 * Stu's exit code, as well as internally in variables named "error". 
 */ 

/* Format of error output:  There are two types of error output lines:
 * error messages and traces.  Error messages are of the form 
 *
 *         $0: *** $MESSAGE
 * 
 * and traces are of the form 
 *
 *         $FILENAME:$LINE:$COLUMN:$MESSAGE
 *
 * Traces are used when it is possible to refer to a specific location
 * in the input files (or command line, etc.).  Errors are avoided when
 * possible: all errors should be traced back to a place in the source
 * if possible.
 */

/* Wording of messages:  Error messages begin with uppercase letters;
 * trace messages with lowercase letters.  Filenames and operator names
 * are quoted in messages using single quotes.  Messages for both types of
 * error output lines are not terminated by periods.  
 *
 * Use "expected TOKEN" instead of "missing TOKEN".  (Because some
 * tokens in the given list may be optional, making the "missing"
 * phrasing confusing, as it would imply that the token is mandatory.)
 * Include articles after "expected" to avoid interpreting "expected" as
 * an adjective.  Don't mention the invalid token, i.e., don't use
 * phrases such as "invalid token". 
 * 
 * Use "must not" instead of "cannot" in error messages, e.g., "filename
 * must not be empty".  But remember that in general it is better to
 * state what what expected in the syntax than to say that what was
 * encountered cannot be used.  For instance, say "expected a filename"
 * instead of "filename must not be empty". 
 */

#include <assert.h>

#include "global.hh"
#include "frmt.hh"

#define ERROR_BUILD     1
#define ERROR_LOGICAL   2
#define ERROR_SYSTEM    4

/* Build errors (code 1) are the most common errors encountered by Stu:
 * commands that fail, files that cannot be accessed, etc.  Stu will
 * continue execution after a build error when the -k option is used.
 *
 * Logical errors (code 2) are mostly syntax errors in the source code,
 * but also things like cycles in the dependency graph.  Logical errors
 * indicate errors within the Stu files.  Logical errors may or may not
 * make Stu abort execution.
 * 
 * System errors (code 4) are errors that lead Stu to abort immediately,
 * even when the -k option is used.  
 * 
 * Build and logical errors can be combined to give error code 3.
 * System errors are never combined with other errors as they make Stu
 * abort immediately. 
 */

/* All thrown objects are integers that represent the errors. 
 */

/* The following functions print messages that do not include a place.
 * The text should begin with an uppercase letter, not end in a period
 * and also not end in a newline character. 
 */ 

/* Print an error without place */
void print_error(string text)
{
	assert(text != "");
	assert(isupper(text.at(0)) || text.at(0) == '\''); 
	assert(text.at(text.size() - 1) != '\n'); 
	fprintf(stderr, "%s: *** %s\n", dollar_zero, text.c_str()); 
}

void print_info(string text)
{
	assert(text != "");
	assert(isupper(text.at(0)) || text.at(0) == '\''); 
	assert(text.at(text.size() - 1) != '\n'); 
	fprintf(stderr, "%s: %s\n", dollar_zero, text.c_str()); 
}

void print_warning(string text)
{
	assert(text != "");
	assert(isupper(text.at(0)) || text.at(0) == '\''); 
	assert(text.at(text.size() - 1) != '\n'); 
	fprintf(stderr, "%s: Warning: %s\n", dollar_zero, text.c_str()); 
}

/* Denotes a position in Stu code.  This is either in a file or in
 * arguments to Stu.  A Place object can also be empty. 
 */ 
class Place
{
public:

	enum Type {
		P_EMPTY,  /* Empty */
		P_FILE,   /* FILENAME is filename */
		P_ARGV    /* FILENAME is argument */ 
	};

	Place::Type type; 

	/* File in which the error occurred */ 
	string filename;

	/* Line number, one-based */ 
	unsigned line; 

	/* Line number, zero-based.
	 * In output, column numbers are one-based, but they are saved
	 * here as zero-based numbers as these are easier to generate.
	 */ 
	unsigned column; 

	/* An empty place */ 
	Place() 
		:  type(P_EMPTY) 
	{ }

	Place(Type type_,
	      string filename_, 
	      unsigned line_, 
	      unsigned column_)
		:  type(type_),
		   filename(filename_),
		   line(line_),
		   column(column_)
	{ 
		assert(line >= 1);
	}

	/* Print the trace to STDERR as part of an error message.  The 
	 * trace is printed as a single line, which can be parsed by
	 * tools, e.g. the compile mode of Emacs.  Line and column
	 * numbers are output as 1-based values. 
	 */
	void operator<< (string message) const; 

	/* Print the beginning of the line, with the place and the
	 * whitespace, but not any message. */ 
	void print_beginning() const;

	/* Represented as a compact string */
	string as_string() const;

	string as_string_nocolumn() const;
};

/* A place along with a message.  This class is only used when traces
 * cannot be printed immediately.  Otherwise, Place::operator<<() is
 * called directly. 
 */
class Trace
{
public:

	Place place;

	/* The message associated with it.  This may be "". 
	 * If the trace is printed, it must not be empty, and not begin
	 * with an upper-case letter */      
	string message; 

	Trace(const Place &place_, string message_) 
		:  place(place_), message(message_) 
	{ }

	/* Print the trace to STDERR as part of an error message.  The
	 * trace is printed as a single line, which can be parsed by
	 * tools, e.g. the compile mode of Emacs.  Line and column
	 * numbers are output as 1-based values. 
	 */
	void print() const 
	{
		place << message; 
	}

	void print_beginning() const
	{
		place.print_beginning();
	}
};

void Place::operator<<(string text) const
{
	assert(text != "" && ! isupper(text[0])); 

	switch (type) {
	default:  assert(0); 
	case P_FILE:
		fprintf(stderr, "%s:%u:%u: %s\n", 
				filename.c_str(), line, 1 + column, text.c_str());  
		break;
	case P_ARGV:
		fprintf(stderr, 
				"Command line argument: '%s': %s\n",
				filename.c_str(),
				text.c_str()); 
	}
}

void Place::print_beginning() const
{
	string s= as_string(); 
	fprintf(stderr, "%s: ", s.c_str()); 
}

string Place::as_string() const
{
	switch (type) {
	default:  assert(0); 

	case P_EMPTY:
		return "<empty>"; 

	case P_FILE:
		return frmt("%s:%u:%u", 
			    filename.c_str(), line, 1 + column);  

	case P_ARGV:
		return frmt("Command line argument: '%s'",
			    filename.c_str()); 

	}
}

string Place::as_string_nocolumn() const
{
	switch (type) {
	default:  assert(0); 

	case P_EMPTY:
		return "<empty>"; 

	case P_FILE:
		return frmt("%s:%u", 
			    filename.c_str(), line);  

	case P_ARGV:
		return frmt("Command line argument: '%s'",
			    filename.c_str()); 

	}
}

/* Explanation functions:  they output an explanation of a feature of
 * Stu on standard output.  This is used after certain non-trivial error
 * messages. */

void explain_clash() 
{
	fputs("Explanation: A dependency cannot be declared as existence-only\n"
	      "(with '!') and optional (with '?') at the same time, as that would mean\n"
	      "that its command is never executed.\n",
	      stderr); 
}

void explain_file_without_command_with_dependencies()
{
	fputs("Explanation: If a file rule has no command, this means that the file\n"
	      "is always up-to-date whenever its dependencies are up to date.  In general,\n"
	      "this means that the file is generated in conjunction with its dependencies.\n",
	      stderr); 
}

void explain_file_without_command_without_dependencies()
{
	fputs("Explanation: A filename followed by a semicolon declares a file that is\n"
	      "always present.\n",
	      stderr); 
}

void explain_no_target()
{
	fputs("Explanation: There must be either a target given as an argument to Stu\n"
	      "invocation, a -c or -C option, an -f option with a default target, or a file\n"
	      "'main.stu' with a default target.\n",
	      stderr); 
}

#endif /* ! ERROR_HH */ 
