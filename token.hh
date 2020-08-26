#ifndef TOKEN_HH
#define TOKEN_HH

/* 
 * Data structures for representing tokens.  
 *
 * There are four types of tokens:  
 *   - operators (all are represented by single characters)
 *   - flags (e.g. "-o")
 *   - names (including all their quoting mechanisms)
 *   - commands (delimited by { }) 
 */

#include <memory>

typedef unsigned Environment;
/* Information about a token, as flags */

enum {
	E_WHITESPACE    = 1 << 0,
	/* The token is preceded by whitespace */
};

class Token
/* A token.  This class is mainly used through unique_ptr/shared_ptr.  */
{
public:

	Environment environment; 

	Token(Environment environment_)
		:  environment(environment_)
	{  }

	virtual ~Token() = default; 

	virtual const Place &get_place() const= 0; 
	/* The place of the token.  May be in the middle of the token.
	 * This is the case for commands. */

	virtual const Place &get_place_start() const= 0;
	/* The starting place.  Always the first character. */ 

	virtual string format_start_err() const= 0;
	/* Formatting of the starting character of character sequence */
};

class Operator
/* An operator, e.g. ':', '[', etc.  Operators are all single
 * characters.  */  
	:  public Token
{
public: 
	const Place place; 

	const char op; 
	/* The operator as a character, e.g. ':', '[', etc.  */

	Operator(char op_, Place place_, Environment environment_)
		:  Token(environment_),
		   place(place_),
		   op(op_)
	{ 
		assert(! isalnum(op_)); 
	}

	const Place &get_place() const {
		return place; 
	}

	const Place &get_place_start() const {
		return place; 
	}

	string format_start_err() const {
		return char_format_err(op); 
	}

	string format_long_err() const;
};

class Flag_Token
	:  public Token
{
public:

	const Place place;
	/* The place of the letter */ 

	mutable Place place_start;
	/* The place of the '-' */ 

	const char flag;
	/* The flag character */

	/* PLACE is the place of the letter */
	Flag_Token(char flag_, const Place place_, Environment environment_)
		:  Token(environment_),
		   place(place_),
		   flag(flag_)
	{
		assert(isalnum(flag)); 

		/* Can never be on the first column because there is a
		 * preceding dash.  */
		if (place.type == Place::Type::INPUT_FILE)
			assert(place.column > 0); 
	}

	const Place &get_place() const {
		return place;
	}

	const Place &get_place_start() const {
		if (place_start.type == Place::Type::EMPTY) {
			place_start= place;
			if (place_start.type == Place::Type::INPUT_FILE)
				-- place_start.column; 
		}
		return place_start;
	}

	string format_start_err() const {
		return char_format_err('-'); 
	}
};

class Name_Token
/* This contains two types of places:  the places for the individual
 * parameters in Place_Param_Name, and the place of the complete token
 * from Token.  */
	:  public Token, public Place_Name
{
public:
	Name_Token(const Place_Name &place_name_, 
		   bool environment_) 
		:  Token(environment_),
		   Place_Name(place_name_)
	{  }

	const Place &get_place() const {
		return Place_Name::place; 
	}

	const Place &get_place_start() const {
		return Place_Name::place; 
	}

	string format_start_err() const {
		return Place_Name::format_err(); 
	}
};

class Command
/* A command delimited by braces, or the content of a file, also
 * delimited by braces.  */
	:  public Token
{
private:

	mutable unique_ptr <vector <string> > lines;
	/* The individual lines of the command.  Empty lines and leading
	 * spaces are not included.  These lines are only used for
	 * output and writing content, not for execution.  May be null.
	 * Generated on demand, and therefore declared as mutable.  */ 

public:

	const string command;
	/* The command as written in the input; contains newlines */ 

	const Place place; 
	/* In general, the first non-whitespace character of the command */ 

	const Place place_start;
	/* The opening brace */ 

	Command(string command_, 
		const Place &place_,
		const Place &place_start_,
		Environment environment_); 

	const Place &get_place() const {
		return place; 
	}

	const Place &get_place_start() const {
		return place_start; 
	}

	string format_start_err() const {
		return char_format_err('{'); 
	}

	const vector <string> &get_lines() const;
};

Command::Command(string command_, 
		 const Place &place_,
		 const Place &place_start_,
		 Environment environment_)
	:  Token(environment_),
	   command(command_),
	   place(place_),
	   place_start(place_start_)
{  }	

const vector <string> &
Command::get_lines() const
/* 
 * This code parses the command string into lines ready for output.
 * Most of the code is for making the output pretty.
 *
 * We only output a command when it has a single line, but the following
 * code also handles the case of multiline commands.  We keep it because
 * we may need it in the future.
 */
{
	if (lines != nullptr) {
		return *lines; 
	}
	
	lines= unique_ptr <vector <string> > (new vector <string> ()); 

	const char *p= command.c_str();
	const char *p_end= p + command.size(); 

	/* Split into lines */ 
	while (p < p_end) {
		const char *q= p;
		while (p < p_end && *p != '\n')  ++p;
		string line(q, p - q);
		if (line.size()) {
			/* Discard lines consisting only of whitespace */ 

			bool keep= false;
			for (size_t i= 0;  i < line.size();  ++i) {
				if (! isspace(line[i]))
					keep= true;
			}

			if (keep)
				lines->push_back(line); 
		}
		if (p < p_end) {
			assert(*p == '\n');
			++p;
		}
	}

	/* Remove initial whitespace common to all lines */ 
	while (lines->size()) {
		char begin= (*lines)[0][0];
		if ((begin & 0x80) || ! isspace(begin))  break;
		bool equal= true;
		for (auto &i:  *lines) {
			assert(i.size()); 
			if (i[0] != begin)  equal= false;
		}
		if (! equal)  break;
		for (size_t i= 0; i < lines->size(); ) {
			string &line= (*lines)[i]; 
			assert(line.size()); 
			line.erase(0, 1); 
			if (line.empty())
				lines->erase(lines->begin() + i); 
			else
				++i;
		}
	}

	/* Remove whitespace at end of lines */
	for (string &line:  *lines) {
		size_t l= line.size();
		while (l != 0 && isspace(line[l - 1]))  --l;
		line.resize(l);
	}

	return *lines; 
}

string Operator::format_long_err() const
{
	string t;
	switch (op) {
	default: assert(false); break;
	case '(':  t= "opening parenthesis";  break;
	case ')':  t= "closing parenthesis";  break;
	case '[':  t= "opening bracket";      break;
	case ']':  t= "closing bracket";      break;
	case '@':  t= "operator";             break;
	}

	return fmt("%s %s", t, char_format_err(op)); 
}

#endif /* ! TOKEN_HH */
