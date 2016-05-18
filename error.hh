#ifndef ERROR_HH
#define ERROR_HH

/* Code for managing errors in Stu.  Errors codes in Stu are represented
 * by integers between 0 and 4, as defined in the ERROR_* constants
 * below.  Zero represents no error.  These codes are used for both
 * Stu's exit code and as value that are thrown and caught.   Variables
 * containing error codes are integer and named "error". 
 */ 

#include <assert.h>

#include "global.hh"
#include "text.hh"

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
 * phrases such as "invalid token '&'". 
 * 
 * Use "must not" rather than "cannot" in error messages, e.g., "filename
 * must not be empty".  But remember that in general it is better to
 * state what what expected in the syntax than to say that what was
 * encountered cannot be used.  For instance, say "expected a filename"
 * instead of "filename must not be empty".  This cannot always be done,
 * so "must not" is sometimes used. 
 */

#define ERROR_BUILD          1
#define ERROR_LOGICAL        2
#define ERROR_FATAL          4

/* Errors 1 and 2 are recoverable.  If the -k option is given, Stu notes
 * these errors and continues.  If the -k option is not given, they
 * cause Stu to abort.  When the -k option is used, the final exit code
 * may combine errors 1 and 2, giving exit code 3.  Error 4 is
 * unrecoverable, and leads to Stu aborting immediately. 
 *
 * Build errors (code 1) are errors encountered during the normal
 * operation of Stu.  They indicate failures of the executed commands or
 * errors with files.  Exit code 1 is also used for the -q option, when
 * the targets are not up to date.  
 *
 * Logical errors (code 2) are errors with the usage of Stu.  These are
 * for instance syntax errors in the source code and cycles in the
 * dependency graph.  
 * 
 * Fatal errors (code 4) are errors that lead Stu to abort immediately,
 * even when the -k option is used.  
 *
 * Build and logical errors can be combined to give error code 3.
 * Fatal errors are never combined with other errors as they make Stu
 * abort immediately. 
 */

/* The following functions print messages that do not include a place.
 * The text should begin with an uppercase letter, not end in a period
 * and also not end in a newline character. 
 */ 

/* Print an error without place */
void print_error(string text)
{
	assert(text != "");
	assert(isupper(text[0]) || text[0] == '\''); 
	assert(text[text.size() - 1] != '\n'); 
	fprintf(stderr, "%s: *** %s\n", dollar_zero, text.c_str()); 
}

void print_info(string text)
{
	assert(text != "");
	assert(isupper(text[0]) || text[0] == '\''); 
	assert(text[text.size() - 1] != '\n'); 
	fprintf(stderr, "%s: %s\n", dollar_zero, text.c_str()); 
}

void print_warning(string text)
{
	assert(text != "");
	assert(isupper(text[0]) || text[0] == '\''); 
	assert(text[text.size() - 1] != '\n'); 
	fprintf(stderr, "%s: Warning: %s\n", dollar_zero, text.c_str()); 
}

/* Denotes a position in Stu code.  This is either in a file or in
 * arguments to Stu.  A Place object can also be empty. 
 */ 
class Place
{
public:

	enum class Type {
		EMPTY,        /* Empty */
		INPUT_FILE,   /* FILENAME is filename */
		ARGV          /* FILENAME is argument */ 
	};

	Place::Type type; 

	/* File in which the error occurred.  Empty string for standard
	 * input.  The command line argument for ARGV. */ 
	string filename;

	/* Line number, one-based.  Ignored for ARGV. */ 
	unsigned line; 

	/* Column number, zero-based.
	 * In output, column numbers are one-based, but they are saved
	 * here as zero-based numbers as these are easier to generate.
	 * Ignored for ARGV. 
	 */ 
	unsigned column; 

	/* An empty place */ 
	Place() 
		:  type(Type::EMPTY) 
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

	/* For command line arguments */ 
	Place(Type type_,
	      string argument)
		:  type(type_),
		   filename(argument)
	{
		assert(type_ == Type::ARGV); 
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

	string as_argv0() const;

	const char *get_filename_str() const;

	bool empty() const { 
		return type == Type::EMPTY;
	}
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
	assert(text != "");
	assert(! isupper(text[0])); 

	switch (type) {
	default:  assert(0); 
	case Type::INPUT_FILE:
		fprintf(stderr, "%s:%u:%u: %s\n", 
			get_filename_str(), line, 1 + column, text.c_str());  
		break;
	case Type::ARGV:
		fprintf(stderr, 
			"Command line argument '%s': %s\n",
			filename.c_str(),
			text.c_str()); 
		break;
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

	case Type::EMPTY:
		return "<empty>"; 

	case Type::INPUT_FILE:
		return frmt("%s:%u:%u", 
			    get_filename_str(),
			    line, 1 + column);  

	case Type::ARGV:
		return frmt("Command line argument '%s'",
			    filename.c_str()); 

	}
}

string Place::as_argv0() const
{
	switch (type) {
	default:  assert(0); 

	case Type::EMPTY:
		return "<empty>"; 

	case Type::INPUT_FILE: {
		const char *s= get_filename_str();
		return frmt("%s%s:%u", 
			    s[0] == '-' ? "file " : "",
			    s,
			    line);  
	}

	case Type::ARGV:
		return frmt("Command line argument '%s'",
			    filename.c_str()); 
	}
}

const char *Place::get_filename_str() const
{
	return filename == ""
		? "<stdin>"
		: filename.c_str();
}

/* Explanation functions:  they output an explanation of a feature of
 * Stu on standard output.  This is used after certain non-trivial error
 * messages. */

void explain_clash() 
{
	if (! option_explain)  return;
	fputs("Explanation: A dependency cannot be declared as existence-only\n"
	      "(with '!') and optional (with '?') at the same time, as that would mean\n"
	      "that its command is never executed.\n",
	      stderr); 
}

void explain_file_without_command_with_dependencies()
{
	if (! option_explain)  return;
	fputs("Explanation: If a file rule has no command, this means that the file\n"
	      "is always up-to-date whenever its dependencies are up to date.  In general,\n"
	      "this means that the file is generated in conjunction with its dependencies.\n",
	      stderr); 
}

void explain_file_without_command_without_dependencies()
{
	if (! option_explain)  return;
	fputs("Explanation: A filename followed by a semicolon declares a file that is\n"
	      "always present.\n",
	      stderr); 
}

void explain_no_target()
{
	if (! option_explain)  return;
	fputs("Explanation: There must be either a target given as an argument to Stu\n"
	      "invocation, a -c or -C option, an -f option with a default target, a file\n"
	      "'main.stu' with a default target, or an -F option.\n",
	      stderr); 
}

void explain_parameter_character()
{
	if (! option_explain)  return;
	fputs("Explanation: Parameter names can only include alphanumeric characters\n"
	      "and underscores.\n",
	      stderr); 
}

void explain_cycle()
{
	if (! option_explain)  return;
	fputs("A cycle in the dependency graph is an error.  Cycles are verified on\n"
	      "the rule level, not on the target level.\n",
	      stderr);
}

#endif /* ! ERROR_HH */ 
