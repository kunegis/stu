#ifndef TOKEN_HH
#define TOKEN_HH

/* 
 * Data structures for representing tokens.  
 *
 * There are three types of tokens:  
 *   - operators (all single characters)
 *   - names (including all their quoting mechanisms)
 *   - commands (delimited by { }) 
 */

#include <memory>

/* 
 * A token.  This class is mainly used through unique_ptr/shared_ptr. 
 */
class Token
{
public:

	/* Whether the token is preceded by whitespace */ 
	const bool whitespace;

	Token(bool whitespace_)
		:  whitespace(whitespace_)
	{  }

	virtual ~Token(); 

	/* The place of the token.  May be in the middle of the token.
	 * This is the case for commands. */
	virtual const Place &get_place() const= 0; 

	/* The starting place.  Always the first character. */ 
	virtual const Place &get_place_start() const= 0;

	virtual string format_start_word() const= 0;
};

/* An operator, e.g. ':', '[', etc.  Operators are all single
 * characters.  */  
class Operator
	:  public Token
{
public: 

	/* The operator as a character, e.g. ':', '[', etc.  All
	 * operators are single characters.  */  
	const char op; 

	const Place place; 

	Operator(char op_, Place place_, bool whitespace_)
		:  Token(whitespace_),
		   op(op_),
		   place(place_)
		{ }

	const Place &get_place() const {
		return place; 
	}

	const Place &get_place_start() const {
		return place; 
	}

	string format_start_word() const {
		return char_format_word(op); 
	}
};

/* This contains two types of places:  the places for the individual
 * parameters in Place_Param_Name, and the place of the complete token
 * from Token.  */
class Name_Token
	:  public Token, public Place_Param_Name
{
public:
	Name_Token(const Place_Param_Name &place_param_name_, 
		   bool whitespace_) 
		:  Token(whitespace_),
		   Place_Param_Name(place_param_name_)
	{  }

	const Place &get_place() const {
		return Place_Param_Name::place; 
	}

	const Place &get_place_start() const {
		return Place_Param_Name::place; 
	}

	string format_start_word() const {
		return Place_Param_Name::format_word(); 
	}
};

/* A command delimited by braces, or the content of a file, also
 * delimited by braces.  */
class Command
	:  public Token
{
private:
	/* The individual lines of the command.  Empty lines and leading
	 * spaces are not included.  These lines are only used for
	 * output and writing content, not for execution.  May be null.
	 * Generated on demand, and therefore declared as mutable.  */ 
	mutable unique_ptr <vector <string> > lines;

public:

	/* The command as written in the input; contains newlines */ 
	const string command;

	/* In general, the first non-whitespace character of the command */ 
	const Place place; 

	/* The opening brace */ 
	const Place place_start;

	Command(string command_, 
		const Place &place_,
		const Place &place_start_,
		bool whitespace_); 

	const Place &get_place() const {
		return place; 
	}

	const Place &get_place_start() const {
		return place_start; 
	}

	string format_start_word() const {
		return char_format_word('{'); 
	}

	const vector <string> &get_lines() const;
};

Token::~Token() { }

Command::Command(string command_, 
		 const Place &place_,
		 const Place &place_start_,
		 bool whitespace_)
	:  Token(whitespace_),
	   command(command_),
	   place(place_),
	   place_start(place_start_)
{  }	

const vector <string> &
Command::get_lines() const
{
	if (lines != nullptr) {
		return *lines; 
	}
	
	lines= unique_ptr <vector <string> > (new vector <string> ()); 

	/* The following code parses the command string into lines ready for
	 * output.  Most of the code is for making the output pretty. */   

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
				if (! is_space(line[i]))
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
		if ((begin & 0x80) || ! is_space(begin))  break;
		bool equal= true;
		for (auto &i:  *lines) {
			assert(i.size()); 
			if (i[0] != begin)  equal= false;
		}
		if (! equal)  break;
		for (unsigned i= 0; i < lines->size(); ) {
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
		int l= line.size();
		while (l != 0 && is_space(line[l - 1]))  --l;
		line.resize(l);
	}

	return *lines; 
}

#endif /* ! TOKEN_HH */
