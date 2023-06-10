#ifndef TOKENIZER_HH
#define TOKENIZER_HH

/*
 * On errors, these functions print a message and throw integer error codes.
 */

constexpr const char *FILENAME_INPUT_DEFAULT= "main.stu";

class Tokenizer
{
public:
	enum Context { SOURCE, DYNAMIC, OPTION_C, OPTION_F };

	static void parse_tokens_file(vector <shared_ptr <Token> > &tokens,
				      Context context,
				      Place &place_end,
				      string filename,
				      const Place &place_diagnostic,
				      int fd= -1,
				      bool allow_enoent= false)
	/*
	 * Parse the tokens from the file FILENAME.
	 *
	 * The given file descriptor FD may optionally be that file
	 * already opened.  If the file was not yet opened, FD is -1.
	 * If FILENAME is "", use standard input, but FD must be -1.
	 *
	 * Append the read tokens to TOKENS, and set PLACE_END to the
	 * end of the parsed file.
	 *
	 * PLACE_DIAGNOSTIC is the place where this file is included
	 * from, e.g. the -f option.
	 *
	 * If ALLOW_ENOENT, an ENOENT error on this first open() is not
	 * reported as an error, and the function just returns.
	 */
	{
		vector <Backtrace> backtraces;
		vector <string> filenames;
		set <string> includes;
		parse_tokens_file(tokens,
				  context,
				  place_end, filename,
				  backtraces, filenames, includes,
				  place_diagnostic,
				  fd,
				  allow_enoent);
	}

	static bool is_flag_char(char);
	/* Whether the character is a valid flag */

	static void parse_tokens_string(vector <shared_ptr <Token> > &tokens,
					Context context,
					Place &place_end,
					string string_,
					const Place &place_string);
	/* Parse tokens from the given TEXT.  Other arguments are
	 * identical to parse_tokens_file().  */

private:
	/* Stacks of included files */
	vector <Backtrace> &backtraces;
	vector <string> &filenames;

	set <string> &includes;

	Place place_base;
	/* The place where the tokenizer is tokenizing.  The line and
	 * column numbers are modified accordingly -- only the type and
	 * text is used.  */

	size_t line; /* Line number */
	const char *p_line; /* Beginning of current line */
	const char *p; /* Current position */
	const char *const p_end; /* End of input */

	Environment environment= E_WHITESPACE;
	/* For the next token */

	Tokenizer(vector <Backtrace> &backtraces_,
		  vector <string> &filenames_,
		  set <string> &includes_,
		  const Place &place_base_,
		  const char *p_,
		  size_t length)
		:  backtraces(backtraces_),
		   filenames(filenames_),
		   includes(includes_),
		   place_base(place_base_),
		   line(1),
		   p_line(p_),
		   p(p_),
		   p_end(p_ + length)
	{  }

	void parse_tokens(vector <shared_ptr <Token> > &tokens,
			  Context context,
			  const Place &place_diagnostic);

	shared_ptr <Command> parse_command();

	shared_ptr <Place_Name> parse_name(bool allow_special);
	/* Returns null when no name could be parsed.  Prints and throws
	 * on other errors, including on empty names.
	 * ALLOW_SPECIAL:  whether the name is allowed to start with one of '-+~'.
	 * E_SLASH set in ENVIRONMENT as appropriate.  */

	bool parse_parameter(string &parameter, Place &place_dollar);
	/* Parse a parameter starting with '$'.  Return whether a parameter was
	 * parsed (always TRUE).  The current position must be on the '$'
	 * character, not after it.  If a parameter is found, write it into the
	 * parameters.  */

	/* The following three functions parse the two types of quotes, and
	 * escapes.  The pointer must be on a ", ', or \ character respectively.
	 * The read string is appended to RET, or a logical error is thrown.
	 * parse_escape() consumes only the backslash, not what follows; it
	 * returns true if what follows must be escaped.  */
	void parse_double_quote(Place_Name &ret);
	void parse_single_quote(Place_Name &ret);
	bool parse_escape();

	void parse_directive(vector <shared_ptr <Token> > &tokens,
			     Context context,
			     const Place &place_diagnostic);
	/* Parse a directive.  The pointer must be on the '%' character.  Throw
	 * a logical error when encountered.  */

	bool skip_space(bool &skipped_actual_space);
	/* Skip any whitespace (including backslash-newline combinations).
	 * The return value tells whether anything was skipped.  The return
	 * parameter tells whether anything other than backslash-newline was
	 * skipped.  */

	Place current_place() const {
		return Place(place_base.type, place_base.text, line, p - p_line);
	}

	static void parse_tokens_file(vector <shared_ptr <Token> > &tokens,
				      Context context,
				      Place &place_end,
				      string filename,
				      vector <Backtrace> &backtraces,
				      vector <string> &filenames,
				      set <string> &includes,
				      const Place &place_diagnostic,
				      int fd= -1,
				      bool allow_enoent= false);
	/* TRACES can include traces that lead to this inclusion.  TRACES must
	 * not be modified when returning, but is declared as non-const
	 * because it is used as a stack.
	 *
	 * FILENAMES is the list of filenames parsed up to here. I.e., it has
	 * length zero for the main read file.  FILENAME should *not* be
	 * included in FILENAMES.  */

	static bool is_name_char(char);
	/* Whether the given character can be used as part of a bare filename in
	 * Stu.  Note that all non-ASCII characters are allowed, and thus we
	 * don't have to distinguish UTF-8 from 8-bit encodings: all characters
	 * with the most significant bit set will make this return TRUE.  See
	 * the file CHARACTERS for more information.  This returns TRUE for the
	 * mid-name characters '-', '+' and '~'.  */

	static bool is_operator_char(char);
	/* Whether the character can be an operator */

	static void parse_version(string version_req,
				  const Place &place_version,
				  const Place &place_percent);
	/* Parse a version directive.  VERSION_REQ is the version number
	 * given after "%version", and PLACE its place.  */
};

#endif /* ! TOKENIZER_HH */
