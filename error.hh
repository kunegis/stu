#ifndef ERROR_HH
#define ERROR_HH

/* Code for managing errors in Stu.  Errors codes in Stu are represented
 * by integers between 0 and 4, as defined in the ERROR_* constants
 * below.  Zero represents no error.  These codes are used for both
 * Stu's exit code, as well as internally in variables named "error". 
 */ 

/* Format of error output:  There are two types of error output lines:
 * error messages and traces.  Error message are of the form "$0: ***
 * $MESSAGE" and traces are of the form "$FILENAME:$LINE:$COLUMN:
 * $MESSAGE".  Traces are used when it is possible to refer to a
 * specific location in the input files (or command line, etc.).  
 * Errors are avoided when possible: all errors should be traced back to
 * a place in the source if possible.    
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

#define ERROR_BUILD     1
#define ERROR_LOGICAL   2
#define ERROR_SYSTEM    4

/* Build errors (code 1) are the most common errors encountered by Stu:
 * commands that fail.  Stu will continue execution after a build error
 * when the -k option is used.  Build errors indicate errors in the
 * programs called by Stu. 
 *
 * Logical errors (code 2) are mostly syntax errors in the source code,
 * but also things like cycles in the dependency graph.  Logical errors
 * indicate errors within the Stu files.  Logical errors may or may not
 * make Stu abort execution.
 * 
 * System errors (code 4) are errors indicating system-level problems,
 * and always lead Stu to abort.  For instance, they happen when Stu
 * cannot access a source file. 
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

// TODO replace these with variadic macros that call fprintf directly to
// save the call to fmt() in the callers.  (Dubious optimization) 

/* Print an error without place */
void print_error(string text)
{
	assert(text != "" && isupper(text.at(0)));
	fprintf(stderr, "%s: *** %s\n", dollar_zero, text.c_str()); 
}

void print_info(string text)
{
	assert(text != "" && isupper(text.at(0)));
	fprintf(stderr, "%s: %s\n", dollar_zero, text.c_str()); 
}

void print_warning(string text)
{
	assert(text != "" && isupper(text.at(0)));
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
		P_FILE,   /* Fields as documented below */
		P_ARGV    /* FILENAME is argument; rest is unused */ 
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

	/* File place */ 
	Place(string filename_, unsigned line_, unsigned column_)
		:  type(P_FILE),
		   filename(filename_),
		   line(line_),
		   column(column_)
	{ 
		assert(line >= 1);
	}

	/* Argv place */
	Place(string string_)
		:  type(P_ARGV),
		   filename(string_)
	{ }

	/* Print the trace to STDERR as part of an error message.  The 
	 * trace is printed as a single line, which can be parsed by
	 * tools, e.g. the compile mode of Emacs.  Line and column
	 * numbers are output as 1-based values. 
	 */
	void operator<< (string message) const; 

	/* Print the beginning of the line, with the place and the
	 * whitespace, but not any message. */ 
	void print_beginning() const;
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
	switch (type) {
	default:  assert(0); 
	case P_EMPTY:
		fprintf(stderr, "<empty>: "); 
		break;
	case P_FILE:
		fprintf(stderr, "%s:%u:%u: ", 
				filename.c_str(), line, 1 + column);  
		break;
	case P_ARGV:
		fprintf(stderr, 
				"Command line argument: '%s': ",
				filename.c_str()); 
		break;
	}
}

#endif /* ! ERROR_HH */ 
