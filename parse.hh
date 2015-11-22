#ifndef PARSE_HH
#define PARSE_HH

/* Coding for tokenization, i.e., parsing Stu source code into an array
 * of tokens.  
 */

#include <memory>
#include <algorithm>

#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>

#include "version.hh"

/* The default filename read.  Must be "main.stu". 
 */
#define FILENAME_INPUT_DEFAULT "main.stu"

shared_ptr <Command> parse_command(const string filename, 
				   unsigned &line, 
				   const char *&p_line,
				   const char *&p, 
				   const char *p_end);

shared_ptr <Place_Param_Name> parse_name(string filename,
					 unsigned &line,
					 const char *&p_line,
					 const char *&p,
					 const char *p_end);

/* Whether the given character can be used as part of a bare filename in
 * Stu.  Note that all non-ASCII characters are allowed, and thus we
 * don't have to distinguish UTF-8 from 8-bit encodings:  all characters
 * with the most significant bit set will make this return TRUE. 
 * See the file CHARACTERS for more information. 
 */
bool is_name_char(char);

bool is_operator_char(char);

void parse_version(string version, const Place &place_version); 

/*
 * Parse the tokens in a file.
 *
 * The given file descriptor FD may optionally be that file already
 * opened.  If the file was not yet opened, FD must be smaller than 0. 
 *
 * Append the read tokens to TOKENS. 
 * 
 * TRACES can include traces that lead to this inclusion.  TRACES must
 * not be modified when returning.  
 *
 * FILENAMES is the list of filenames parsed up to here. I.e., it has
 * length zero for the main read file.  FILENAME should *not* be
 * included in FILENAMES. 
 *
 * Throws integers as errors. 
 */
void parse(vector <shared_ptr <Token> > &tokens, 
	   Place &place_end,
	   string filename, 
	   bool allow_include,
	   vector <Trace> &traces,
	   vector <string> &filenames,
	   int fd= -1)
{
	const char *in= nullptr;
	struct stat buf;

	try {

		static unordered_set <string> includes;

		if (allow_include) {
			assert(filenames.size() == 0 || filenames.at(filenames.size() - 1) != filename); 
			assert(includes.count(filename) == 0); 
			includes.insert(filename); 
		} else {
			assert(filenames.size() == 0);
			assert(traces.size() == 0);
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
			if (filename.at(filename.size() - 1) != '/')
				filename += '/';
			filename += FILENAME_INPUT_DEFAULT;
			int fd2= openat(fd, FILENAME_INPUT_DEFAULT, O_RDONLY);
			if (fd2 < 0) 
				goto error_close;
			close(fd);
			fd= fd2;
			if (fstat(fd, &buf) < 0) 
				goto error_close;
		}

		/* Handle a file of zero length separately because mmap() may fail
		 * on it, i.e., return an error and refuse to create a memory
		 * map of length zero. */  
		if (buf.st_size == 0) {
			return;
		}

		const char *p, *p_end, *p_line; 
		unsigned line; 

		in= (char *)mmap(nullptr, buf.st_size, 
				 PROT_READ, MAP_SHARED, fd, 0); 
		if (in == MAP_FAILED) {
			goto error_close;
		}

		p= in; /* current parse pointer */ 
		p_end= p + buf.st_size;  /* end of buffer (*not* '\0'-indicated) */ 
		p_line= p; /* beginning of current line */ 
		line= 1;  /* current line number, one-based */ 

		while (p < p_end) {

			/* Operators except '$' */ 
			if (is_operator_char(*p)) {
				Place place(filename, line, p - p_line);
				tokens.push_back(shared_ptr <Token> (new Operator(*p, place)));
				++p;
			}

			/* Variable dependency */ 
			else if (*p == '$' && p + 1 < p_end && p[1] == '[') {
				Place place_dollar(filename, line, p - p_line);
				Place place_langle(filename, line, p + 1 - p_line);
				tokens.push_back(shared_ptr <Token> (new Operator('$', place_dollar)));
				tokens.push_back(shared_ptr <Token> (new Operator('[', place_langle))); 
				p += 2;
			}

			/* Command */ 
			else if (*p == '{') {
				shared_ptr <Command> token= parse_command(filename, line, p_line, p, p_end);
				tokens.push_back(token); 
			}

			/* Comment */ 
			else if (*p == '#') {
				/* Skip the comment without generating any token */ 
				do  ++p;  while (p < p_end && *p != '\n');
			} 

			/* Whitespace */
			else if (is_space(*p)) { 
				do {
					if (*p == '\n') {
						++line;  
						p_line= p + 1; 
					}
					++p; 
				} while (p < p_end && is_space(*p));
			} 

			/* Inclusion */ 
			else if (*p == '%') {
				Place place_percent(filename, line, p - p_line); 
				++p;
				while (p < p_end && is_space(*p)) {
					if (*p == '\n') {
						++line;  
						p_line= p + 1; 
					}
					++p; 
				}
				assert(p <= p_end); 

				const char *const p_name= p;

				Place place_name(filename, line, p_name - p_line); 

				while (p < p_end && isalnum(*p)) {
					++p;
				}

				if (p == p_name) {
					place_name << "'%' must be followed by statement name"; 
					throw ERROR_LOGICAL; 
				}

				const string name(p_name, p - p_name); 

				if (name == "include") {

					if (! allow_include) {
						place_percent <<
							"'%include' is not allowed in dynamic dependencies"; 
						throw ERROR_LOGICAL;
					}

					if (! (p < p_end && is_space(*p))) {
						place_percent << "'%include' must be followed by filename";
						throw ERROR_LOGICAL;
					}

					++p;

					shared_ptr <Place_Param_Name> place_param_name= parse_name
						(filename, line, p_line, p, p_end); 

					if (place_param_name->get_n() != 0) {
						place_param_name->place <<
							"name must not be parametrized in file inclusion";
						throw ERROR_LOGICAL;
					}
			
					const string filename_include= place_param_name->unparametrized();

					Trace trace_stack
						(place_param_name->place,
						 fmt("'%s' is included from here", filename_include));

					traces.push_back(trace_stack);
					filenames.push_back(filename); 

					if (includes.count(filename_include)) {
						/* Do nothing -- file was already parsed, or is being parsed */  
				
						/* It is an error if a file includes
						 * itself directly or indirectly */ 
						for (auto i= filenames.begin();  i != filenames.end();  ++i) {
							if (filename_include == *i) {
								vector <Trace> traces_backward;
								for (auto j= traces.rbegin();  j != traces.rend(); ++j) {
									Trace trace(*j);
									if (j == traces.rbegin()) {
										trace.message= fmt("recursive inclusion of '%s'", 
														   filename_include);
									}
									traces_backward.push_back(trace); 
								}
								for (auto j= traces_backward.begin();
									 j != traces_backward.end();  ++j) {
									j->print(); 
								}
								throw ERROR_LOGICAL;
							}					
						}
					} else {
						/* Ignore the end place; it is only
						 * used for the top-level file */  
						Place place_end_sub; 
						parse(tokens, place_end_sub, 
							  filename_include, true, 
							  traces, filenames, -1);
					}
					traces.pop_back(); 
					filenames.pop_back(); 
				} else if (name == "version") {
					while (p < p_end && is_space(*p)) {
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
					Place place_version(filename, line, p_version - p_line); 

					parse_version(version_required, place_version); 
				
				} else {
					/* Unknown statement */ 
					place_percent << fmt("invalid statement '%%%s'", name);
					throw ERROR_LOGICAL;
				}
			}

			/* Invalid token:  everything else is considered invalid */ 
			else if (*p == '}' 
					 || ((unsigned char)*p) < 0x20 /* ASCII control characters */ 
					 || *p == 0x7F /* DEL */ 
					 ) {
				string text_char= 
					*p == '\0' ? "\\0" :
					*p == '}'  ? "}"   :
					frmt("\\%03o", (unsigned char) *p);
				Place(filename, line, p - p_line) <<
					fmt("invalid character '%s'", text_char);
				throw ERROR_LOGICAL;
			}

			/* Name */ 		
			else {
				shared_ptr <Place_Param_Name> place_param_name= parse_name
					(filename, line, p_line, p, p_end);
				assert(! place_param_name->empty());
				tokens.push_back(make_shared <Name_Token> (*place_param_name)); 
			}
		}

		place_end= Place(filename, line, p - p_line); 

		if (0 > munmap((void *)in, buf.st_size))
			goto error_close;

		if (0 > close(fd))
			goto error;

		return;

	error_close:
		close(fd);
	error:
		if (traces.size() > 0) {
			for (auto j= traces.begin();  j != traces.end();  ++j) {
				if (j == traces.begin()) {
					j->print_beginning();
					perror(filename.c_str()); 
				} else
					j->print(); 
			}
		} else
			perror(filename.c_str()); 
		exit(ERROR_SYSTEM); 

	} catch (int error) {
		if (in != NULL)
			munmap((void *)in, buf.st_size);
		close(fd);
		throw;
	}
}


shared_ptr <Command> parse_command(const string filename, 
				   unsigned &line, 
				   const char *&p_line,
				   const char *&p, 
				   const char *p_end)
{
	assert(p < p_end && *p == '{');

	const Place place_open(filename, line, p - p_line);

	++p;

	const char *const p_beg= p;

	/* The following is to determine the place of the command.  These
	 * rules are intended to make the editor go to the right place to
	 * enter a command into the editor when the command is empty. 
	 * - If there is non-whitespace in the command, the place is the
	 *   first non-whitespace character 
	 * - If the command is completely empty (i.e., {}), then the place
	 *   is the closing bracket.
	 * - If the command contains only whitespace, is one one line and is
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

	/* May contain:  {'"`( */ 
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
			} else if (! is_space(*p)) {
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
					const Place place_command(filename, line_command, column_command); 
					return shared_ptr <Command> (new Command(command, place_command)); 
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
			if (last == '{' || last == '(' || last == '`' || last == '(') {
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
	Place(filename, line, p - p_line) << "unfinished command";
	place_open << "beginning of command";
	throw ERROR_LOGICAL;
}

shared_ptr <Place_Param_Name> parse_name(string filename,
					 unsigned &line,
					 const char *&p_line,
					 const char *&p,
					 const char *p_end)
{
	Place place_begin(filename, line, p - p_line);

	shared_ptr <Place_Param_Name> ret{new Place_Param_Name("", place_begin)};

	while (p < p_end) {
		
		if (*p == '\'' || *p == '"') {
			char begin= *p; 
			Place place_begin_quote(filename, line, p - p_line); 
			++p;
			while (p < p_end) {
				if (*p == '\'' || *p == '"') {
					if (*p != begin) {
						Place(filename, line, p - p_line) <<
							"wrong closing quote";
						place_begin_quote << "opening quote";
						throw ERROR_LOGICAL;
					}
					++p;
					break;
				} else if (*p == '\\') {
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
					case '\'': c= '\'';  break;
					case '\"': c= '\"';  break;
					case '?':  c= '\?';  break;

					default:
						Place(filename, line, p - p_line) <<
							frmt("invalid escape sequence '\\%c'", *p);
						throw ERROR_LOGICAL;
					}
					ret->last_text() += c; 
					++p;
				} else if (*p == '\n') {
					Place(filename, line, p - p_line) <<
						"unfinished quoted string";
					place_begin_quote <<
						"beginning of quote"; 
					throw ERROR_LOGICAL;
				} else if (*p == '\0') {
					Place(filename, line, p - p_line) <<
						"names must not contain '\\0'"; 
					throw ERROR_LOGICAL;
				} else {
					ret->last_text() += *p++; 
				}
			}
		} 

		else if (*p == '$') {
			Place place_dollar(filename, line, p - p_line); 
			++p;
			bool braces= false; 
			if (p < p_end && *p == '{') {
				++p;
				braces= true; 
			}
			Place place_parameter_name(filename, line, p - p_line); 
			const char *const p_parameter_name= p;
			while (p < p_end && (isalnum(*p) || *p == '_')) {
				++p;
			}
			if (braces) {
				if (p == p_end || *p != '}') {
					Place(filename, line, p - p_line) <<
						"invalid character in parameter"; 
					throw ERROR_LOGICAL;
				} 
			}
			if (p == p_parameter_name) {
				place_parameter_name << "'$' must be followed by parameter name";
				throw ERROR_LOGICAL;
			}
			if (isdigit(p_parameter_name[0])) {
				place_parameter_name << "parameter name must not start with a digit"; 
				throw ERROR_LOGICAL;
			}
			const string parameter(p_parameter_name, p - p_parameter_name);
			if (braces)
				++p; 
			ret->append_parameter(parameter, place_dollar);
		}			

		else if (is_name_char(*p)) {
			/* Ordinary character */ 
			ret->last_text() += *p++;
		}
		else {
			/* As soon as the parametrized name cannot be parsed
			 * further, stop parsing */  
			break;
		}	
	}

	if (ret->empty()) {
		place_begin << "name must not be empty";
		throw ERROR_LOGICAL;
	}

	return ret;
}

bool is_name_char(char c) 
{
	// TODO replace by lookup table (?)
	return
		(c >= 0x20 && c <0x7F /* ASCII printable character */ 
		 && c != ' ' && c != '\n' && c != '\t' && c != '\f' && c != '\v' && c != '\r'
		 && c != '[' && c != ']' && c != '"' && c != '\'' 
		 && c != ':' && c != '=' && c != '{' && c != '}' 
		 && c != '#' && c != '<' && c != '>' && c != '@' 
		 && c != '$' && c != ';' && c != '(' && c != ')' 
		 && c != '%' && c != '*' && c != '\\'
		 && c != '!' && c != '?' && c != '|' && c != '&'
		 && c != ',')
		|| ((unsigned char)c) >= 0x80
		;
}

bool is_operator_char(char c) 
{
	// TODO replace by lookup table (?)
	return 
		c == ':' || c == '<' || c == '>' || c == '=' ||
		c == '@' || c == ';' || c == '(' ||
		c == ')' || c == '?' || c == '[' || c == ']' ||
		c == '!' || c == '&' || c == ',' || c == '\\' ||
		c == '|';
}

void parse_version(string version_req, const Place &place_version) 
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
	if ((size_t)chars != version_req.size())
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
			fmt("requested version %s is incompatible with this Stu's version %s",
				version_req, STU_VERSION);
		throw ERROR_LOGICAL;
	}

 error:	
	place_version <<
		fmt("invalid version number '%s'; "
			"required version number must be of the form "
			"MAJOR.MINOR or MAJOR.MINOR.PATCH", 
			version_req);
	throw ERROR_LOGICAL;
}

#endif /* ! PARSE_HH */
