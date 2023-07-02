#ifndef PARSER_HH
#define PARSER_HH

/*
 * This is a recursive descent parser written by hand.
 */

/*
 * Stu has only prefix and circumfix operators, and therefore its syntax
 * is trivial, i.e., there are no ambiguities, and no need to consider
 * precendence levels or associativity.
 *
 * A Yacc-like description of Stu syntax is given in the manpage.
 *
 * In principle, the following gives operator precedences.  Higher in
 * the list means higher precedence.  This list is to be understood as
 * specifying in what order operators can be nested, not to disambiguate
 * expressions.
 *
 * @...      (prefix) Transient dependency; argument can only contain name
 * ---------------
 * <...      (prefix) Input redirection; argument must not contain '()',
 *           '[]', '$[]' or '@'
 * ---------------
 * !...      (prefix) Persistent dependency; argument must not contain '$[]'
 * ?...      (prefix) Optional dependency; argument must not contain '$[]'
 * &...      (prefix) Trivial dependency
 * ---------------
 * [...]     (circumfix) Dynamic dependency; must not contain '$[]' or '@'
 * (...)     (circumfix) Capture
 * $[...]    (circumfix) Variable inclusion; argument must not contain
 *           -o, '[]', '()', '*' or '@'
 */

/*
 * This code does not check that imcompatible constructs (like -p and -o
 * or -p and '$[') are used together.  Instead, this is checked within
 * Executor and not here, because these can also be combined from
 * different sources, e.g., a file and a dynamic dependency.
 */

#include "rule.hh"

class Parser
/*
 * An object of this type represents a location within a token list.
 */
{
public:
	/*
	 * Methods for building the syntax tree:  Each has a name corresponding
	 * to the symbol given by the Yacc syntax in the manpage.  The argument
	 * RET (if it is used) is where the result is written.  If the return
	 * value is BOOL, it denotes whether something was read or not.  On
	 * syntax errors, ERROR_LOGICAL is thrown.
	 */

	/* In some of the following functions, write the input filename into
	 * PLACE_NAME_INPUT.  If PLACE_NAME_INPUT is already non-empty, throw an
	 * error if a second input filename is specified. PLACE_INPUT is the
	 * place of the '<' input redirection operator.  */

	static void get_rule_list(vector <shared_ptr <Rule> > &rules,
				  vector <shared_ptr <Token> > &tokens,
				  const Place &place_end,
				  shared_ptr <const Place_Param_Target> &target_first);

	static void get_expression_list(vector <shared_ptr <const Dep> > &deps,
					vector <shared_ptr <Token> > &tokens,
					const Place &place_end,
					Place_Name &input,
					Place &place_input);
	/* DEPS is filled.  DEPS is empty when called. */

	static void get_expression_list_delim(vector <shared_ptr <const Dep> > &deps,
					      const char *filename,
					      char c, char c_printed,
					      const Printer &printer);
	/* Read delimiter-separated dynamic dependency from FILENAME,
	 * delimited by C.  Write result into DEPS.  Throws errors.  */

	static void get_target_arg(vector <shared_ptr <const Dep> > &deps,
				   int argc, const char *const *argv);
	/* Parse a dependency as given on the command line outside of
	 * options.  Strings in ARGV may be empty; those are ignored.
	 * Parsed dependency are appended to DEPS, which is not
	 * necessarily empty on invocation.  */

	static void get_file(string filename,
			     int file_fd,
			     Rule_Set &rule_set,
			     shared_ptr <const Place_Param_Target> &target_first,
			     Place &place_first);
	/* Read in an input file and add the rules to the given rule set.  Used
	 * for the -f option and the default input file.  If not yet non-null,
	 * set TARGET_FIRST to the first target of the first rule.  FILE_FD can be -1 or the FD or
	 * the filename, if already opened.  If FILENAME is "-", use standard input.  If FILENAME is
	 * "", use the default file ('main.stu').  */

	static void get_string(const char *s,
			       Rule_Set &rule_set,
			       shared_ptr <const Place_Param_Target> &target_first);
	/* Read rules from a string; same argument semantics as the other get_*() functions.  */

	static void add_deps_option_C(vector <shared_ptr <const Dep> > &deps,
				      const char *string_);
	/* Parse a string of dependencies and add them to the vector. Used for
	 * the -C option.  Support the full Stu syntax.  */

private:
	vector <shared_ptr <Token> > &tokens;
	vector <shared_ptr <Token> > ::iterator &iter;
	const Place place_end;

	Parser(vector <shared_ptr <Token> > &tokens_,
	       vector <shared_ptr <Token> > ::iterator &iter_,
	       const Place &place_end_)
		:  tokens(tokens_),
		   iter(iter_),
		   place_end(place_end_)
	{ }

	void parse_rule_list(vector <shared_ptr <Rule> > &ret,
			     shared_ptr <const Place_Param_Target> &target_first);
	/* The returned rules may not be unique -- this is checked later */

	bool parse_expression_list(vector <shared_ptr <const Dep> > &ret,
				   Place_Name &place_name_input,
				   Place &place_input,
				   const vector <shared_ptr <const Place_Param_Target> > &targets);
	/* RET is filled.  RET is empty when called. */

	shared_ptr <Rule> parse_rule(shared_ptr <const Place_Param_Target> &target_first);
	/* Return null when nothing was parsed */

	bool parse_expression(shared_ptr <const Dep> &ret,
			      Place_Name &place_name_input, Place &place_input,
			      const vector <shared_ptr <const Place_Param_Target> > &targets);
	/* Parse an expression.  Write the parsed expression into RET.
	 * RET must be empty when called.  Return whether an expression
	 * was parsed.  TARGETS is passed to construct error
	 * messages.  */

	shared_ptr <const Dep> parse_variable_dep
	(Place_Name &place_name_input, Place &place_input,
	 const vector <shared_ptr <const Place_Param_Target> > &targets);
	/* A variable dependency */

	shared_ptr <const Dep> parse_redirect_dep
	(Place_Name &place_name_input, Place &place_input,
	 const vector <shared_ptr <const Place_Param_Target> > &targets);

	/* If the next token is of type T, return it, otherwise return
	 * null.  Also return null when at the end of the token list.  */
	template <typename T>
	shared_ptr <T> is() const {
		if (iter == tokens.end())
			return nullptr;
		else
			return dynamic_pointer_cast <T> (*iter);
	}

	/* Whether the next token is the given operator */
	bool is_operator(char op) const {
		return is <Operator> () && is <Operator> ()->op == op;
	}

	/* Whether the next token is the given flag token */
	bool is_flag(char flag) const {
		return is <Flag_Token> () && is <Flag_Token> ()->flag == flag;
	}

	bool next_concatenates() const;
	/* Whether there is a next token which concatenates to the
	 * current token.  The current token is assumed to be a
	 * candidate for concatenation.  */

	static void print_separation_message(shared_ptr <const Token> token);

	static void append_copy(      Name &to,
				const Name &from);
	/* If TO ends in '/', append to it the part of FROM that
	 * comes after the last slash, or the full target if it contains
	 * no slashes.  Parameters are not considered for containing
	 * slashes */
};

void Parser::parse_rule_list(vector <shared_ptr <Rule> > &ret,
			     shared_ptr <const Place_Param_Target> &target_first)
{
	assert(ret.size() == 0);
	while (iter != tokens.end()) {

#ifndef NDEBUG
		const auto iter_begin= iter;
#endif /* ! NDEBUG */

		shared_ptr <Rule> rule= parse_rule(target_first);

		if (rule == nullptr) {
			assert(iter == iter_begin);
			break;
		}
		ret.push_back(rule);
	}
}

#endif /* ! PARSER_HH */
