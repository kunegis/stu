#ifndef TOKEN_HH
#define TOKEN_HH

/* Data structures for representing tokens. 
 */

/* Is the character a space in the C locale? 
 * Note:  we don't use isspace() because isspace() uses the current
 * locale and may (in principle) consider locale-specific characters,
 * whereas the syntax of Stu specifies that only these six characters
 * count as whitespace. 
 */
bool is_space(char c) 
{
	return c != '\0' && nullptr != strchr(" \n\t\v\r\f", c);
}

class Token
{
public:

	virtual ~Token(); 
	
	virtual const Place &get_place() const= 0; 
};

/* An operator, e.g. ':', '[', etc.  Operators are all single characters
 * in Stu. 
 */
class Operator
	:  public Token
{
public: 

	Place place; 

	/* The operator as a character, e.g. ':', '[', etc.  Note that all
	 * operators are single characters. 
	 */ 
	char op; 

	Operator(char op_, Place place_)
		:  place(place_),
		   op(op_)
		{ }

	const Place &get_place() const {
		return place; 
	}
};

/* This contains two types of places:  the places for the individual
 * parameters in Place_Param_Name, and the place of the complete token
 * from Token. 
 */
class Name_Token
	:  public Token, public Place_Param_Name
{
public:
	Name_Token(const Place_Param_Name &place_param_name_) 
		:  Place_Param_Name(place_param_name_)
	{  }

	const Place &get_place() const {
		return Place_Param_Name::place; 
	}
};

/* A command delimited by braces. 
 */
class Command
	:  public Token
{
private:
	/* The individual lines of the command. 
	 * Empty lines and leading spaces are not included.  These lines
	 * are only used for output, not for execution. 
	 * May be NULLPTR.  Generated on demand. 
	 */ 
	
	unique_ptr <vector <string> > lines;

public:

	Place place; 

	/* The command as written in the input; contains newlines  */ 
	const string command;

	Command(string command_, 
		const Place &place_);

	const Place &get_place() const {
		return place; 
	}

	const vector <string> &get_lines();
};

Token::~Token() { }

Command::Command(string command_, 
		 const Place &place_)
	:  place(place_),
	   command(command_)
{
}	

const vector <string> &
Command::get_lines()
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
				if (! is_space(line.at(i)))
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

	return *lines; 
}

#endif /* ! TOKEN_HH */
