#ifndef ERROR_HH
#define ERROR_HH

/*
 * Code for managing errors in Stu.  Errors codes in Stu are represented
 * by integers between 0 and 4, as defined in the ERROR_* constants
 * below.  Zero represents no error.  These codes are used for both
 * Stu's exit code and as values that are thrown and caught.   Variables
 * containing error codes are ints and are named "error". 
 */ 

/*
 * Format of error output:  There are two types of error output lines:
 * error messages and traces.  Error messages are of the form 
 *
 *         $0: *** $MESSAGE
 * 
 * and traces are of the form 
 *
 *         $FILENAME:$LINE:$COLUMN: $MESSAGE
 *
 * Traces are used when it is possible to refer to a specific location
 * in the input files (or command line, etc.).  Error messages are
 * avoided: all errors should be traced back to a place in the source if
 * possible. But sometimes they must be used. 
 */

/*
 * Wording of messages:  Error messages begin with uppercase letters;
 * trace messages with lowercase letters, as per the GNU Coding
 * Standard.  Filenames and operator names are quoted in messages using
 * single quotes.  Messages for both types of error output lines are not
 * terminated by periods.   
 *
 * The general forms of error messages is:
 *
 *    Location 2: expected XXX, not YYY
 *    Location 1: in ZZZ	
 *
 *    Location 2: XXX must not be used
 *    Location 1: in YYY 
 *
 * Use "expected TOKEN" instead of "missing TOKEN".  That's because some
 * tokens in the given list may be optional, making the "missing"
 * phrasing confusing, as it would imply that the token is mandatory.
 * Include definite or indefinite articles after "expected" to avoid
 * interpreting "expected" as an adjective.  
 *
 * "not YYY" mentions the invalid token.  If end-of-file is encountered,
 * the "not ..." part is not used. 
 * 
 * Use "must not" rather than "cannot" or "shall" in error messages when
 * something must be present, but is erroneous, e.g., "filename must not
 * be empty".  On the other hand, use "cannot" when something completely
 * unexpected was encountered, e.g., "transient targets cannot be used
 * with copy rule".
 * 
 * But remember that in general it is better to state what what expected
 * in the syntax than to say that what was encountered cannot be used.
 * For instance, say "expected a filename" instead of "filename must not
 * be empty".  This cannot always be done, so "must not" is sometimes
 * used.  
 */

#include <assert.h>

#include "global.hh"
#include "text.hh"
#include "color.hh"
#include "format.hh"

/* The error constants.  Not declared as an enum because they are thrown
 * and thus need to be integers.  */
const int ERROR_BUILD=     1;
const int ERROR_LOGICAL=   2;
const int ERROR_FATAL=     4;

/*
 * Errors 1 and 2 are recoverable.  If the -k option is given, Stu notes
 * these errors and continues.  If the -k option is not given, they
 * cause Stu to abort.  When the -k option is used, the final exit code
 * may combine errors 1 and 2, giving exit code 3.  Error 4 is
 * unrecoverable, and leads to Stu aborting immediately.  Error 4 is
 * never combined.  
 *
 * Build errors (code 1) are errors encountered during the normal
 * operation of Stu.  They indicate failures of the executed commands or
 * errors with files.  Exit code 1 is also used for the -q option
 * (question mode), when the targets are not up to date.  
 *
 * Logical errors (code 2) are errors with the usage of Stu.  These are
 * for instance syntax errors in the source code, cycles in the
 * dependency graph, or multiple matching rules.  
 * 
 * Fatal errors (code 4) are errors that lead Stu to abort immediately,
 * even when the -k option is used.  They are avoided as much as
 * possible. 
 *
 * Build and logical errors can be combined to give error code 3.
 * Fatal errors are never combined with other errors as they make Stu
 * abort immediately. 
 */

/*
 * The following functions print messages that do not include a place.
 * The text must begin with an uppercase letter, not end in a period
 * and also not end in a newline character.  All are printed to standard
 * error output. 
 */ 

/* Print an error without place */
void print_error(string message)
{
	assert(message != "");
	assert(isupper(message[0]) || message[0] == '\''); 
	assert(message[message.size() - 1] != '\n'); 
	fprintf(stderr, "%s%s%s: *** %s\n", 
		Color::error_word, dollar_zero, Color::end,
		message.c_str()); 
}

/* Like perror(), but use color.  MESSAGE must not contain color codes. */ 
void print_error_system(string message)
{
	assert(message.size() > 0 && message[0] != '') ;
	string t= name_format_word(message); 
	fprintf(stderr, "%s: %s\n",
		t.c_str(),
		strerror(errno));
}

void print_info(string message)
{
	assert(message != "");
	assert(isupper(message[0]) || message[0] == '\''); 
	assert(message[message.size() - 1] != '\n'); 
	fprintf(stderr, "%s%s%s: %s\n", 
		Color::warning, dollar_zero, Color::end,
		message.c_str()); 
}

/* System error message.  Includes the given message, and the
 * ERRNO-based text.  Cf. perror().  Color is not added.  The output of
 * this function is used as input to one of the print_*() functions. 
 */
string system_format(string text)
{
	return fmt("%s: %s",
		   text,
		   strerror(errno)); 
}

/* Print a message to standard output.  This is used in only very few
 * cases, in defiance of the principle that a program should by default
 * only output something when there is an error.  We do it mostly
 * because Make does it, and users expect it. 
 * These messages are suppressed by the -Q option (quiet).  */
void print_out(string text)
{
	assert(text != "");
	assert(isupper(text[0]));
	assert(text[text.size() - 1] != '\n');

	if (option_quiet)
		return; 

	printf("%s%s%s\n",
	       Color::out_print,
	       text.c_str(),
	       Color::out_end); 
}

/* 
 * Denotes a position in Stu source code.  This is either in a file or in
 * arguments/options to Stu.  A Place object can also be empty, which is
 * used as the "uninitialized" value.  
 *
 * Places are used to show the location of an error on standard error
 * output. 
 */ 
class Place
{
public:

	enum class Type {
		EMPTY,        /* Empty "Place" object */
		INPUT_FILE,   /* In a file, with line/column numbers */
		ARGUMENT,     /* Command line argument (outside options) */ 
		OPTION,       /* In an option */
		ENV_OPTIONS   /* In $STU_OPTIONS */
	} type;

	/* 
	 * INPUT_FILE:  Name of the file in which the error occurred.
	 *              Empty string for standard input.  
	 * OPTION:  Name of the option (a single character)
	 * Others:  Unused.  
	 */ 
	string text;

	/* INPUT_FILE:  Line number, one-based.  
	 * Others:  unused.  */ 
	unsigned line; 

	/* INPUT_FILE:  Column number, zero-based.  In output, column
	 * numbers are one-based, but they are saved here as zero-based
	 * numbers as these are easier to generate. 
	 * Others: Unused.  */ 
	unsigned column; 

	/* Empty */ 
	Place() 
		:  type(Type::EMPTY) 
	{ }

	/* Generic constructor */ 
	Place(Type type_,
	      string filename_, 
	      unsigned line_, 
	      unsigned column_)
		:  type(type_),
		   text(filename_),
		   line(line_),
		   column(column_)
	{ 
		assert(line >= 1);
	}

	/* In command line argument (ARGV) */ 
	Place(Type type_)
		:  type(type_)
	{
		assert(type == Type::ARGUMENT); 
	}

	/* In an option (OPTION) */
	Place(Type type_, char option)
		:  type(type_),
		   text(string(&option, 1))
	{ 
		assert(type == Type::OPTION); 
	}
	

	Type get_type() const { return type; }
	const char *get_filename_str() const;

	/* Print the trace to STDERR as part of an error message.  The 
	 * trace is printed as a single line, which can be parsed by
	 * tools, e.g. the compile mode of Emacs.  Line and column
	 * numbers are output as 1-based values.  Return THIS.  */
	const Place &operator<<(string message) const; 

	/* Print a message.  The COLOR arguments determine whether this
	 * is an error or a warning.  */ 
	void print(string message,
		   const char *color,
		   const char *color_word) const;

	/* The string used for the argv[0] parameter of child processes.
	 * Does not include color codes.  Returns "" when no special
	 * string should be used.  */
	string as_argv0() const;

	bool empty() const { 
		return type == Type::EMPTY;
	}

};

/* A place along with a message.  This class is only used when traces
 * cannot be printed immediately.  Otherwise, Place::operator<<() is
 * called directly.  */
class Trace
{
public:

	Place place;

	/* The message associated with it.  This may be "". 
	 * When the trace is printed, it must not be empty, and not begin
	 * with an upper-case letter.  */
	string message; 

	Trace(const Place &place_, string message_) 
		:  place(place_), message(message_) 
	{  }

	/* Print the trace to STDERR as part of an error message; see
	 * Place::operator<< for format information.  */
	void print() const 
	{
		place << message; 
	}
};

const Place &Place::operator<<(string message) const
{
	print(message, Color::error, Color::error_word); 
	return *this;
}

void Place::print(string message, 
		  const char *color,
		  const char *color_word) const
{
	assert(message != "");
	assert(! isupper(message[0])); 

	switch (type) {
	default:  
	case Type::EMPTY:
		/* It's a common bug in Stu to have empty places, so
		 * better provide sensible behavior in NDEBUG builds.  */ 
		assert(false); 
		fprintf(stderr, 
			"%s\n",
			message.c_str()); 
		break; 

	case Type::INPUT_FILE:
		fprintf(stderr,
			"%s%s%s:%s%u%s:%s%u%s: %s\n", 
			color_word, get_filename_str(), Color::end,
			color, line, Color::end,
			color, 1 + column, Color::end,
			message.c_str());  
		break;

	case Type::ARGUMENT:
		fprintf(stderr,
			"%s%s%s: %s\n",
			color,
			"Command line argument",
			Color::end,
			message.c_str());
		break;

	case Type::OPTION:
		assert(text.size() == 1); 
		fprintf(stderr,
			"%sOption %s-%c%s: %s\n",
			color,
			color_word,
			text[0],
			Color::end,
			message.c_str());
		break;

	case Type::ENV_OPTIONS:
		fprintf(stderr,
			"In %s$STU_OPTIONS%s: %s\n",
			color_word, Color::end,
			message.c_str()); 
		break;
	}
}

string Place::as_argv0() const
{
	switch (type) {
	default:  
	case Type::EMPTY:
		assert(false); 
	case Type::ARGUMENT:
		return ""; 

	case Type::INPUT_FILE: {
		/* The given argv[0] should not begin with a dash,
		 * because some shells enable special behaviour
		 * (restricted/login mode and similar) when argv[0]
		 * begins with a dash. */ 
		const char *s= get_filename_str();
		return frmt("%s%s:%u", 
			    s[0] == '-' ? "file " : "",
			    s,
			    line);  
	}
	}
}

const char *Place::get_filename_str() const
{
	assert(type == Type::INPUT_FILE);
	return text == ""
		? "<stdin>"
		: text.c_str();
}

void print_warning(const Place &place, string message)
{
	assert(message != "");
	assert(isupper(message[0]) || message[0] == '\''); 
	assert(message[message.size() - 1] != '\n'); 
	place.print(fmt("warning: %s", message),
		    Color::warning, Color::warning_word); 
}

/* Explanation functions:  they output an explanation of a feature of
 * Stu on standard output.  This is used after certain non-trivial error
 * messages, and is enabled by the -E option. */

void explain_clash() 
{
	if (! option_explain)  return;
	fputs("Explanation: A dependency cannot be declared as persistent\n"
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
	      "invocation, one of the target-specifying options -c/-C/-p/-o/-n/-0,\n"
	      "an -f option with a default target, a file 'main.stu' with a default\n"
	      "target, or an -F option.\n",
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
	fputs("Explanation: A cycle in the dependency graph is an error.  Cycles are \n"
	      "verified on the rule level, not on the target level.\n", 
	      stderr);
}

void explain_startup_time()
{
	if (! option_explain)  return;
	fputs("Explanation: If a created file has a timestamp older than the startup of Stu,\n"
	      "a clock skew is likely.\n",
	      stderr); 
}

void explain_variable_equal()
{
	if (! option_explain)  return;
	fputs("Explanation: The name of an environment variable cannot contain the\n"
	      "equal sign '=', because the operating system uses '=' as a delimiter\n"
	      "when passing environment variables to child processes.\n"
	      "The syntax $[VARIABLENAME = FILENAME] can be used to use a different\n"
	      "name for the variable.\n",
	      stderr); 
}

void explain_version()
{
	if (! option_explain)  return;
	fputs("Explanation: Each Stu file can declare a version to which it is compatible\n"
	      "using the syntax '% version X.Y' or '% version X.Y.Z'.  Stu will then fail at\n"
	      "runtime if (a) 'X' does not equal the major version number of Stu,\n"
	      "(b) 'Y' is larger than Stu's minor version number, or (c) 'X' equals Stu's\n"
	      "minor version number and 'Z' is larger than Stu's patch level.  These rules\n"
	      "correspond to the SemVer.org version semantics.\n",
	      stderr); 
}

#endif /* ! ERROR_HH */ 
