#include "tokenizer.hh"

#include <sys/mman.h>

void Tokenizer::parse_tokens_file(vector <shared_ptr <Token> > &tokens,
				  Context context,
				  Place &place_end,
				  string filename,
				  vector <Backtrace> &backtraces,
				  vector <string> &filenames,
				  set <string> &includes,
				  const Place &place_diagnostic,
				  int fd,
				  bool allow_enoent)
{
	const char *in= nullptr;
	size_t in_size;
	struct stat buf;
	FILE *file= nullptr;

	bool use_malloc;
	/* False:  use mmap()
	 * True:   use malloc()  */

	try {
		if (context == SOURCE) {
			assert(filenames.size() == 0 ||
			       filenames[filenames.size() - 1] != filename);
			assert(includes.count(filename) == 0);
			includes.insert(filename);
		} else {
			assert(filenames.size() == 0);
			assert(backtraces.size() == 0);
		}

		/* Map empty string to stdin */
		if (filename.empty()) {
			assert(fd == -1);
			fd= 0;
			file= stdin;
		}

		if (fd < 0) {
			fd= open(filename.c_str(), O_RDONLY);
			if (fd < 0) {
				if (allow_enoent) {
					if (errno == ENOENT) {
						return;
					}
				}
				goto error;
			}
		}

		if (fstat(fd, &buf) < 0) {
			goto error_close;
		}

		/* If the file is a directory, open the file "main.stu" within it */
		if (S_ISDIR(buf.st_mode)) {
			if (filename[filename.size() - 1] != '/')
				filename += '/';
			filename += FILENAME_INPUT_DEFAULT;
			int fd2= openat(fd, FILENAME_INPUT_DEFAULT, O_RDONLY);
			if (fd2 < 0)
				goto error_close;
			if (close(fd) < 0) {
				close(fd2);
				goto error;
			}
			fd= fd2;
			if (fstat(fd, &buf) < 0)
				goto error_close;
		}

		/* Handle a file of zero length separately because mmap() may fail
		 * on it, i.e., return an error and refuse to create a memory
		 * map of length zero. */
		if (S_ISREG(buf.st_mode) && buf.st_size == 0) {
			place_end= Place(Place::Type::INPUT_FILE, filename, 1, 0);
			goto return_close;
		}

		if (! S_ISREG(buf.st_mode)) {
			goto try_read;
		}

		use_malloc= false;

		in_size= buf.st_size;
		in= (char *) mmap(nullptr, in_size,
				  PROT_READ, MAP_SHARED, fd, 0);
		if (in == MAP_FAILED) {

		try_read:
			use_malloc= true;

			if (file == nullptr) {
				file= fdopen(fd, "r");
				if (file == nullptr)
					goto error_close;
			}
			const size_t BUFLEN= 0x1000;
			char *mem= nullptr;
			size_t len= 0;

			while (true) {
				if (len + BUFLEN < len) {
					free(mem);
					errno= ENOMEM;
					goto error_close;
				}
				char *mem_new= (char *) realloc(mem, len + BUFLEN);
				if (mem_new == nullptr) {
					free(mem);
					goto error_close;
				}
				mem= mem_new;
				size_t r= fread(mem + len, 1, BUFLEN, file);
				assert(r <= BUFLEN);
				if (r == 0) {
					if (ferror(file)) {
						if (! filename.empty()) {
							int errno_save= errno;
							fclose(file);
							errno= errno_save;
						}
						goto error_close;
					} else {
						break;
					}
				}
				len += r;
			}

			/* Close the input file, but not if it is STDIN */
			if (! filename.empty()) {
				if (fclose(file) != 0) {
					goto error_close;
				}
			} else {
				if (ferror(file))
					goto error_close;
			}

			in= mem;
			in_size= len;
		}

		if (! filename.empty()) {
			assert(fd >= 3);
			if (0 > close(fd))
				goto error;
		}

		{
			Tokenizer tokenizer(backtraces, filenames, includes,
					    Place(Place::Type::INPUT_FILE, filename, 1, 0),
					    in, in_size);
			tokenizer.parse_tokens(tokens, context, place_diagnostic);
			place_end= tokenizer.current_place();

			if (use_malloc) {
				free((void *) in);
			} else { /* mmap() */
				if (0 > munmap((void *) in, in_size))
					goto error;
			}

			return;
		}

	return_close:
		if (! filename.empty()) {
			assert(fd >= 3);
			if (0 > close(fd))
				goto error;
		} else {
			assert(fd == 0);
		}
		return;

	error_close:
		if (! filename.empty()) {
			assert(fd >= 3);
			close(fd);
		} else {
			assert(fd == 0);
		}
	error:
		const char *filename_diagnostic= !filename.empty()
			? filename.c_str() : "<stdin>";
		if (backtraces.size() > 0) {
			for (auto j= backtraces.begin(); j != backtraces.end(); ++j) {
				if (j == backtraces.begin()) {
					j->place << format_errno
						(fmt("%s %s",
						     show_operator("%include"),
						     show(filename_diagnostic)));
				} else {
					j->print();
				}
			}
		} else {
			if (place_diagnostic.get_type() != Place::Type::EMPTY)
				place_diagnostic << format_errno
					(show(filename_diagnostic));
			else
				print_error(format_errno
					    (show(filename_diagnostic)));
		}
		throw ERROR_BUILD;

	} catch (int error) {

		if (in != nullptr) {
			if (use_malloc) {
				free((void *) in);
			} else { /* mmap() */
				munmap((void *) in, in_size);
			}
		}

		throw;
	}
}

shared_ptr <Command> Tokenizer::parse_command()
/*
 * To determine the place of the command:  These rules are intended to
 * make the editor go to the correct place to enter a command into the
 * editor when the command is empty.
 *    - If there is non-whitespace in the command, the place is the
 *      first non-whitespace character
 *    - If the command is completely empty (i.e., {}), then the place is
 *      the closing bracket.
 *    - If the command contains only whitespace, is on one line and is
 *      not empty, the place is on the first whitespace character.
 *    - If the command contains only whitespace and a single newline,
 *      then the place is the end of the first line.
 *    - If the command contains only whitespace and at least two
 *      newlines, the place is the first character of the second line.
 */
/*
 * The parsing of commands in Stu only approximates shell syntax.  In
 * actual shell syntax, the characters {} are only recognized as
 * block-opening and -closing in certain circumstances.  For instance:
 *
 *             echo }
 *             echo {
 *
 * In each of these lines, the character '{' or '}' is an argument to
 * 'echo' for the shell, but is interpreted as opening and closing a {}
 * block by Stu.  This is a known limitation in Stu, but fixing it would
 * be backward incompatible, because it would break Stu scripts such as
 *
 *             >A { echo Hello World }
 *
 * because Stu would then interpret the '}' as an argument to 'echo' --
 * this would be consistent with how the shell parses things, and the
 * solution is to write
 *
 *             >A { echo Hello World ; }
 *
 * This ';' is obligatory in the shell, but not in Stu.  We would like
 * to make the ';' mandatory in Stu too, but this would break existing
 * Stu scripts, and we don't do backward-incompatible changes without
 * increasing the major version number.  (And even then, it should be
 * well-motivated.)  Thus, this change will come, if at all, only in
 * Version 3 of Stu.
 *
 * Also, the case that Stu script actually try do something like
 *
 *             >A { echo } ; }
 *
 * is much much much rarer.  (As an aside, if a Stu command does complex
 * shell stuff with weird characters etc., it's probably better to put
 * it in a separate script.)
 *
 * Note also that this does *not* apply to (), which behave as expected
 * in both the shell and Stu.  The three quote-like characters '"`
 * cannot be used recursively, so the problem does not arise.  There may
 * also be other syntax corner-cases of the shell that Stu does not
 * parse identically as the shell.
 */
{
	assert(p < p_end && *p == '{');
	const Place place_open= current_place();
	++p;
	const char *const p_beg= p;

	size_t line_command= line; /* The line of the place of the command */
	size_t column_command= p - p_line; /* The column of the place of
					    * the command */
	const size_t line_first= line; /* Where the command started */
	bool begin= true; /* We have not yet seen non-whitespace */

	string stack= "{";
	/* Stack of opened parenthesis-like symbols to parse shell
	 * syntax.  May contain:  {'"`( */

	while (p < p_end) {
		const char last= stack[stack.size() - 1];

		if (begin) {
			if (*p == '\n') {
				if (line == line_first) {
					line_command= line;
					column_command= p - p_line;
				} else if (line == line_first + 1) {
					assert(line_command == line - 1);
					line_command= line;
					column_command= 0;
				}
			} else if (! isspace(*p)) {
				if (*p != '}') {
					begin= false;
					line_command= line;
					column_command= p - p_line;
				}
			}
		}

		switch (*p) {
		case '}':
			if (last == '{') {
				stack.resize(stack.size() - 1);
				if (stack.empty()) {
					const string command= string(p_beg, p - p_beg);
					++p;
					const Place place_command
						(place_base.type, place_base.text,
						 line_command, column_command);
					return make_shared <Command>
						(command, place_command, place_open, environment);
				} else {
					++p;
				}
			} else {
				++p;
			}
			break;

		case '{':
			if (last == '{' || last == '(') {
				stack += '{';
				++p;
			} else {
				++p;
			}
			break;

		case '\'':
			if (last == '\'') {
				stack.resize(stack.size() - 1);
				++p;
			} else if (last == '{' || last == '(') {
				stack += '\'';
				++p;
			} else /* last == '"' || last == '`' */
				++p;
			break;

		case '"':
			if (last == '"') {
				stack.resize(stack.size() - 1);
				++p;
			} else if (last == '\'' || last == '`') {
				++p;
			} else /* last == '{' || last == '(' */ {
				stack += '"';
				++p;
			}
			break;

		case '`':
			if (last == '`') {
				stack.resize(stack.size() - 1);
				++p;
			} else if (last == '\'') {
				++p;
			} else /* last == '{' || last == '"' || last == '(' */ {
				stack += *p;
				++p;
			}
			break;

		case '\\':
			if (last == '\'') {
				++p;
			} else {
				++p;
				if (p < p_end) {
					if (*p == '\n') {
						++line;
						p_line= p + 1;
					}
					++p;
				}
			}
			break;

		case '#':
			++p;
			if (last == '{' || last == '(' || last == '`') {
				while (p < p_end && *p != '\n')  ++p;
			}
			break;

		case '(':
			if (last == '{' || last == '(') {
				++p;
				stack += '(';
			} else
				++p;
			break;

		case ')':
			if (last == '(') {
				stack.resize(stack.size() - 1);
				++p;
			} else
				++p;
			break;

		case '$':
			++p;
			if (p < p_end && *p == '(') {
				if (last == '{' || last == '(' || last == '"') {
					++p;
					stack += '(';
				} else
					++p;
			}
			break;

		default:
			if (*p == '\n') {
				++line;
				p_line= p + 1;
			}
			++p;
		}
	}

	current_place() << fmt("expected a closing %s", show_operator('}'));
	place_open << fmt("for command started by %s", show_operator('{'));
	throw ERROR_LOGICAL;
}

shared_ptr <Place_Name> Tokenizer::parse_name(bool allow_special)
{
	const char *const p_begin= p;
	Place place_begin= current_place();

	shared_ptr <Place_Name> ret= make_shared <Place_Name> ("", place_begin);

	/* Don't allow '-', '+' and '~' at beginning of a name */
	if (p < p_end && ! allow_special) {
		if (*p == '-' || *p == '+' || *p == '~') {
			return nullptr;
		}
	}

	bool has_escape= false;
	while (p < p_end) {
		if (!has_escape && *p == '"') {
			parse_double_quote(*ret);
		} else if (!has_escape && *p == '\'') {
			parse_single_quote(*ret);
		} else if (!has_escape && *p == '$') {
			string parameter;
			Place place_dollar;
			if (parse_parameter(parameter, place_dollar)) {
				ret->append_parameter(parameter, place_dollar);
			} else {
				assert(false);
			}
		} else if (!has_escape && *p == '\\') {
			has_escape= parse_escape();
		} else if (has_escape || is_name_char(*p)) {
			/* An ordinary character */
			assert(p != p_begin
			       || (*p != '-' && *p != '+' && *p != '~')
			       || allow_special
			       || has_escape);
			has_escape= false;
			ret->last_text() += *p++;
		}
		else {
			/* As soon as the name cannot be parsed
			 * further, stop parsing */
			break;
		}
	}

	if (ret->empty()) {
		if (p == p_begin)
			return nullptr;
		place_begin << "name must not be empty";
		throw ERROR_LOGICAL;
	}

	return ret;
}

bool Tokenizer::parse_parameter(string &parameter, Place &place_dollar)
{
	assert(p < p_end && *p == '$');

	place_dollar= current_place();
	++p;
	bool braces= false;
	if (p < p_end && *p == '{') {
		++p;
		braces= true;
	}
	Place place_parameter_name= current_place();

	const char *const p_parameter_name= p;

	while (p < p_end && (isalnum(*p) || *p == '_')) {
		++p;
	}

	if (braces) {
		if (p == p_end) {
			current_place() <<
				fmt("expected a closing %s", show_operator('}'));
			place_dollar <<
				fmt("for parameter started by %s",
				    show_operator("${"));
			explain_parameter_character();
			throw ERROR_LOGICAL;
		} else if (*p != '}') {
			current_place() <<
				fmt("character %s must not appear", show(string(p, 1)));
			place_dollar <<
				fmt("in parameter started by %s",
				    show_operator("${"));
			explain_parameter_character();
			throw ERROR_LOGICAL;
		}
	}

	if (p == p_parameter_name) {
		if (p < p_end)
			place_parameter_name <<
				fmt("expected a parameter name, not %s",
				    show(string(p, 1)));
		else
			place_parameter_name <<
				"expected a parameter name";
		place_dollar << fmt("after %s", show_operator('$'));
		explain_parameter_syntax();
		throw ERROR_LOGICAL;
	}

	parameter= string(p_parameter_name, p - p_parameter_name);

	if (isdigit(parameter[0])) {
		place_parameter_name <<
			fmt("parameter name %s must not start with a digit",
			    show_prefix("$", parameter));
		throw ERROR_LOGICAL;
	}
	if (braces)
		++p;

	return true;
}

bool Tokenizer::is_name_char(char c)
/* The characters in the string constant are those characters that have
 * special meaning (as defined in the manpage), and those reserved for
 * future extension (also defined in the manpage)  */
{
	return (c > 0x20 && c < 0x7F /* ASCII printable character except space */
		&& nullptr == strchr("[]\"\':={}#<>@$;()%*\\!?|&,", c))
		|| ((unsigned char) c) >= 0x80;
}

bool Tokenizer::is_operator_char(char c)
{
	return c != '\0' && nullptr != strchr(":<>=@;()[],|", c);
}

bool Tokenizer::is_flag_char(char c)
/* These correspond to persistent, optional and trivial dependencies,
 * respectively.  'p'/'o'/'t' were '!', '?' and '&' formerly.  The others are
 * new.  */
{
	return c == 'p' || c == 'o' || c == 't' ||
		c == 'n' || c == '0';
}

void Tokenizer::parse_version(string version_req,
			      const Place &place_version,
			      const Place &place_percent)
/* Note: there may be any number of version directives in Stu (in
 * particular from multiple source files), so we don't keep track
 * whether one has already been provided.  */
{
	unsigned major_req, minor_req, patch_req;
	int chars= -1;
	int n= sscanf(version_req.c_str(), "%u.%u.%u%n",
		      &major_req, &minor_req, &patch_req, &chars);
	if (n != 3) {
		chars= -1;
		n= sscanf(version_req.c_str(), "%u.%u%n",
			  &major_req, &minor_req, &chars);
		if (n != 2)
			goto error;
	}
	assert(chars >= 0);
	if ((size_t) chars != version_req.size())
		goto error;

	assert(n == 2 || n == 3);

	if (n == 2) {
		if (STU_VERSION_MAJOR != major_req || STU_VERSION_MINOR < minor_req)
			goto wrong_version;
	} else {
		if (STU_VERSION_MAJOR != major_req || STU_VERSION_MINOR < minor_req ||
		    (STU_VERSION_MINOR == minor_req && STU_VERSION_PATCH < patch_req))
			goto wrong_version;
	}

	return;

 wrong_version: {
		place_version <<
			fmt("requested version %s using %s is incompatible with this Stu's version %s",
			    show(version_req),
			    show_operator("%version"),
			    show(STU_VERSION)
			    );
		explain_version();
		throw ERROR_LOGICAL;
	}

 error:
	place_version <<
		fmt("expected a version number of the form %s or %s, not %s",
		    show("MAJOR.MINOR"),
		    show("MAJOR.MINOR.PATCH"),
		    show(version_req));
	place_percent << fmt("after %s",
			     show_operator("%version"));

	throw ERROR_LOGICAL;
}

void Tokenizer::parse_tokens(vector <shared_ptr <Token> > &tokens,
			     Context context,
			     const Place &place_diagnostic)
{
	bool skipped_actual_space;
	/* Output parameter for skip_space(); will become better in C++17. */

	while (p < p_end) {
		/* Operators except '$' */
		if (is_operator_char(*p)) {
			Place place= current_place();
			tokens.push_back(make_shared <Operator> (*p, place, environment));
			++p;
		}

		/* Variable dependency */
		else if (*p == '$' && p + 1 < p_end && p[1] == '[') {
			Place place_dollar= current_place();
			Place place_langle(place_base.type, place_base.text,
					   line, p + 1 - p_line);
			tokens.push_back(make_shared <Operator> ('$', place_dollar, environment));
			tokens.push_back(make_shared <Operator> ('[', place_langle, environment));
			p += 2;
		}

		/* Command */
		else if (*p == '{') {
			shared_ptr <Command> token= parse_command();
			tokens.push_back(token);
		}

		/* Comment */
		else if (*p == '#') {
			/* Skip the comment without generating any token */
			do  ++p;  while (p < p_end && *p != '\n');
		}

		/* Whitespace */
		else if (skip_space(skipped_actual_space)) {
			if (skipped_actual_space) {
				environment |= E_WHITESPACE;
				goto had_whitespace;
			}
		}

		/* Directive */
		else if (*p == '%') {
			parse_directive(tokens, context, place_diagnostic);
		}

		/* Flag, name, or invalid character */
		else {
			/* Flag */
			bool allow_special=
				!(environment & E_WHITESPACE)
				&& !tokens.empty()
				&& to <Operator> (tokens.back())
				&& (to <Operator> (tokens.back())->op == ']' ||
				    to <Operator> (tokens.back())->op == ')');

			if (p < p_end &&
			    (*p == '-' || *p == '+' || *p == '~') &&
			    ! allow_special) {
				if (*p == '+' || *p == '~') {
					current_place() <<
						fmt("an unquoted name must not begin with the character %s",
						    show_text(string(1, *p)));
					throw ERROR_LOGICAL;
				}
				Place place_dash= current_place();
				++p;
				assert(p <= p_end);
				if (p == p_end) {
					current_place() << "expected a flag character";
					place_dash << fmt("after dash %s",
							  show_operator('-'));
					throw ERROR_LOGICAL;
				}
				char op= *p;
				if (! is_flag_char(op)) {
					if (isalnum(op)) {
						current_place() <<
							fmt("invalid flag %s",
							    show_prefix("-", frmt("%c", op)));
					} else {
						current_place() <<
							fmt("expected a flag character, not %s",
							    show(string(1, op)));
						place_dash <<
							fmt("after dash %s",
							    show_operator('-'));
					}
					explain_flags();
					throw ERROR_LOGICAL;
				}
				assert(isalnum(op));
				shared_ptr <Flag_Token> token= make_shared <Flag_Token>
					(op, current_place(), environment);
				tokens.push_back(token);
				++p;
				if (p < p_end &&
				    (is_name_char(*p) || *p == '"' || *p == '\'' || *p == '$' || *p == '@')) {
					current_place() <<
						fmt("expected whitespace before character %s",
						    show(string(p, 1)));
					token->get_place() <<
						fmt("after flag %s",
						    show_prefix("-", frmt("%c", op)));
					throw ERROR_LOGICAL;
				}
			} else {
				shared_ptr <Place_Name> place_name= parse_name(allow_special);
				if (place_name == nullptr) {
					if (*p == '!') {
						current_place() <<
							fmt("character %s is invalid for persistent dependencies; use %s instead",
							    show_operator('!'),
							    show_prefix("-", "p"));
					} else if (*p == '?') {
						current_place() <<
							fmt("character %s is invalid for optional dependencies; use %s instead",
							    show_operator('?'),
							    show_prefix("-", "o"));
					} else if (*p == '&') {
						current_place() <<
							fmt("character %s is invalid for trivial dependencies; use %s instead",
							    show_operator('&'),
							    show_prefix("-", "t"));
					} else {
						current_place() << fmt("invalid character %s",
								       show(string(p, 1)));
						if (strchr("#%\'\":;-$@<>={}()[]*\\&|!?,", *p)) {
							explain_quoted_characters();
						}
					}
					throw ERROR_LOGICAL;
				}
				assert(! place_name->empty());
				tokens.push_back(make_shared <Name_Token>
						 (*place_name, environment));
			}
		}
		environment &= ~E_WHITESPACE;
	had_whitespace:;
	}
}

void Tokenizer::parse_tokens_string(vector <shared_ptr <Token> > &tokens,
				    Context context,
				    Place &place_end,
				    string string_,
				    const Place &place_string)
{
	vector <Backtrace> backtraces;
	vector <string> filenames;
	set <string> includes;

	Tokenizer parse(backtraces, filenames, includes,
			place_string,
			string_.c_str(), string_.size());
	parse.parse_tokens(tokens, context, place_string);
	place_end= parse.current_place();
}

bool Tokenizer::skip_space(bool &skipped_actual_space)
{
	bool ret= false;
	skipped_actual_space= false;
	while (p < p_end && (isspace(*p) || *p == '\\')) {
		if (*p == '\\') {
			if (p+1 < p_end) {
				++p;
				if (*p == '\n') {
					++line;
					p_line= p+1;
					ret= true;
				} else {
					/* A backslash followed by something
					 * else--this is a normal name character
					 * and not a space  */
					--p;
					return ret;
				}
			} else {
				current_place() << "backslash must be followed by a character";
				throw ERROR_LOGICAL;
			}
		}
		else if (*p == '\n') {
			++line;
			p_line= p + 1;
			ret= true;
			skipped_actual_space= true;
		} else {
			ret= true;
			skipped_actual_space= true;
		}
		++p;
	}
	assert(p <= p_end);
	return ret;
}

void Tokenizer::parse_double_quote(Place_Name &ret)
{
	Place place_begin_quote= current_place();
	++p;

	while (p < p_end) {
		if (*p == '"') {
			++p;
			goto end_of_double_quote;
		} else if (*p == '$') {
			string parameter;
			Place place_dollar;
			if (parse_parameter(parameter, place_dollar)) {
				ret.append_parameter(parameter, place_dollar);
			} else {
				assert(false);
			}
		} else if (*p == '\\') {
			Place place_backslash= current_place();
			++p;
			char c;
			switch (*p) {
			case 'a':  c= '\a';  break;
			case 'b':  c= '\b';  break;
			case 'f':  c= '\f';  break;
			case 'n':  c= '\n';  break;
			case 'r':  c= '\r';  break;
			case 't':  c= '\t';  break;
			case 'v':  c= '\v';  break;
			case '\\': c= '\\';  break;
			case '\"': c= '\"';  break;
			case '$':  c = '$';  break;

			default:
				if (*p >= 33 && *p <= 126)
					place_backslash
						<< fmt("invalid escape sequence %s",
						       show_operator(frmt("\\%c", *p)));
//							Color::stderr_highlight_on, *p, Color::stderr_highlight_off);
				else 
					// TODO generate a \x?? hexadecimal sequence instead
					place_backslash
						<< fmt("invalid escape sequence %s",
						       show_operator(string(p-1, 2)));
				place_begin_quote <<
					fmt("in quote started by %s",
					    show_operator('"'));
				throw ERROR_LOGICAL;
			}
			ret.last_text() += c;
			++p;
		} else if (*p == '\0') {
			current_place() <<
				fmt("invalid character %s", show_operator('\0'));
			place_begin_quote <<
				fmt("in quote started by %s",
				    show_operator('"'));
			throw ERROR_LOGICAL;
		} else {
			if (*p == '\n') {
				++line;
				p_line= p + 1;
			}
			ret.last_text() += *p++;
		}
	}
	/* Reached end of file without closing the quote */
	current_place() << fmt("expected a closing %s", show_operator('"'));
	place_begin_quote << fmt("for quote started by %s", show_operator('"'));
	throw ERROR_LOGICAL;
 end_of_double_quote:;
}

void Tokenizer::parse_single_quote(Place_Name &ret)
{
	Place place_begin_quote= current_place();
	++p;
	while (p < p_end) {
		if (*p == '\'') {
			++p;
			goto end_of_single_quote;
		} else if (*p == '\0') {
			current_place() << fmt("invalid character %s", show(string(p, 1)));
			place_begin_quote <<
				fmt("in quote started by %s", show_operator('\''));
			throw ERROR_LOGICAL;
		} else {
			if (*p == '\n') {
				++line;
				p_line= p + 1;
			}
			ret.last_text() += *p++;
		}
	}
	/* Reached end of file without closing the quote */
	current_place() << fmt("expected a closing %s", show_operator('\''));
	place_begin_quote << fmt("for quote started by %s", show_operator('\''));
	throw ERROR_LOGICAL;
 end_of_single_quote:;
}

bool Tokenizer::parse_escape()
{
	assert(p < p_end && *p == '\\');
	Place place_escape= current_place();
	++p;
	if (p == p_end) {
		/* Error:  expected a character after \ */
		place_escape << fmt("expected a character after %s", show_operator('\\'));
		throw ERROR_LOGICAL;
	}

	/* \ followed by newline */
	if (*p == '\n') {
		/* Ignore the two */
		++p;
		return false;
	}

	if (*p == '\0') {
		place_escape << fmt
			("expected a non-nul character after %s",
			 show_operator('\\'));
		throw ERROR_LOGICAL;
	}

	return true;
}

void Tokenizer::parse_directive(vector <shared_ptr <Token> > &tokens,
				Context context,
				const Place &place_diagnostic)
{
	Place place_percent= current_place();
	assert(*p == '%');
	++p;
	bool skipped_actual_space;
	skip_space(skipped_actual_space);
	const char *const p_name= p;
	Place place_directive= current_place();

	while (p < p_end && isalnum(*p)) {
		++p;
	}

	if (p == p_name) {
		if (p < p_end)
			place_directive
				<< fmt("expected a directive name, not %s",
				       show(string(p, 1)));
		else
			place_directive
				<< "expected a directive name";
		place_percent << fmt("after %s", show_operator('%'));
		throw ERROR_LOGICAL;
	}

	const string name(p_name, p - p_name);
	skip_space(skipped_actual_space);

	if (name == "include") {
		if (context == DYNAMIC) {
			place_percent
				<< fmt("%s must not appear in dynamic dependencies",
				       show_operator("%include"));
//					Color::stderr_highlight_on, Color::stderr_highlight_off);
			throw ERROR_LOGICAL;
		}
		if (context == OPTION_C || context == OPTION_F) {
			place_percent
				<< fmt("%s must not be used",
				       show_operator("%include"));
//					Color::stderr_highlight_on, Color::stderr_highlight_off);
			throw ERROR_LOGICAL;
		}

		shared_ptr <Place_Name> place_name= parse_name(false);

		if (place_name == nullptr) {
			current_place() <<
				(p == p_end
				 ? "expected a filename"
				 : fmt("expected a filename, not %s",
				       show(string(p, 1))));
			place_percent << fmt("after %s",
					     show_operator("%include"));
//					      Color::stderr_highlight_on, Color::stderr_highlight_off);
			throw ERROR_LOGICAL;
		}
		if (place_name->get_n() != 0) {
			place_name->place <<
				fmt("name %s must not be parametrized",
				    show(*place_name));
			place_percent << fmt("after %s",
					     show_operator("%include"));
//					      Color::stderr_highlight_on, Color::stderr_highlight_off);
			throw ERROR_LOGICAL;
		}

		const string filename_include= place_name->unparametrized();
		Backtrace backtrace_stack(place_name->place,
					  fmt("%s is included from here",
					      show(filename_include)));
		backtraces.push_back(backtrace_stack);
		filenames.push_back(place_base.text);

		if (includes.count(filename_include)) {
			/* Do nothing -- file was already parsed, or is being
			 * parsed.  It is an error if a file includes itself
			 * directly or indirectly.  It it ignored if a file is
			 * included a second time non-recursively.  */
			for (auto &i: filenames) {
				if (filename_include != i)
					continue;
				vector <Backtrace> backtraces_backward;
				for (auto j= backtraces.rbegin(); j != backtraces.rend(); ++j) {
					Backtrace backtrace(*j);
					if (j == backtraces.rbegin()) {
						backtrace.message=
							fmt("recursive inclusion of %s using %s",
							    show(filename_include),
							    show_operator("%include"));
//							    Color::stderr_highlight_on, Color::stderr_highlight_off);
					}
					backtraces_backward.push_back(backtrace);
				}
				for (auto &j: backtraces_backward) {
					j.print();
				}
				throw ERROR_LOGICAL;
			}
		} else {
			/* Ignore the end place; it is only used for the
			 * top-level file  */
			Place place_end_sub;
			parse_tokens_file(tokens,
					  Tokenizer::SOURCE,
					  place_end_sub,
					  filename_include,
					  backtraces, filenames, includes,
					  place_diagnostic, -1);
		}
		backtraces.pop_back();
		filenames.pop_back();

	} else if (name == "version") {
		while (p < p_end && isspace(*p)) {
			if (*p == '\n') {
				++line;
				p_line= p + 1;
			}
			++p;
		}
		const char *const p_version= p;
		while (p < p_end && is_name_char(*p)) {
			++p;
		}
		const string version_required(p_version, p - p_version);
		Place place_version(place_base.type, place_base.text,
				    line, p_version - p_line);
		parse_version(version_required, place_version, place_percent);
	} else {
		/* Invalid directive */
		place_percent <<
			fmt("invalid directive %s",
			    show_prefix("%", name));
		throw ERROR_LOGICAL;
	}
}
