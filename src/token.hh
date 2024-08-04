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

typedef unsigned Environment;
/* Information about a token, as flags */

enum {
	E_WHITESPACE    = 1 << 0,
	/* The token is preceded by whitespace */
};

class Token
/* Mainly used through unique_ptr/shared_ptr */
{
public:
	Environment environment;

	Token(Environment environment_)
		: environment(environment_)  {  }

	virtual ~Token()= default;

	virtual const Place &get_place() const= 0;
	/* The place of the token.  May be in the middle of the token.
	 * This is the case for commands. */

	virtual const Place &get_place_start() const= 0;
	/* The starting place.  Always the first character. */

	virtual void render(Parts &, Rendering= 0) const= 0;
	/* Render only the start of the token if it is very long */
};

void render(shared_ptr <const Token> token, Parts &parts, Rendering rendering= 0);

class Operator
	: public Token
{
public:
	const char op;
	const Place place;

	Operator(char op_, Place place_, Environment environment_)
		: Token(environment_), op(op_), place(place_) {
		assert(! isalnum(op_));
	}

	const Place &get_place() const override { return place; }
	const Place &get_place_start() const override { return place; }
	void render(Parts &, Rendering= 0) const override;
};

class Flag_Token
	: public Token
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
		: Token(environment_),
		  place(place_),
		  flag(flag_)
	{
		assert(isalnum(flag));

		/* Can never be on the first column because there is a
		 * preceding dash.  */
		if (place.type == Place::Type::INPUT_FILE)
			assert(place.column > 0);
	}

	const Place &get_place() const override { return place; }

	const Place &get_place_start() const override {
		if (place_start.type == Place::Type::EMPTY) {
			place_start= place;
			if (place_start.type == Place::Type::INPUT_FILE)
				-- place_start.column;
		}
		return place_start;
	}

	void render(Parts &, Rendering= 0) const override;
};

class Name_Token
/* This contains two types of places:  the places for the individual
 * parameters in Place_Param_Name, and the place of the complete token
 * from Token.  */
	: public Token, public Place_Name
{
public:
	Name_Token(const Place_Name &place_name_,
		   bool environment_)
		: Token(environment_),
		  Place_Name(place_name_)
	{  }

	const Place &get_place() const override { return Place_Name::place; }
	const Place &get_place_start() const override { return Place_Name::place; }

	void render(Parts &parts, Rendering rendering= 0) const override {
		Place_Name::render(parts, rendering);
	}
};

class Command
/* A command delimited by braces, or the content of a file, also
 * delimited by braces.  */
	: public Token
{
private:
	mutable std::unique_ptr <std::vector <string> > lines;
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

	Command(string command_, const Place &place_, const Place &place_start_,
		Environment environment_);
	const Place &get_place() const override { return place; }
	const Place &get_place_start() const override { return place_start; }
	void render(Parts &parts, Rendering= 0) const override { parts.append_operator("{"); }
	const std::vector <string> &get_lines() const;
};

#endif /* ! TOKEN_HH */
