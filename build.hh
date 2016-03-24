#ifndef BUILD_HH
#define BUILD_HH

/* Code for generating rules from a vector of tokens, i.e, for
 * performing the parsing of Stu syntax itself beyond tokenization.
 * This is a recursive descent parser. 
 */

/* A Yacc-like syntax description is given in the manpage. 
 */ 

/*
 * Operator precedence.  Higher in the list means higher precedence.  At
 * the moment, only prefix and circumfix operators exist, and thus the
 * precedence is trivial. 
 *
 * @...	     (prefix) Phony dependency; argument can only contain name
 * ---------------
 * <...	     (prefix) Input redirection; argument cannot contain '()',
 *           '[]', '$[]' or '@' 
 * ---------------
 * !...	     (prefix) Existence-only; argument cannot contain '$[]'
 * ?...	     (prefix) Optional dependency; argument cannot contain '$[]'
 * &...      (prefix) Trivial dependency
 * ---------------
 * [...]     (circumfix) Dynamic dependency; cannot contain '$[]' or '@'
 * (...)     (circumfix) Capture
 * $[...]    (circumfix) Variable inclusion; argument cannot contain
 *           '?', '[]', '()', '*' or '@' 
 */

/* As a general rule, this code does not check that imcompatible
 * constructs (like '!' and '?' or '!' and '$[') are used together.
 * Instead, this is checked within Execution and not here, because these
 * can also come from dynamic dependencies. */

#include <memory>

#include "frmt.hh"

/* An object of this type represents a location within a token list */ 
class Build
{
private:
	vector <shared_ptr <Token> > &tokens;
	vector <shared_ptr <Token> > ::iterator &iter;
	const Place place_end; 

public:

	Build(vector <shared_ptr <Token> > &tokens_,
	      vector <shared_ptr <Token> > ::iterator &iter_,
	      Place place_end_)
		:  tokens(tokens_),
		   iter(iter_),
		   place_end(place_end_)
	{ }
	
	/*
	 * Methods for building the syntax tree:  Each has a name
	 * corresponding to the symbol given by the Yacc syntax in the
	 * manpage.  The argument RET is where the result is written.
	 * If the return value is BOOL, it denotes whether something was
	 * read.  On syntax errors, LOGICAL_ERROR is thrown. 
	 */

	/* In some of the following functions, write the input filename into
	 * PLACE_NAME_INPUT.  If 
	 * PLACE_NAME_INPUT is alrady non-empty, throw an error if a second
	 * input filename is specified. 
	 * PLACE_INPUT is the place of the '<' input redirection operator. 
	 */ 

	/* The returned rules may not be unique -- this is checked later */
	void build_rule_list(vector <shared_ptr <Rule> > &ret);

	/* Return nullptr when nothing was parsed */ 
	shared_ptr <Rule> build_rule(); 

	bool build_expression_list(vector <shared_ptr <Dependency> > &ret, 
				   Place_Param_Name &place_param_name_input,
				   Place &place_input);

	bool build_expression(vector <shared_ptr <Dependency> > &ret, 
			      Place_Param_Name &place_param_name_input,
			      Place &place_input);

	shared_ptr <Dependency> build_variable_dependency(Place_Param_Name &place_param_name_input,
							  Place &place_input);

	shared_ptr <Dependency> build_redirect_dependency(Place_Param_Name &place_param_name_input,
							  Place &place_input); 

	/* Whether the next token is the given operator */ 
	bool is_operator(char op) const {
		return 
			iter != tokens.end() &&
			dynamic_pointer_cast <Operator> (*iter) &&
			dynamic_pointer_cast <Operator> (*iter)->op == op;
	}
};

void Build::build_rule_list(vector <shared_ptr <Rule> > &ret)
{
	assert(ret.size() == 0); 
	
	while (iter != tokens.end()) {

#ifndef NDEBUG
		auto iter_begin= iter; 
#endif /* ! NDEBUG */ 

		shared_ptr <Rule> rule= build_rule(); 

		if (rule == nullptr) {
			assert(iter == iter_begin); 
			(*iter)->get_place() << "expected a rule"; 
			throw ERROR_LOGICAL;
		}

		ret.push_back(rule); 
	}
}

shared_ptr <Rule> Build::build_rule()
{
	if (! (dynamic_pointer_cast <Param_Name> (*iter) || 
	       is_operator('@') ||
	       is_operator('>'))) {
		return shared_ptr <Rule> (); 
	}

	bool redirect_output= false;
	Place place_output; 

	if (is_operator('>')) {
		place_output= (*iter)->get_place();
		++iter;
		if (iter == tokens.end()) {
			place_end << "expected a filename";
			place_output << "after '>'"; 
			throw ERROR_LOGICAL;
		}
		else if (! (dynamic_pointer_cast <Param_Name> (*iter) || 
					is_operator('@'))) {
			(*iter)->get_place() << "expected a filename";
			place_output << "after '>'"; 
			throw ERROR_LOGICAL;
		}
		redirect_output= true; 
	}

	Place place_target= (*iter)->get_place();

	Type type= T_FILE;
 	if (is_operator('@')) {
		Place place_at= (*iter)->get_place();

		if (redirect_output) {
			place_at << "phony target is invalid";
			place_output << "after '>'"; 
			throw ERROR_LOGICAL;
		}

		++iter;
		if (iter == tokens.end()) {
			place_end << "expected the name of phony target";
			place_at << "after '@'";
			throw ERROR_LOGICAL;
		}
		if (! dynamic_pointer_cast <Param_Name> (*iter)) {
			(*iter)->get_place() << "expected the name of phony target";
			place_at << "after '@'";
			throw ERROR_LOGICAL;
		}

		type= T_PHONY;
	}

	/* Target */ 
	shared_ptr <Name_Token> target_name= dynamic_pointer_cast <Name_Token> (*iter);
	++iter;

	if (! target_name->valid()) {
		place_target <<
			"two parameters must be separated by at least one character";
		throw ERROR_LOGICAL;
	}

	string parameter_duplicate;
	if (target_name->has_duplicate_parameters(parameter_duplicate)) {
		place_target <<
			fmt("target contains duplicate parameter $%s", 
				parameter_duplicate); 
		throw ERROR_LOGICAL;
	}

	shared_ptr <Place_Param_Target> place_param_target
		(new Place_Param_Target(type, *target_name, place_target));

	if (iter == tokens.end()) {
		place_end << "expected a dependency or a command";
		place_target << fmt("after target %s", place_param_target->format()); 
		throw ERROR_LOGICAL;
	}

	vector <shared_ptr <Dependency> > dependencies;

	bool had_colon= false;

	/* Empty at first */ 
	Place_Param_Name filename_input;
	Place place_input;

	if (is_operator(':')) {
		had_colon= true; 
		++iter; 
		build_expression_list(dependencies, filename_input, place_input); 
	} 

	/* Command */ 
	if (iter == tokens.end()) {
		place_end << "excpected command or ';'";
		place_target << fmt("for target %s", place_param_target->format());
		throw ERROR_LOGICAL;
	}

	/* Remains nullptr when there is no command */ 
	shared_ptr <Command> command;
	/* Place of ';' */ 
	Place place_nocommand; 

	if (dynamic_pointer_cast <Command> (*iter)) {
		command= dynamic_pointer_cast <Command> (*iter);
		++iter; 
	} else {
		/* If there is no command there must be ';' */ 
		if (! is_operator(';')) {
			(*iter)->get_place() <<
				(had_colon
				 ? "expected a dependency, a command or ';'"
				 : "expected ':', a command or ';'");
			place_target <<
				fmt("for target %s", place_param_target->format());
			throw ERROR_LOGICAL;
		}
		place_nocommand= (*iter)->get_place(); 
		++iter;
	}

	/* When output redirection is present, the rule must have a command,
	 * and refer to a file (not a phony). */ 
	if (redirect_output) {
		/* Already checked before */ 
		assert(place_param_target->type == T_FILE); 

		if (command == nullptr) {
			place_output << 
				"output redirection using '>' must not be used";
			place_nocommand <<
				"in rule without a command";
			throw ERROR_LOGICAL;
		}
	}

	/* When input redirection is present, the rule must have a command */ 
	if (! filename_input.empty()) {
		if (command == nullptr) {
			place_input <<
				"input redirection using '<' must not be used";
			place_nocommand <<
				"in rule without a command";
			throw ERROR_LOGICAL;
		}
	}

	shared_ptr <Rule> ret(new Rule(place_param_target, dependencies, 
				       command, redirect_output, filename_input));

	return ret; 
}

bool Build::build_expression_list(vector <shared_ptr <Dependency> > &ret, 
				  Place_Param_Name &place_param_name_input,
				  Place &place_input)
{
	assert(ret.size() == 0);

	while (iter != tokens.end()) {
		vector <shared_ptr <Dependency> > ret_new; 
		bool r= build_expression(ret_new, place_param_name_input, place_input);
		if (!r) {
			assert(ret_new.size() == 0); 
			return ! ret.empty(); 
		}
		ret.insert(ret.end(), ret_new.begin(), ret_new.end()); 
	}

	return ! ret.empty(); 
}

bool Build::build_expression(vector <shared_ptr <Dependency> > &ret, 
			     Place_Param_Name &place_param_name_input,
			     Place &place_input)
{
	assert(ret.size() == 0); 

	/* '(' expression* ')' */ 
	if (is_operator('(')) {
		Place place_paren= (*iter)->get_place();
		++iter;
		vector <shared_ptr <Dependency> > r;
		while ( build_expression_list(r, place_param_name_input, place_input)) {
			ret.insert(ret.end(), r.begin(), r.end()); 
			r.clear(); 
		}
		if (iter == tokens.end()) {
			place_end << "expected ')'";
			place_paren << "for group started by '('"; 
			throw ERROR_LOGICAL;
		}
		if (! is_operator(')')) {
			(*iter)->get_place() << "expected ')'";
			place_paren << "for group started by '('"; 
			throw ERROR_LOGICAL;
		}
		++ iter; 
		return true; 
	} 

	/* '[' expression* ']' */
	if (is_operator('[')) {
		Place place_paren= (*iter)->get_place();
		++iter;	
		vector <shared_ptr <Dependency> > r2;
		vector <shared_ptr <Dependency> > r;
		while (build_expression_list(r, place_param_name_input, place_input)) {
			r2.insert(r2.end(), r.begin(), r.end()); 
			r.clear(); 
		}
		if (iter == tokens.end()) {
			place_end << "expected ']'";
			place_paren << "for group started by '['"; 
			throw ERROR_LOGICAL;
		}
		if (! is_operator(']')) {
			(*iter)->get_place() << "expected ']'";
			place_paren << "for group started by '['"; 
			throw ERROR_LOGICAL;
		}
		++ iter; 
		for (auto j= r2.begin();  j != r2.end();  ++j) {
			
			/* Variable dependency cannot appear within
			 * dynamic dependency */ 
			if ((*j)->has_flags(F_VARIABLE)) {
				string text= dynamic_pointer_cast <Direct_Dependency> (*j)
					->place_param_target.format_mid();
				(*j)->get_place() <<
					fmt("variable dependency $[%s] must not appear", text);
				place_paren <<
					"within dynamic dependency started by '['";
				throw ERROR_LOGICAL; 
			}

			shared_ptr <Dependency> dependency_new= 
				make_shared <Dynamic_Dependency> (0, *j);
			ret.push_back(dependency_new);
		}
		return true; 
	} 

	/* '!' single_expression */ 
 	if (is_operator('!')) {
 		Place place_exclam= (*iter)->get_place();
 		++iter; 
		if (! build_expression(ret, place_param_name_input, place_input)) {
			if (iter == tokens.end()) 
				place_end << "expected a dependency";
			else
				(*iter)->get_place() << "expected a dependency";
			place_exclam << "after '!'"; 
			throw ERROR_LOGICAL;
		}
		for (auto j= ret.begin();  j != ret.end();  ++j) {
			(*j)->add_flags(F_EXISTENCE);
			(*j)->set_place_existence(place_exclam); 
		}
		return true;
	}

	/* '?' single_expression */ 
 	if (is_operator('?')) {
 		Place place_question= (*iter)->get_place();
 		++iter; 
		if (! build_expression(ret, place_param_name_input, place_input)) {
			if (iter == tokens.end()) {
				place_end << "expected a dependency";
				place_question << "after '?'"; 
				throw ERROR_LOGICAL;
			} else {
				(*iter)->get_place() << "expected a dependency";
				place_question << "after '?'"; 
				throw ERROR_LOGICAL;
			}
		}
		if (! option_nonoptional) {
			for (auto j= ret.begin();  j != ret.end();  ++j) {
				/* D_INPUT and D_OPTIONAL cannot be used at the same
				 * time. Note: Input redirection is not allowed in
				 * dynamic dependencies, and therefore it is sufficient
				 * to check this here.     */   
				if (place_param_name_input.place.type != Place::P_EMPTY) {
					place_input <<
						"input redirection using '<' must not be used";
					place_question <<
						"in conjunction with optional dependencies using '?'"; 
					throw ERROR_LOGICAL;
				}

				(*j)->add_flags(F_OPTIONAL); 
				(*j)->set_place_optional(place_question); 
			}
		}
		return true;
	}

	/* '&' single_expression */ 
	if (is_operator('&')) {
		Place place_ampersand= (*iter)->get_place(); 
		++iter;
		if (! build_expression(ret, place_param_name_input, place_input)) {
			if (iter == tokens.end()) {
				place_end << "expected a dependency";
				place_ampersand << "after '&'"; 
				throw ERROR_LOGICAL;
			} else {
				(*iter)->get_place() << "expected a dependency";
				place_ampersand << "after '&'"; 
				throw ERROR_LOGICAL;
			}
		}
		for (auto j= ret.begin();  j != ret.end();  ++j) {
			if (! option_nontrivial)
				(*j)->add_flags(F_TRIVIAL); 
			(*j)->set_place_trivial(place_ampersand); 
		}
		return true;
	}
		
	/* '$' ; variable dependency */ 
	shared_ptr <Dependency> dependency= 
		build_variable_dependency(place_param_name_input, place_input);
	if (dependency != nullptr) {
		ret.push_back(dependency); 
		return true; 
	}

	shared_ptr <Dependency> r= 
		build_redirect_dependency(place_param_name_input, place_input); 
	if (r != nullptr) {
		ret.push_back(r);
		return true; 
	}

	return false;
}

shared_ptr <Dependency> Build
::build_variable_dependency(Place_Param_Name &place_param_name_input, 
			    Place &place_input)
{
	bool has_input= false;

	shared_ptr <Dependency> ret;

	if (! is_operator('$')) 
		return shared_ptr <Dependency> ();

	Place place_dollar= (*iter)->get_place();
	++iter;

	if (iter == tokens.end()) {
		place_end << "expected '['";
		place_dollar << "after '$'"; 
		throw ERROR_LOGICAL;
	}
	
	if (! is_operator('[')) 
		return shared_ptr <Dependency> ();

	++iter;

	Flags flags= F_VARIABLE;

	/* Flags */ 
	Place place_flag_last;
	char flag_last= 'E';
	while (is_operator('!') || is_operator('&') || is_operator('?')) {

		flag_last= dynamic_pointer_cast <Operator> (*iter)->op; 
		if (is_operator('!')) {
			place_flag_last= (*iter)->get_place();
			flags |= F_EXISTENCE; 
		} else if (is_operator('?')) {
			if (! option_nonoptional) {
				(*iter)->get_place() << 
					"optional dependency using '?' is not allowed";
				place_dollar << "within dynamic variable declaration";
				throw ERROR_LOGICAL; 
			}
		} else if (is_operator('&')) {
			place_flag_last= (*iter)->get_place();
			if (! option_nontrivial)
				flags |= F_TRIVIAL; 
		}
		++iter;
	}

	/* Input redirection with '<' */ 
	if (is_operator('<')) {
		has_input= true;
		place_input= (*iter)->get_place(); 
		++iter;
	}
	
	/* Name of variable dependency */ 
	if (! dynamic_pointer_cast <Param_Name> (*iter)) {
		(*iter)->get_place() << "expected a filename";
		if (has_input)
			place_input << "after '<'";
		else if (place_flag_last.type != Place::P_EMPTY) 
			place_flag_last << frmt("after '%c'", flag_last);
		else
			place_dollar << "after '$['";

		throw ERROR_LOGICAL;
	}
	shared_ptr <Place_Param_Name> place_param_name= 
		dynamic_pointer_cast <Place_Param_Name> (*iter);
	++iter;

	if (has_input && ! place_param_name_input.empty()) {
		place_param_name->place << 
			fmt("duplicate input redirection <%s", 
			    place_param_name->unparametrized());
		place_param_name_input.place << 
			fmt("shadows previous input redirection <%s", 
			    place_param_name_input.unparametrized()); 
		throw ERROR_LOGICAL;
	}

	/* Check that the name does not contain '=' */ 
	for (auto j= place_param_name->get_texts().begin();
		 j != place_param_name->get_texts().end();  ++j) {
		if (j->find('=') != string::npos) {
			place_param_name->place <<
				"name of variable dependency must not contain '='"; 
			throw ERROR_LOGICAL;
		}
	}

	/* Closing ']' */ 
	if (iter == tokens.end()) {
		place_end << "expected ']'";
		place_dollar << "after opening '$['";
		throw ERROR_LOGICAL;
	}
	if (! is_operator(']')) {
		(*iter)->get_place() << "expected ']'";
		place_dollar << "after opening '$['";
		throw ERROR_LOGICAL;
	}
	++iter;

	if (has_input) {
		place_param_name_input= *place_param_name;
	}

	/* The place of the variable dependency as a whole is set on the
	 * dollar sign.  It would be conceivable to also set it
	 * on the name contained in it. 
	 */
	return shared_ptr <Dependency> 
		(new Direct_Dependency
		 (flags,
		  Place_Param_Target(T_FILE, *place_param_name, place_dollar)));
}

shared_ptr <Dependency> Build::build_redirect_dependency
(Place_Param_Name &place_param_name_input,
 Place &place_input)
{
	bool has_input= false;

	if (is_operator('<')) {
		place_input= (*iter)->get_place();

		++iter;
		has_input= true; 
	}

	bool has_phony= false;
	Place place_at; 
	if (is_operator('@')) {
		place_at= (*iter)->get_place();
		if (has_input) {
			place_at << "expected a filename";
			place_input << "after '<'";
			throw ERROR_LOGICAL;
		}
		++ iter;
		has_phony= true; 
	}

	if (iter == tokens.end()) {
		if (has_input) {
			place_end << "expected a filename";
			place_input << "after '<'"; 
			throw ERROR_LOGICAL;
		} else if (has_phony) {
			place_end << "expected the name of a phony target";
			place_at << "after '@'"; 
			throw ERROR_LOGICAL; 
		} else {
			return shared_ptr <Dependency> (); 
		}
	}

	if (nullptr == dynamic_pointer_cast <Name_Token> (*iter)) {
		if (has_input) {
			(*iter)->get_place() << "expected a filename";
			place_input << "after '<'"; 
			throw ERROR_LOGICAL;
		} else if (has_phony) {
			(*iter)->get_place() << "expected the name of a phony target";
			place_at << "after '@'"; 
			throw ERROR_LOGICAL;
		} else {
			return shared_ptr <Dependency> (); 
		}
	}

	shared_ptr <Name_Token> name_token= dynamic_pointer_cast <Name_Token> (*iter);
	++iter; 

	if (has_input && ! place_param_name_input.empty()) {
		name_token->place << 
			fmt("duplicate input redirection <%s", 
			    name_token->unparametrized());
		place_param_name_input.place << 
			fmt("shadows previous input redirection <%s", 
			    place_param_name_input.unparametrized()); 
		throw ERROR_LOGICAL;
	}

	Flags flags= 0;
	if (has_input) {
		assert(place_param_name_input.empty()); 
		place_param_name_input= *name_token;
	}

	shared_ptr <Dependency> ret
		(new Direct_Dependency
		 (flags,
		  Place_Param_Target(has_phony ? T_PHONY : T_FILE,
				     *name_token,
				     has_phony ? place_at : name_token->place))); 

	return ret; 
}

#endif /* ! BUILD_HH */
