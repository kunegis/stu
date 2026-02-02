#ifndef PLACE_HH
#define PLACE_HH

/*
 * Denotes a position in Stu source code.  This is either in a file or in
 * arguments/options to Stu.  A Place object can also be empty, which is used as the
 * "uninitialized" value.
 */
class Place
{
public:
	enum class Type {
		EMPTY,
		INPUT_FILE,   /* In a file, with line/column numbers */
		ARGUMENT,     /* Command line argument (outside options) */
		OPTION,       /* In an option */
		ENV_OPTIONS   /* In $STU_OPTIONS */
	} type;

	string text;
	/* INPUT_FILE:  Name of the file in which the error occurred.  Empty string for
	 *              standard input.
	 * OPTION:  Name of the option (a single character)
	 * Others:  Unused */

	size_t line;
	/* INPUT_FILE:  Line number, one-based.
	 * Others:  unused.
	 * The line number is one-based, but is allowed to be set to zero temporarily.  It
	 * should be >0 however when operator<<() is called. */

	size_t column;
	/* INPUT_FILE:  Column number, zero-based.  In output, column numbers are
	 * one-based, but they are saved here as zero-based numbers as these are easier to
	 * generate.  Others: Unused. */

	Place(): type(Type::EMPTY) { }

	Place(Type type_, std::string_view filename_,
	      size_t line_, size_t column_)
	/* Generic constructor */
		: type(type_), text(filename_),
		  line(line_), column(column_)
	{ }

	Place(Type type_): type(type_) {
		assert(type == Type::ARGUMENT || type == Type::ENV_OPTIONS);
	}

	Place(Type type_, char option)
	/* In an option (OPTION) */
		: type(type_), text(string(&option, 1))
	{
		assert(type == Type::OPTION);
	}

	Type get_type() const { return type; }
	const char *get_filename_str() const;

	const Place &operator<<(string message) const;
	/* Print the backtrace to STDERR as part of an error message.  The backtrace is
	 * printed as a single line, which can be parsed by tools, e.g. the compile mode
	 * of Emacs.  Line and column numbers are output as 1-based values.  Return
	 * THIS. */

	void print(string message,
		   const char *color_on, const char *color_off) const;

	string as_argv0() const;
	/* The string used for the argv[0] parameter of child processes.  Does not include
	 * color codes.  Returns "" when no special string should be used. */

	bool empty() const  {  return type == Type::EMPTY;  }
	void clear()  {  type= Type::EMPTY;  }

	static const Place place_empty;
	/* A static empty place object, used in various places when a reference to an
	 * empty place object is needed.  Otherwise, Place() is an empty place. */
};

void print_warning(const Place &place, string message);

#endif /* ! PLACE_HH */
