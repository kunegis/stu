#ifndef ERROR_HH
#define ERROR_HH

/*
 * Code for managing errors in Stu.  Errors codes in Stu are represented
 * by integers between 0 and 4, as defined in the ERROR_* constants
 * below.  Zero represents no error.  These codes are used for both
 * Stu's exit status and as values that are stored in ints, as well as
 * thrown and caught.  Variables containing error codes are ints and are
 * named "error".
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
 * possible, but sometimes they must be used. 
 */

/*
 * Wording of messages:  
 *
 * Error messages begin with uppercase letters; trace messages with
 * lowercase letters, as per the GNU Coding Standards.  Filenames and
 * operator names are quoted in messages using single quotes.  Messages
 * for neither type of error output lines are terminated by periods.
 *
 * Typical forms of error messages are:
 *
 *    Location 2: expected an AAA, not BBB
 *    Location 1: in CCC
 *
 *    Location 2: AAA must not be used
 *    Location 1: in BBB
 *
 *    Location 2: AAA must not be used
 *    Location 1: in FEATURE_NAME using OPERATOR
 *
 * Use "expected TOKEN" instead of "missing TOKEN".  That is because
 * some tokens in the given list may be optional, making the "missing"
 * phrasing confusing, as it would imply that the token is mandatory.
 * Include definite or indefinite articles after "expected" to avoid
 * interpreting "expected" as an adjective.
 *
 * "not BBB" mentions the invalid token.  If end-of-file is encountered,
 * the "not BBB" part is not used. 
 * 
 * Use "must not" rather than "cannot" or "shall" in error messages when
 * something must be present, but is erroneous, e.g., "filename must not
 * be empty".  On the other hand, use "cannot" when something completely
 * unexpected was encountered, e.g., "transient targets cannot be used
 * with copy rule".
 *
 * Operators and other syntax elements are often introduced by the word
 * "using" rather than "with", etc., e.g., "expected a filename after
 * input redirection using '<'".  We always mention both the operator as
 * well as its function.
 *
 * When referring to dependencies and targets, we don't use the words
 * "dependency" or "target".  For instance, just write "'X' is needed by
 * 'A'" instead of "dependency 'X' is needed by 'A'".  The exception is
 * when the word "dependency" is qualified, e.g. "dynamic dependency [X]
 * must not have input redirection using '<'", or when referring
 * specifically to a dependency with respect to a target. 
 * 
 * But remember that in general it is better to state what what expected
 * in the syntax than to say that what was encountered cannot be used.
 * For instance, say "expected a filename" instead of "filename must not
 * be empty".  This cannot always be done, so "must not" is sometimes
 * used.  
 *
 * Even though error messages should contain all the information
 * mentioned above, they should still be terse.  More information is
 * included in the explanations using the -E options, output by the
 * explain_*() functions.
 */

#include <assert.h>

#include "options.hh"
#include "text.hh"
#include "color.hh"
#include "format.hh"

/* The error constants.  Not declared as an enum because they are thrown
 * and thus need to be integers.  */
const int ERROR_BUILD=     1;
const int ERROR_LOGICAL=   2;
const int ERROR_FATAL=     4;

/*
 * Build errors (code 1) are errors encountered during the normal
 * operation of Stu.  They indicate failures of the executed commands or
 * errors with files.  Exit status 1 is also used for the -q option
 * (question mode), when the targets are not up to date.
 *
 * Logical errors (code 2) are errors with the usage of Stu.  These are
 * for instance syntax errors in Stu scripts, cycles in the dependency
 * graph, or multiple matching rules.  
 * 
 * Fatal errors (code 4) are errors that lead Stu to abort immediately,
 * even when the -k option is used.  They are avoided as much as
 * possible.
 *
 * Errors 1 and 2 are recoverable.  If the -k option ("keep going") is
 * given, Stu notes these errors and continues.  If the -k option is not
 * given, they cause Stu to abort.  When the -k option is used, the
 * final exit status may combine errors 1 and 2, giving exit status 3.
 * Error 4 is unrecoverable, and leads to Stu aborting immediately.
 * Error 4 is never combined with other errors. 
 *
 * Fatal errors are rare and are only used in cases where it is OK to
 * abort the whole Stu process:
 *    - When there are errors in the usage of Stu on the command line,
 *      e.g. non-existing options.  In these cases, we don't lose
 *      anything because nothing was yet started.
 *    - Errors that happen just before Stu exits anyway.
 *    - Errors that should not happen, such as failure to set up a
 *      signal handler. 
 *
 * We have to be careful with calling exit(ERROR_FATAL):  We cannot do
 * it when child processes could still be running.  For errors that
 * happen in the middle of a Stu run, it makes more sense to generate a
 * build error.  In the very few cases that absolutely need to be fatal
 * (such as malloc() returning null), we call abort() instead, since
 * that will take care of terminating the child processes. 
 */

/*
 * The following functions print messages that do not include a place.
 * The text must begin with an uppercase letter, not end in a period
 * and also not end in a newline character.  All are printed to standard
 * error output. 
 */ 

void print_error(string message)
/* Print an error without a place */
{
	assert(message != "");
	assert(isupper(message[0]) || message[0] == '\''); 
	assert(message[message.size() - 1] != '\n'); 
	fprintf(stderr, "%s%s%s: *** %s\n", 
		Color::error_word, dollar_zero, Color::end,
		message.c_str()); 
}

void print_error_system(string message)
/* Like perror(), but use color.  MESSAGE must not contain color codes. */ 
{
	assert(message.size() > 0 && message[0] != '') ;
	string t= name_format_err(message); 
	fprintf(stderr, "%s: %s\n",
		t.c_str(),
		strerror(errno));
}

void print_error_reminder(string message)
/* Print a reminder of an error on STDERR.  This is used in situations
 * where an error has already been output, but it is better to remind
 * the user of the error.  Since the error as already been output, use
 * the color of warnings.  */
{
	assert(message != "");
	assert(isupper(message[0]) || message[0] == '\''); 
	assert(message[message.size() - 1] != '\n'); 
	fprintf(stderr, "%s%s%s: %s\n", 
		Color::warning, dollar_zero, Color::end,
		message.c_str()); 
}

string system_format(string text)
/* System error message.  Includes the given message, and the
 * ERRNO-based text.  Cf. perror().  Color is not added.  The output of
 * this function is used as input to one of the print_*() functions.  */
{
	return fmt("%s: %s",
		   text,
		   strerror(errno)); 
}

void print_out(string text)
/* Print a message to standard output in "print" colors.  This is used
 * in only very few cases, in defiance of the principle that a program
 * should by default only output something when there is an error.  We
 * do it mostly because Make does it, and users expect it, and because
 * not doing it would be very strange for most users.  These messages
 * are suppressed by the -s option (silent).  Messages that do not have
 * colored output are printed directly using printf() etc.  */
{
	assert(text != "");
	assert(isupper(text[0]));
	assert(text[text.size() - 1] != '\n');

	if (option_silent)
		return; 

	printf("%s%s%s\n",
	       Color::out_print,
	       text.c_str(),
	       Color::out_end); 
}

void print_error_silenceable(string text)
/* A message on STDERR that is made silent by the silent option (-s) */ 
{
	assert(text != "");
	assert(isupper(text[0]));
	assert(text[text.size() - 1] != '\n');

	if (option_silent)
		return;

	fprintf(stderr,
		"%s%s%s\n",
		Color::error,
		text.c_str(),
		Color::end);
}

class Place
/* 
 * Denotes a position in Stu source code.  This is either in a file or
 * in arguments/options to Stu.  A Place object can also be empty, which
 * is used as the "uninitialized" value.
 *
 * Places are used to show the location of an error on standard error
 * output.
 */ 
{
public:

	enum class Type {
		EMPTY,        /* Empty "Place" object */
		INPUT_FILE,   /* In a file, with line/column numbers */
		ARGUMENT,     /* Command line argument (outside options) */ 
		OPTION,       /* In an option */
		ENV_OPTIONS   /* In $STU_OPTIONS */
	} type;

	string text;
	/* INPUT_FILE:  Name of the file in which the error occurred.
	 *              Empty string for standard input.  
	 * OPTION:  Name of the option (a single character)
	 * Others:  Unused  */ 

	size_t line; 
	/* INPUT_FILE:  Line number, one-based.  
	 * Others:  unused.  
	 * The line number is one-based, but is allowed to be set to
	 * zero temporarily.  It should be >0 however when operator<<()
	 * is called.  */ 

	size_t column; 
	/* INPUT_FILE:  Column number, zero-based.  In output, column
	 * numbers are one-based, but they are saved here as zero-based
	 * numbers as these are easier to generate. 
	 * Others: Unused.  */ 

	Place() 
	/* Empty */ 
		:  type(Type::EMPTY) 
	{  }

	Place(Type type_,
	      string filename_, 
	      size_t line_, 
	      size_t column_)
	/* Generic constructor */ 
		:  type(type_),
		   text(filename_),
		   line(line_),
		   column(column_)
	{  }

	Place(Type type_)
	/* In command line argument (ARGV) */ 
		:  type(type_)
	{
		assert(type == Type::ARGUMENT); 
	}

	Place(Type type_, char option)
	/* In an option (OPTION) */
		:  type(type_),
		   text(string(&option, 1))
	{ 
		assert(type == Type::OPTION); 
	}
	

	Type get_type() const { return type; }
	const char *get_filename_str() const;

	const Place &operator<<(string message) const; 
	/* Print the trace to STDERR as part of an error message.  The 
	 * trace is printed as a single line, which can be parsed by
	 * tools, e.g. the compile mode of Emacs.  Line and column
	 * numbers are output as 1-based values.  Return THIS.  */

	void print(string message,
		   const char *color,
		   const char *color_word) const;
	/* Print a message.  The COLOR arguments determine whether this
	 * is an error or a warning.  */ 

	string as_argv0() const;
	/* The string used for the argv[0] parameter of child processes.
	 * Does not include color codes.  Returns "" when no special
	 * string should be used.  */

	bool empty() const { 
		return type == Type::EMPTY;
	}
	
	void clear() {
		type= Type::EMPTY; 
	}

	static const Place place_empty;
	/* A static empty place object, used in various places when a
	 * reference to an empty place object is needed.  Otherwise,
	 * Place() is an empty place.  */
};

class Trace
/* 
 * A place along with a message.  This class is only used when traces
 * cannot be printed immediately.  Otherwise, Place::operator<<() is
 * called directly.  
 */
{
public:

	Place place;

	string message; 
	/* The message associated with it.  This may be "". 
	 * When the trace is printed, it must not be empty, and not begin
	 * with an upper-case letter.  */

	Trace(const Place &place_, string message_) 
		:  place(place_), message(message_) 
	{  }

	void print() const 
	/* Print the trace to STDERR as part of an error message; see
	 * Place::operator<< for format information.  */
	{
		place << message; 
	}
};

class Printer
/* Interface that has the << operator like Place.  Only used at one
 * place for now.  The parameter MESSAGE may be "" to print a generic
 * error message (i.e., only the traces).  */
{
public:
	virtual void operator<<(string message) const= 0; 
	virtual ~Printer() = default;
};

const Place Place::place_empty;

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
		assert(line >= 1); 
		fprintf(stderr,
			"%s%s%s:%s%zu%s:%s%zu%s: %s\n", 
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

	case Type::OPTION:
		return fmt("Option -%s", text); 

	case Type::INPUT_FILE: {
		/* The given argv[0] should not begin with a dash,
		 * because some shells enable special behaviour
		 * (restricted/login mode and similar) when argv[0]
		 * begins with a dash. */ 
		const char *s= get_filename_str();
		return frmt("%s%s:%zu", 
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

#endif /* ! ERROR_HH */ 
