#ifndef TOKENIZER_HH
#define TOKENIZER_HH

/* 
 * Tokenization.  I.e., parsing Stu source code into an array of tokens.    
 *
 * On errors, these functions print a message and throw integers error
 * codes.   
 */

#include <sys/mman.h>

#include "token.hh"
#include "version.hh"

/* The default filename read  */
const char *const FILENAME_INPUT_DEFAULT= "main.stu"; 

class Tokenizer
{
public:
	
	enum Context { SOURCE, DYNAMIC, OPTION_C, OPTION_F };

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
	 */
	static void parse_tokens_file(vector <shared_ptr <Token> > &tokens, 
				      Context context,
				      Place &place_end,
				      string filename, 
				      const Place &place_diagnostic,
				      int fd= -1)
	{
		vector <Trace> traces;
		vector <string> filenames; 
		set <string> includes;
		parse_tokens_file(tokens, 
				  context,
				  place_end, filename, 
				  traces, filenames, includes,
				  place_diagnostic,
				  fd);
	}

	/* Parse tokens from the given TEXT.  Other arguments are
	 * identical to parse_tokens_file().  */
	static void parse_tokens_string(vector <shared_ptr <Token> > &tokens, 
					Context context,
					Place &place_end,
					string text,
					const Place &place_diagnostic);

private:

	/* Stacks of included files */ 
	vector <Trace> &traces;
	vector <string> &filenames;
	set <string> &includes;

	const Place::Type place_type;
	const string filename;

	/* Line number */
	unsigned line;

	/* Beginning of current line */ 
	const char *p_line;

	/* Current position */
	const char *p;

	/* End of input */ 
	const char *const p_end;

	/* Whether there was whitespace previously */ 
	bool whitespace= true;

	Tokenizer(vector <Trace> &traces_,
		  vector <string> &filenames_,
		  set <string> &includes_,
		  const Place::Type place_type_,
		  const string filename_,
		  const char *p_,
		  size_t length)
		:  traces(traces_),
		   filenames(filenames_),
		   includes(includes_),
		   place_type(place_type_),
		   filename(filename_),
		   line(1),
		   p_line(p_),
		   p(p_),
		   p_end(p_ + length)
	{ }

	void parse_tokens(vector <shared_ptr <Token> > &tokens, 
			  Context context,
			  const Place &place_diagnostic);

	shared_ptr <Command> parse_command();
	
	/* Returns null when no name could be parsed.  Prints and throws
	 * on other errors, including on empty names.  */ 
	shared_ptr <Place_Name> parse_name();

	/* Parse a parameter starting with '$'.  Return whether a
	 * parameter was parsed (always TRUE).  The current position
	 * must be on the 
	 * '$' character, not after it.  If a parameter is found, write
	 * it into the parameters.  */
	bool parse_parameter(string &parameter, Place &place_dollar); 

	void skip_space(); 

	Place current_place() const {
		return Place(place_type, filename, line, p - p_line); 
	}

	/*
	 * TRACES can include traces that lead to this inclusion.  TRACES must
	 * not be modified when returning, but is declared as non-const
	 * because it is used as a stack.  
	 *
	 * FILENAMES is the list of filenames parsed up to here. I.e., it has
	 * length zero for the main read file.  FILENAME should *not* be
	 * included in FILENAMES. 
	 */
	static void parse_tokens_file(vector <shared_ptr <Token> > &tokens, 
				      Context context,
				      Place &place_end,
				      string filename, 
				      vector <Trace> &traces,
				      vector <string> &filenames,
				      set <string> &includes,
				      const Place &place_diagnostic,
				      int fd= -1);

	/* Whether the given character can be used as part of a bare filename in
	 * Stu.  Note that all non-ASCII characters are allowed, and thus we
	 * don't have to distinguish UTF-8 from 8-bit encodings:  all characters
	 * with the most significant bit set will make this return TRUE. 
	 * See the file CHARACTERS for more information. 
	 * This returns TRUE for the mid-name characters '-', '+' and
	 * '~'. 
	 */
	static bool is_name_char(char);

	/* Whether the character can be a standalone operator, i.e., an
	 * operator that is not a flag.  */
	static bool is_standalone_operator_char(char);

	/* Whether the character is a valid flag */ 
	static bool is_flag_char(char); 

	/* Parse a version statement.  VERSION_REQ is the version number given after
	 * "%version", and PLACE its place. */
	static void parse_version(string version_req, 
				  const Place &place_version,
				  const Place &place_percent); 
};

void Tokenizer::parse_tokens_file(vector <shared_ptr <Token> > &tokens, 
				  Context context,
				  Place &place_end,
				  string filename, 
				  vector <Trace> &traces,
				  vector <string> &filenames,
				  set <string> &includes,
				  const Place &place_diagnostic,
				  int fd)
{
	const char *in= nullptr;
	size_t in_size;
	struct stat buf;
	FILE *file= nullptr; 

	/* 
	 * false:  use mmap()
	 * true:   use malloc()
	 */
	bool use_malloc;

	try {
		if (context == SOURCE) {
			assert(filenames.size() == 0 || 
			       filenames[filenames.size() - 1] != filename); 
			assert(includes.count(filename) == 0); 
			includes.insert(filename); 
		} else {
			assert(filenames.size() == 0);
			assert(traces.size() == 0);
		}

		/* Map empty string to stdin */ 
		if (filename == "") {
			assert(fd == -1); 
			fd= 0; 
			file= stdin;
		}

		if (fd < 0) {
			fd= open(filename.c_str(), O_RDONLY); 
			if (fd < 0) 
				goto error;
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
						if (filename != "") {
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
			if (filename != "") {
				if (fclose(file) != 0) {
					goto error_close;
				}
			}
			in= mem; 
			in_size= len;
		}

		if (0 > close(fd)) 
			goto error;

		{
			Tokenizer tokenizer(traces, filenames, includes,
					    Place::Type::INPUT_FILE, filename, 
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
		if (0 > close(fd)) 
			goto error; 
		return;

	error_close:
		close(fd);
	error:
		const char *filename_diagnostic= filename != ""
			? filename.c_str() : "<stdin>";
		if (traces.size() > 0) {
			for (auto j= traces.begin();  j != traces.end();  ++j) {
				if (j == traces.begin()) {
					j->place <<
						system_format
						(fmt("%s%%include%s %s", 
						     Color::word, Color::end,
						     name_format_word(filename_diagnostic)));
				} else
					j->print(); 
			}
		} else {
			if (place_diagnostic
			    .get_type()
			    != Place::Type::EMPTY)
				place_diagnostic << 
					system_format(name_format_word(filename_diagnostic)); 
			else
				print_error(system_format(name_format_word(filename_diagnostic))); 
		}
		throw ERROR_LOGICAL; 

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
{
	assert(p < p_end && *p == '{');

	const Place place_open(place_type, filename, line, p - p_line);

	++p;

	const char *const p_beg= p;

	/* 
	 * The following is to determine the place of the command.  These
	 * rules are intended to make the editor go to the correct place to
	 * enter a command into the editor when the command is empty. 
	 * - If there is non-whitespace in the command, the place is the
	 *   first non-whitespace character 
	 * - If the command is completely empty (i.e., {}), then the place
	 *   is the closing bracket.
	 * - If the command contains only whitespace, is on one line and is
	 *   not empty, the place is on the first whitespace character.
	 * - If the command contains only whitespace and a single newline,
	 *   then the place is the end of the first line.
	 * - If the command contains only whitespace and at least two
	 *   newlines, the place is the first character of the second line. 
	 */
	unsigned line_command= line; /* The line of the place of the command */
	unsigned column_command= p - p_line; /* The column of the place of
					      *	the command */
	const unsigned line_first= line; /* Where the command started */ 
	bool begin= true; /* We have not yet seen non-whitespace */ 

	/* Stack of opened parenthesis-like symbols to parse shell
	 * syntax.  May contain:  {'"`( */ 
	string stack= "{"; 

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
						(place_type,
						 filename, line_command, column_command); 
					return make_shared <Command> 
						(command, place_command, place_open, whitespace); 
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

	/* Reached the end of the file without closing the command */ 
	current_place() << fmt("expected a closing %s",
			       char_format_word('}'));
	place_open << fmt("for command started by %s",
			  char_format_word('{'));
	throw ERROR_LOGICAL;
}

shared_ptr <Place_Name> Tokenizer::parse_name()
{
	const char *const p_begin= p; 
	Place place_begin(place_type, filename, line, p - p_line);

	shared_ptr <Place_Name> ret= make_shared <Place_Name> ("", place_begin);

	/* Don't allow '-', '+' and '~' at beginning of name. */
	if (p < p_end) {
		if (*p == '-' || *p == '+' || *p == '~') {
			return nullptr;
		}
	}

	while (p < p_end) {
		
		if (*p == '"') {

			Place place_begin_quote(place_type, filename, line, p - p_line); 
			++p;

			while (p < p_end) {
				if (*p == '"') {
					++p;
					goto end_of_double_quote; 
				} else if (*p == '$') {
					string parameter;
					Place place_dollar;
					if (parse_parameter(parameter, place_dollar)) {
						ret->append_parameter(parameter, place_dollar);
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
								<< frmt("invalid escape sequence %s\\%c%s",  
									Color::word, *p, Color::end);
						else 
							place_backslash
								<< frmt("invalid escape sequence %s\\\\%03o%s",  
									Color::word, (unsigned char) *p, Color::end);
						place_begin_quote <<
							fmt("in quote started by %s", 
							    char_format_word('"'));
						throw ERROR_LOGICAL;
					}
					ret->last_text() += c; 
					++p;
				} else if (*p == '\0') {
					current_place() << 
						fmt("invalid character %s",
						    char_format_word('\0'));
					place_begin_quote <<
						fmt("in quote started by %s",
						    char_format_word('"')); 
					throw ERROR_LOGICAL;
				} else {
					if (*p == '\n') {
						++line;
						p_line= p + 1;
					}
					ret->last_text() += *p++; 
				}
			}
			/* Reached end of file without closing the quote */
			current_place() <<
				fmt("expected a closing %s", char_format_word('"'));
			place_begin_quote <<
				fmt("for quote started by %s",
				    char_format_word('"')); 
			throw ERROR_LOGICAL; 

		end_of_double_quote:;
		} 

		else if (*p == '\'') {
		
			Place place_begin_quote(place_type, filename, line, p - p_line); 
			++p;
			while (p < p_end) {
				if (*p == '\'') {
					++p;
					goto end_of_single_quote; 
				} else if (*p == '\0') {
					current_place() << 
						fmt("invalid character %s",
						    char_format_word('\0'));
					place_begin_quote <<
						fmt("in quote started by %s",
						    char_format_word('\'')); 
					throw ERROR_LOGICAL;
				} else {
					if (*p == '\n') {
						++line;
						p_line= p + 1;
					}
					ret->last_text() += *p++;
				}
			}
			/* Reached end of file without closing the quote */
			current_place() <<
				fmt("expected a closing %s", char_format_word('\''));
			place_begin_quote <<
				fmt("for quote started by %s",
				    char_format_word('\'')); 
			throw ERROR_LOGICAL; 
		end_of_single_quote:;

		} else if (*p == '$') {
			string parameter;
			Place place_dollar;
			if (parse_parameter(parameter, place_dollar)) {
				ret->append_parameter(parameter, place_dollar);
			} else {
				assert(false); 
			}
		}			

		else if (is_name_char(*p)) {

			/* An ordinary character */ 

			assert(p != p_begin ||
			       (*p != '-' && *p != '+' && *p != '~'));

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

	place_dollar= Place(place_type, filename, line, p - p_line); 
	++p;
	bool braces= false; 
	if (p < p_end && *p == '{') {
		++p;
		braces= true; 
	}
	Place place_parameter_name(place_type, filename, line, p - p_line); 

	const char *const p_parameter_name= p;

	while (p < p_end && (isalnum(*p) || *p == '_')) {
		++p;
	}

	if (braces) {
		if (p == p_end) {
			current_place() <<
				fmt("expected a closing %s", 
				    char_format_word('}'));
			place_dollar <<
				fmt("for parameter started by %s",
				    multichar_format_word("${")); 
			explain_parameter_character(); 
			throw ERROR_LOGICAL;
		} else if (*p != '}') {
			current_place() <<
				fmt("character %s must not appear",
				    char_format_word(*p)); 
			place_dollar <<
				fmt("in parameter started by %s",
				    multichar_format_word("${")); 
			explain_parameter_character(); 
			throw ERROR_LOGICAL;
		} 
	}

	if (p == p_parameter_name) {
		if (p < p_end) 
			place_parameter_name <<
				fmt("expected a parameter name, not %s",
				    char_format_word(*p));
		else
			place_parameter_name <<
				"expected a parameter name";
		place_dollar << fmt("after %s", char_format_word('$')); 
		throw ERROR_LOGICAL;
	}

	parameter= string(p_parameter_name, p - p_parameter_name);

	if (isdigit(parameter[0])) {
		place_parameter_name << 
			fmt("parameter name %s must not start with a digit",
			    prefix_format_word(parameter, "$")); 
		throw ERROR_LOGICAL;
	}
	if (braces)
		++p; 

	return true; 
}

bool Tokenizer::is_name_char(char c) 
{
	return (c > 0x20 && c < 0x7F /* ASCII printable character except space */ 
		&& nullptr == strchr("[]\"\':={}#<>@$;()%*\\!?|&", c))
		|| ((unsigned char) c) >= 0x80;
}

bool Tokenizer::is_standalone_operator_char(char c) 
{
	return c != '\0' && nullptr != strchr(":<>=@;()[],\\|", c);
}

bool Tokenizer::is_flag_char(char c)
{
	/* These correspond to persistent, optional and trivial
	 * dependencies, respectively.  They were '!', '?' and '&'
	 * formerly.  */
	return c == 'p' || c == 'o' || c == 't';
}

void Tokenizer::parse_version(string version_req, 
			  const Place &place_version,
			  const Place &place_percent) 
{
	/* Note:  there may be any number of version statements in Stu
	 * (in particular from multiple source files), so we don't keep
	 * track whether one has already been provided.  */ 

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

 wrong_version:
	{
		place_version <<
			fmt("requested version %s using %s%%version%s is incompatible with this Stu's version %s%s%s",
			    name_format_word(version_req),
			    Color::word, Color::end,
			    Color::word, STU_VERSION, Color::end);
		throw ERROR_LOGICAL;
	}

 error:	
	place_version <<
		fmt("expected a version number of the form "
		    "%sMAJOR.MINOR%s or %sMAJOR.MINOR.PATCH%s, "
		    "not %s",
		    Color::word, Color::end,
		    Color::word, Color::end,
		    name_format_word(version_req)); 
	place_percent <<
		fmt("after %s%%version%s",
		    Color::word, Color::end); 
		    
	throw ERROR_LOGICAL;
}

void Tokenizer::parse_tokens(vector <shared_ptr <Token> > &tokens, 
			 Context context,
			 const Place &place_diagnostic)
{
	while (p < p_end) {

		/* Operators except '$' */ 
		if (is_standalone_operator_char(*p)) {
			Place place(place_type, filename, line, p - p_line);
			tokens.push_back(make_shared <Operator> (*p, place, whitespace));
			++p;
		}

		/* Variable dependency */ 
		else if (*p == '$' && p + 1 < p_end && p[1] == '[') {
			Place place_dollar(place_type, filename, line, p - p_line);
			Place place_langle(place_type, filename, line, p + 1 - p_line);
			tokens.push_back(make_shared <Operator> ('$', place_dollar, whitespace));
			tokens.push_back(make_shared <Operator> ('[', place_langle, whitespace)); 
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
		else if (isspace(*p)) { 
			do {
				if (*p == '\n') {
					++line;  
					p_line= p + 1; 
				}
				++p; 
			} while (p < p_end && isspace(*p));
			whitespace= true;
			goto had_whitespace; 
		} 

		/* Inclusion */ 
		else if (*p == '%') {
			Place place_percent(place_type, filename, line, p - p_line); 
			++p;
			skip_space(); 

			const char *const p_name= p;

			Place place__name(place_type, filename, line, p_name - p_line); 

			while (p < p_end && isalnum(*p)) {
				++p;
			}

			if (p == p_name) {
				if (p < p_end)
					place__name
						<< fmt("expected a statement name, not %s",
						       char_format_word(*p));
				else
					place__name
						<< "expected a statement name";
				place_percent << fmt("after %s", char_format_word('%')); 
				throw ERROR_LOGICAL; 
			}

			const string name(p_name, p - p_name); 

			skip_space(); 

			if (name == "include") {

				if (context == DYNAMIC) {
					place_percent 
						<< frmt("%s%%include%s must not appear in dynamic dependencies",
							Color::word, Color::end);
					throw ERROR_LOGICAL;
				}
				if (context == OPTION_C || context == OPTION_F) {
					place_percent 
						<< frmt("%s%%include%s must not appear in the argument to the %s-%c%s option",
							Color::word, Color::end,
							Color::word, context == OPTION_C ? 'C' : 'F', Color::end); 
					throw ERROR_LOGICAL;
				}

				shared_ptr <Place_Name> place_name= parse_name(); 

				if (place_name == nullptr) {
					Place(place_type, filename, line, p - p_line) <<
						(p == p_end
						 ? "expected a filename"
						 : fmt("expected a filename, not %s", char_format_word(*p)));
					place_percent << frmt("after %s%%include%s",
							      Color::word, Color::end); 
					throw ERROR_LOGICAL;
				}
				
				if (place_name->get_n() != 0) {
					place_name->place <<
						fmt("name %s must not be parametrized",
						    place_name->format_word());
					place_percent << frmt("after %s%%include%s",
							      Color::word, Color::end); 
					throw ERROR_LOGICAL;
				}
			
				const string filename_include= place_name->unparametrized();

				Trace trace_stack
					(place_name->place,
					 fmt("%s is included from here", 
					     name_format_word(filename_include))); 

				traces.push_back(trace_stack);
				filenames.push_back(filename); 

				if (includes.count(filename_include)) {
					/* Do nothing -- file was
					 * already parsed, or is being
					 * parsed.  */  
				
					/* It is an error if a file includes
					 * itself directly or indirectly */ 
					for (auto &i:  filenames) {
						if (filename_include == i) {
							vector <Trace> traces_backward;
							for (auto j= traces.rbegin();  j != traces.rend(); ++j) {
								Trace trace(*j);
								if (j == traces.rbegin()) {
									trace.message= 
										fmt("recursive inclusion of %s using %s%%include%s", 
										    name_format_word(filename_include),
										    Color::word, Color::end);
								}
								traces_backward.push_back(trace); 
						}
							for (auto &j:  traces_backward) {
								j.print(); 
							}
							throw ERROR_LOGICAL;
						}					
					}
				} else {
					/* Ignore the end place; it is only
					 * used for the top-level file */  
					Place place_end_sub; 
					parse_tokens_file(tokens, 
							  Tokenizer::SOURCE,
							  place_end_sub, 
							  filename_include, 
							  traces, filenames, includes, 
							  place_diagnostic,
							  -1);
				}
				traces.pop_back(); 
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
				Place place_version(place_type, filename, 
						    line, p_version - p_line); 

				parse_version(version_required, place_version, place_percent); 
				
			} else {
				/* Invalid statement */ 
				place_percent << 
					fmt("invalid statement %s", 
					    prefix_format_word(name, "%")); 
				throw ERROR_LOGICAL;
			}
		}

		/* Flag, name, or invalid character */ 		
		else {
			/* Flag */ 
			if (p < p_end &&
			    (*p == '-' || *p == '+' || *p == '~')) {
				if (*p == '+' || *p == '~') {
					current_place() <<
						fmt("an unquoted name must not begin with the character %s",
						    char_format_word(*p));
					throw ERROR_LOGICAL; 
				}
				Place place_dash= current_place(); 
				++p;
				assert(p <= p_end); 
				if (p == p_end) {
					current_place() <<
						"expected a flag character";
					place_dash <<
						fmt("after dash %s",
						    char_format_word('-')); 
					throw ERROR_LOGICAL;
				}
				char op= *p;
				if (! is_flag_char(op)) {
					if (isalnum(op)) {
						current_place() <<
							fmt("invalid flag character %s",
							    char_format_word(op)); 
					} else {
						current_place() <<
							fmt("expected a flag character, not %s",
							    char_format_word(op)); 
					}
					place_dash <<
						fmt("after dash %s",
						    char_format_word('-')); 
					throw ERROR_LOGICAL; 
				}
				assert(isalnum(op)); 
				shared_ptr <Flag_Token> token= make_shared <Flag_Token> 
					(op, current_place(), whitespace); 
				tokens.push_back(token); 
				++p;
				if (p < p_end && 
				    (is_name_char(*p) || *p == '"' || *p == '\'' || *p == '$' || *p == '@')) {
					current_place() <<
						fmt("expected whitespace before character %s",
						    char_format_word(*p));
					token->get_place() <<
						fmt("after flag %s",
						    multichar_format_word
						    (frmt("-%c", op))); 
					throw ERROR_LOGICAL; 
				}
			} else {

			shared_ptr <Place_Name> place_name= parse_name();
			if (place_name == nullptr) {
				if (*p == '!') {
					current_place() <<
						fmt("character %s is invalid for persistent dependencies; use %s instead",
						    char_format_word('!'),
						    multichar_format_word("-p")); 
				} else if (*p == '?') {
					current_place() <<
						fmt("character %s is invalid for optional dependencies; use %s instead",
						    char_format_word('?'),
						    multichar_format_word("-o")); 
				} else if (*p == '&') {
					current_place() <<
						fmt("character %s is invalid for trivial dependencies; use %s instead",
						    char_format_word('&'),
						    multichar_format_word("-t")); 
				} else {
					current_place() <<
						fmt("invalid character %s", 
						    char_format_word(*p));
				}
				throw ERROR_LOGICAL;
			}
			assert(! place_name->empty());
			tokens.push_back(make_shared <Name_Token>
					 (*place_name, whitespace)); 
			}
		}
		
		whitespace= false;
		
	had_whitespace:;
	}
}

void Tokenizer::parse_tokens_string(vector <shared_ptr <Token> > &tokens, 
				Context context,
				Place &place_end,
				string string_,
				const Place &place_diagnostic)
{
	vector <Trace> traces;
	vector <string> filenames;
	set <string> includes;

	Tokenizer parse(traces, filenames, includes, 
		    Place::Type::ARGV, string_,
		    string_.c_str(), string_.size());

	parse.parse_tokens(tokens, 
			   context,
			   place_diagnostic);

	place_end= parse.current_place(); 
}

void Tokenizer::skip_space()
{
	while (p < p_end && isspace(*p)) {
		if (*p == '\n') {
			++line;  
			p_line= p + 1; 
		}
		++p; 
	}
	assert(p <= p_end); 
}

#endif /* ! TOKENIZER_HH */
