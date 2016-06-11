#ifndef BUILD_HH
#define BUILD_HH

/* Code for generating rules from a vector of tokens, i.e, for
 * performing the parsing of Stu syntax itself beyond tokenization.
 * This is a recursive descent parser. 
 * 
 * A Yacc-like description of Stu syntax is given in the manpage. 
 */ 

#include <set>

#include "rule.hh"
#include "token.hh"

/*
 * Operator precedence.  Higher in the list means higher precedence.  At
 * the moment, only prefix and circumfix operators exist, and thus the
 * precedence is trivial. 
 *
 * @...	     (prefix) Transient dependency; argument can only contain name
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

/* This code does not check that imcompatible constructs (like '!' and
 * '?' or '!' and '$[') are used together.  Instead, this is checked
 * within Execution and not here, because these can also come from
 * dynamic dependencies.  */  

/* An object of this type represents a location within a token list */ 
class Build
{
public:

	/*
	 * Methods for building the syntax tree:  Each has a name
	 * corresponding to the symbol given by the Yacc syntax in the
	 * manpage.  The argument RET (if it is used) is where the
	 * result is written. 
	 * If the return value is BOOL, it denotes whether something was
	 * read or not.  On syntax errors, ERROR_LOGICAL is thrown. 
	 */

	/* In some of the following functions, write the input filename into
	 * PLACE_NAME_INPUT.  If PLACE_NAME_INPUT is alrady non-empty,
	 * throw an error if a second input filename is specified.
	 * PLACE_INPUT is the place of the '<' input redirection
	 * operator.  
	 */ 


	static void get_rule_list(vector <shared_ptr <Rule> > &rules,
				  vector <shared_ptr <Token> > &tokens,
				  const Place &place_end);

	/* DEPENDENCIES is filled.  DEPENDENCIES is empty when called.
	 * TARGET is used for error messages.  Empty when in a dynamic
	 * dependency. 
	 */
	static void get_expression_list(vector <shared_ptr <Dependency> > &dependencies,
					vector <shared_ptr <Token> > &tokens,
					const Place &place_end,
					Place_Param_Name &input,
					Place place_input);

	/* Parse a dependency as given on the command line outside of
	 * options */
	static shared_ptr <Dependency> get_target_dependency(string text); 

private:

	Build(vector <shared_ptr <Token> > &tokens_,
	      vector <shared_ptr <Token> > ::iterator &iter_,
	      const Place &place_end_)
		:  tokens(tokens_),
		   iter(iter_),
		   place_end(place_end_)
	{ }
	
	/* The returned rules may not be unique -- this is checked later */
	void build_rule_list(vector <shared_ptr <Rule> > &ret);

	/* RET is filled.  RET is empty when called. */
	bool build_expression_list(vector <shared_ptr <Dependency> > &ret, 
				   Place_Param_Name &place_param_name_input,
				   Place &place_input,
				   const vector <shared_ptr <Place_Param_Target> > &targets);

	/* Return null when nothing was parsed */ 
	shared_ptr <Rule> build_rule(); 

	bool build_expression(vector <shared_ptr <Dependency> > &ret, 
			      Place_Param_Name &place_param_name_input,
			      Place &place_input,
			      const vector <shared_ptr <Place_Param_Target> > &targets);

	shared_ptr <Dependency> build_variable_dependency(Place_Param_Name &place_param_name_input,
							  Place &place_input,
							  const vector <shared_ptr <Place_Param_Target> > &targets);

	shared_ptr <Dependency> build_redirect_dependency(Place_Param_Name &place_param_name_input,
							  Place &place_input,
							  const vector <shared_ptr <Place_Param_Target> > &targets);

	/* Whether the next token is the given operator */ 
	bool is_operator(char op) const {
		return 
			iter != tokens.end() &&
			is <Operator> () &&
			is <Operator> ()->op == op;
	}

	/* If the next token is of type T, return it, otherwise return
	 * null. 
	 */ 
	template <typename T>
	shared_ptr <T> is() const {
		return dynamic_pointer_cast <T> (*iter); 
	}

	vector <shared_ptr <Token> > &tokens;
	vector <shared_ptr <Token> > ::iterator &iter;
	const Place place_end; 

	/* If TO ends in '/', append to it the part of FROM that
	 * comes after the last slash, or the full target if it contains
	 * no slashes.  Parameters are not considered for containing
	 * slashes */
	static void append_copy(      Param_Name &to,
				const Param_Name &from);
};

void Build::build_rule_list(vector <shared_ptr <Rule> > &ret)
{
	assert(ret.size() == 0); 
	
	while (iter != tokens.end()) {

#ifndef NDEBUG
		const auto iter_begin= iter; 
#endif /* ! NDEBUG */ 

		shared_ptr <Rule> rule= build_rule(); 

		if (rule == nullptr) {
			assert(iter == iter_begin); 
			break;
		}

		ret.push_back(rule); 
	}
}

shared_ptr <Rule> Build::build_rule()
{
	/* Used to check that when this function fails (i.e., returns
	 * null), is has not read any tokens. */ 
	const auto iter_begin= iter;

	/* T_EMPTY when output is not redirected */
	Place place_output; 

	/* Index of the target that has the output, or -1 */ 
	int redirect_index= -1; 

	vector <shared_ptr <Place_Param_Target> > place_param_targets; 

	while (iter != tokens.end()) {
		
		if (is_operator('>')) {
			if (! place_output.empty()) {
				(*iter)->get_place() <<
					fmt("duplicate output redirection using %s",
					    char_format_err('>'));
				place_output <<
					fmt("shadows previous output redirection %s>%s%s",
					    Color::beg_name_bare, place_param_targets[redirect_index]->format_semi(), Color::end_name_bare); 
				throw ERROR_LOGICAL;
			}
			place_output= (*iter)->get_place();
			assert(! place_output.empty()); 
			redirect_index= place_param_targets.size(); 
			++iter;
			if (iter == tokens.end()) {
				place_end << "expected a filename";
				place_output << fmt("after %s", 
						    char_format_err('>'));
				throw ERROR_LOGICAL;
			}
			else if (! (is <Name_Token> () || 
				    is_operator('@'))) {
				(*iter)->get_place() << "expected a filename";
				place_output << fmt("after %s", char_format_err('>'));
				throw ERROR_LOGICAL;
			}
		}

		Place place_target= (*iter)->get_place();

		Type type= Type::FILE;

		if (is_operator('@')) {
			Place place_at= (*iter)->get_place();

			if (! place_output.empty()) {
				place_at << "transient target is invalid";
				place_output << fmt("after %s", char_format_err('>'));
				throw ERROR_LOGICAL;
			}

			++iter;
			if (iter == tokens.end()) {
				place_end << "expected the name of transient target";
				place_at << fmt("after %s", char_format_err('@'));
				throw ERROR_LOGICAL;
			}
			if (! is <Name_Token> ()) {
				(*iter)->get_place() << "expected the name of transient target";
				place_at << fmt("after %s", char_format_err('@'));
				throw ERROR_LOGICAL;
			}

			type= Type::TRANSIENT;
		}
		
		if (! is <Name_Token> ()) 
			break;

		/* Target */ 
		shared_ptr <Name_Token> target_name= is <Name_Token> ();
		++iter;

		string param_1, param_2;
		if (! target_name->valid(param_1, param_2)) {
			place_target <<
				fmt("the two parameters %s and %s in the name %s must be separated by at least one character",
				    name_format_err('$' + param_1),
				    name_format_err('$' + param_2),
				    target_name->format_err()); 
				    
			throw ERROR_LOGICAL;
		}

		string parameter_duplicate;
		if ((parameter_duplicate= target_name->get_duplicate_parameter()) != "") {
			place_target <<
				fmt("target contains duplicate parameter %s$%s%s", 
				    Color::beg_name_bare, parameter_duplicate, Color::end_name_bare); 
			throw ERROR_LOGICAL;
		}

		shared_ptr <Place_Param_Target> place_param_target= make_shared <Place_Param_Target>
			(type, *target_name, place_target);

		place_param_targets.push_back(place_param_target); 
	}

	if (place_param_targets.size() == 0) {
		assert(iter == iter_begin); 
		return nullptr; 
	}

	/* Check that all targets have the same set of parameters */ 
	set <string> parameters_0;
	for (const string &parameter:  place_param_targets[0]->place_param_name.get_parameters()) {
		parameters_0.insert(parameter); 
	}
	assert(place_param_targets.size() >= 1); 
	for (unsigned i= 1;  i < place_param_targets.size();  ++i) {
		set <string> parameters_i;
		for (const string &parameter:  place_param_targets[i]->place_param_name.get_parameters()) {
			parameters_i.insert(parameter); 
		}
		if (parameters_i != parameters_0) {
			place_param_targets[i]->place <<
				fmt("parameters of target %s differ", 
				    place_param_targets[i]->format_err());
			place_param_targets[0]->place <<
				fmt("from parameters of target %s in rule with multiple targets",
				    place_param_targets[0]->format_err()); 
			throw ERROR_LOGICAL;
		}
	}

	if (iter == tokens.end()) {
		place_end << 
			fmt("expected a command, %s, %s, or %s",
			    char_format_err(':'), char_format_err(';'), char_format_err('=')); 
		place_param_targets.back()->place
			<< fmt("after target %s", 
			       place_param_targets.back()->format_err()); 
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
		build_expression_list(dependencies, filename_input, place_input, place_param_targets); 
	} 

	/* Command */ 
	if (iter == tokens.end()) {
		if (had_colon)
			place_end << fmt("expected a dependency, a command, or %s",
					 char_format_err(';'));
		else
			place_end << fmt("expected a command, %s, %s, or %s",
					 char_format_err(';'), 
					 char_format_err(':'),
					 char_format_err('='));
		place_param_targets[0]->place
			<< fmt("for target %s", place_param_targets[0]->format_err());
		throw ERROR_LOGICAL;
	}

	/* Remains null when there is no command */ 
	shared_ptr <Command> command;

	/* When command is not null, whether the command is a command or
	 * hardcoded content */
	bool is_hardcode;

	/* Place of ';' */ 
	Place place_nocommand; 

	/* Place of '=' */
	Place place_equal;

	/* Name of the copy-from file */ 
	shared_ptr <Name_Token> name_copy;

	if (command= is <Command> ()) {
		++iter; 
		is_hardcode= false;
	} else if (! had_colon && is_operator('=')) {
		place_equal= (*iter)->get_place(); 
		++iter;

		if (iter == tokens.end()) {
			place_end << fmt("expected a filename or %s",
					 char_format_err('{')); 
			place_equal << fmt("after %s", 
					    char_format_err('=')); 
			throw ERROR_LOGICAL;
		}

		if (command= is <Command> ()) {
			/* Hardcoded content */ 
			++iter; 
			assert(place_param_targets.size() != 0); 
			if (place_param_targets.size() != 1) {
				place_equal << fmt("there must not be assigned content using %s", char_format_err('=')); 
				place_param_targets[0]->place << 
					fmt("in rule for %s... with multiple targets",
					    place_param_targets[0]->format_err()); 
					    
				throw ERROR_LOGICAL; 
			}
			if (place_param_targets[0]->type == Type::TRANSIENT) {
				place_equal << fmt("there must not be assigned content using %s", char_format_err('=')); 
				place_param_targets[0]->place <<
					fmt("for transient target %s", 
					    place_param_targets[0]->format_err()); 
				throw ERROR_LOGICAL; 
			}
			/* No redirected output is checked later */ 
			is_hardcode= true; 
		} else {
			Place place_flag_exclam;

			while (is_operator('!')) {
				place_flag_exclam= (*iter)->get_place();
				++iter;
			}

			if (name_copy= is <Name_Token> ()) {
				/* Copy rule */ 
				++iter;

				/* Check that the source file contains
				 * only parameters that also appear in
				 * the target */
				set <string> parameters;
				for (auto &parameter:  place_param_targets[0]->place_param_name.get_parameters()) {
					parameters.insert(parameter); 
				}
				for (unsigned jj= 0;  jj < name_copy->get_n();  ++jj) {
					string parameter= 
						name_copy->get_parameters()[jj]; 
					if (parameters.count(parameter) == 0) {
						name_copy->places[jj] <<
							fmt("parameter %s$%s%s must not appear in copied file %s", 
							    Color::beg_name_bare, name_format_bare(parameter), Color::end_name_bare,
							    name_copy->format_err());
						place_param_targets[0]->place << 
							fmt("because it does not appear in target %s", place_param_targets[0]->format_err());
						throw ERROR_LOGICAL;
					}
				}

				if (iter == tokens.end()) {
					place_end << fmt("expected %s", char_format_err(';'));
					name_copy->get_place() << 
						fmt("after copy dependency %s",
						    name_copy->format_err()); 
					throw ERROR_LOGICAL; 
				}
				if (! is_operator(';')) {
					(*iter)->get_place() <<
						fmt("expected %s", char_format_err(';'));
					name_copy->place << 
						fmt("after copy dependency %s",
						    name_copy->format_err()); 
					throw ERROR_LOGICAL; 
				}
				++iter;

				if (! place_output.empty()) {
					place_output << 
						fmt("output redirection using %s must not be used",
						     char_format_err('>'));
					place_equal << 
						fmt("in copy rule using %s for target %s", 
						    char_format_err('='),
						    place_param_targets[0]->format_err()); 
					throw ERROR_LOGICAL;
				}

				/* Check that there is just a single
				 * target */
				if (place_param_targets.size() != 1) {
					place_equal << fmt("there must not be a copy rule using %s", char_format_err('=')); 
					place_param_targets[0]->place << 
						fmt("for multiple targets %s...",
						    place_param_targets[0]->format_err()); 
					throw ERROR_LOGICAL; 
				}

				if (place_param_targets[0]->type != Type::FILE) {
					assert(place_param_targets[0]->type == Type::TRANSIENT); 
					place_equal << fmt("copy rule using %s cannot be used", char_format_err('='));
					place_param_targets[0]->place 
						<< fmt("with transient target %s", place_param_targets[0]->format_err()); 
					throw ERROR_LOGICAL;
				}

				assert(place_param_targets.size() == 1); 

				/* Append target name when source ends
				 * in slash */
				append_copy(*name_copy, place_param_targets[0]->place_param_name); 

				return make_shared <Rule> (place_param_targets[0], name_copy,
							   place_flag_exclam);

			} else if (is_operator('?')) {
				(*iter)->get_place() 
					<< fmt("optional dependency using %s must not be used",
						char_format_err('?'));
				place_equal << 
					fmt("in copy rule using %s for target %s", 
					    char_format_err('='),
					    place_param_targets[0]->format_err()); 
				throw ERROR_LOGICAL;
			} else if (is_operator('&')) {
				(*iter)->get_place() 
					<< fmt("trivial dependency using %s must not be used",
						char_format_err('&')); 
				place_equal << 
					fmt("in copy rule using %s for target %s", 
					    char_format_err('='),
					    place_param_targets[0]->format_err()); 
				throw ERROR_LOGICAL;
			} else {
				(*iter)->get_place() << 
					fmt("expected a filename or %s, not %s", 
					    char_format_err('{'),
					    (*iter)->format_start_err()); 
				place_equal << fmt("after %s", char_format_err('='));
				throw ERROR_LOGICAL;
			}
		}
		
	} else if (is_operator(';')) {
		place_nocommand= (*iter)->get_place(); 
		++iter;
	} else {
		(*iter)->get_place() <<
			(had_colon
			 ? fmt("expected a dependency, a command, or %s", char_format_err(';'))
			 : fmt("expected a command, %s, %s, or %s",
			       char_format_err(':'),
			       char_format_err(';'),
			       char_format_err('=')));
		place_param_targets[0]->place <<
			fmt("for target %s", 
			    place_param_targets[0]->format_err());
		throw ERROR_LOGICAL;
	}

	/* Cases where output redirection is not possible */ 
	if (! place_output.empty()) {
		/* Already checked before */ 
		assert(place_param_targets[redirect_index]->type == Type::FILE); 

		if (command == nullptr) {
			place_output << 
				fmt("output redirection using %s must not be used",
				     char_format_err('>'));
			place_nocommand <<
				fmt("in rule for %s without a command",
				    place_param_targets[0]->format_err());
			throw ERROR_LOGICAL;
		}

		if (command != nullptr && is_hardcode) {
			place_output <<
				fmt("output redirection using %s must not be used",
				     char_format_err('>'));
			place_equal <<
				fmt("in rule for %s with assigned content using %s",
				    place_param_targets[0]->format_err(),
				    char_format_err('=')); 
			throw ERROR_LOGICAL;
		}
	}

	/* Cases where input redirection is not possible */ 
	if (! filename_input.empty()) {
		if (command == nullptr) {
			place_input <<
				fmt("input redirection using %s must not be used",
				     char_format_err('<'));
			place_nocommand <<
				fmt("in rule for %s without a command",
				    place_param_targets[0]->format_err()); 
			throw ERROR_LOGICAL;
		} else {
			assert(! is_hardcode); 
		}
	}

	return make_shared <Rule> 
		(move(place_param_targets), 
		 dependencies, 
		 command, is_hardcode, 
		 redirect_index,
		 filename_input);
}

bool Build::build_expression_list(vector <shared_ptr <Dependency> > &ret, 
				  Place_Param_Name &place_param_name_input,
				  Place &place_input,
				  const vector <shared_ptr <Place_Param_Target> > &targets)
{
	assert(ret.size() == 0);

	while (iter != tokens.end()) {
		vector <shared_ptr <Dependency> > ret_new; 
		bool r= build_expression(ret_new, place_param_name_input, place_input, targets);
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
			     Place &place_input,
			     const vector <shared_ptr <Place_Param_Target> > &targets)
{
	assert(ret.size() == 0); 

	/* '(' expression* ')' */ 
	if (is_operator('(')) {
		Place place_paren= (*iter)->get_place();
		++iter;
		vector <shared_ptr <Dependency> > r;
		while ( build_expression_list(r, place_param_name_input, place_input, targets)) {
			ret.insert(ret.end(), r.begin(), r.end()); 
			r.clear(); 
		}
		if (iter == tokens.end()) {
			place_end << fmt("expected %s", char_format_err(')'));
			place_paren << fmt("for group started by %s", 
					    char_format_err('(')); 
			throw ERROR_LOGICAL;
		}
		if (! is_operator(')')) {
			(*iter)->get_place() << fmt("expected %s", char_format_err(')'));
			place_paren << fmt("for group started by %s",
					    char_format_err('(')); 
			throw ERROR_LOGICAL;
		}
		++ iter; 
		return true; 
	} 

	/* '[' expression* ']' */
	if (is_operator('[')) {
		Place place_bracket= (*iter)->get_place(); 
		++iter;	
		vector <shared_ptr <Dependency> > r2;
		vector <shared_ptr <Dependency> > r;
		while (build_expression_list(r, place_param_name_input, place_input, targets)) {
			r2.insert(r2.end(), r.begin(), r.end()); 
			r.clear(); 
		}
		if (iter == tokens.end()) {
			place_end << fmt("expected %s", char_format_err(']'));
			place_bracket << fmt("for group started by %s", 
					      char_format_err('[')); 
			throw ERROR_LOGICAL;
		}
		if (! is_operator(']')) {
			(*iter)->get_place() << fmt("expected %s", char_format_err(']'));
			place_bracket << fmt("for group started by %s",
					      char_format_err('[')); 
			throw ERROR_LOGICAL;
		}
		++ iter; 
		for (auto &j:  r2) {
			
			/* Variable dependency cannot appear within
			 * dynamic dependency */ 
			if (j->has_flags(F_VARIABLE)) {
				string text= dynamic_pointer_cast <Direct_Dependency> (j)
					->place_param_target.format_mid();
				j->get_place() <<
					fmt("variable dependency %s$[%s]%s must not appear", 
					    Color::beg_name_bare, text, Color::end_name_bare);
				place_bracket <<
					fmt("within dynamic dependency started by %s",
					     char_format_err('[')); 
				throw ERROR_LOGICAL; 
			}

			shared_ptr <Dependency> dependency_new= 
				make_shared <Dynamic_Dependency> (0, j);
			ret.push_back(dependency_new);
		}
		return true; 
	} 

	/* '!' single_expression */ 
 	if (is_operator('!')) {
 		Place place_exclam= (*iter)->get_place();
 		++iter; 
		if (! build_expression(ret, place_param_name_input, place_input, targets)) {
			if (iter == tokens.end()) 
				place_end << "expected a dependency";
			else
				(*iter)->get_place() << "expected a dependency";
			place_exclam << fmt("after %s",
					     char_format_err('!')); 
			throw ERROR_LOGICAL;
		}
		for (auto &j:  ret) {
			j->add_flags(F_EXISTENCE);
			j->set_place_existence(place_exclam); 
		}
		return true;
	}

	/* '?' single_expression */ 
 	if (is_operator('?')) {
 		Place place_question= (*iter)->get_place();
 		++iter; 

		if (! build_expression(ret, place_param_name_input, place_input, targets)) {
			if (iter == tokens.end()) {
				place_end << "expected a dependency";
				place_question << fmt("after %s",
						       char_format_err('?')); 
				throw ERROR_LOGICAL;
			} else {
				(*iter)->get_place() << "expected a dependency";
				place_question << fmt("after %s",
						       char_format_err('?')); 
				throw ERROR_LOGICAL;
			}
		}
		if (! option_nonoptional) {
			/* D_INPUT and D_OPTIONAL cannot be used at the same
			 * time. Note: Input redirection must not appear in
			 * dynamic dependencies, and therefore it is sufficient
			 * to check this here.     */   
			if (! place_param_name_input.place.empty()) { 
				place_input <<
					fmt("input redirection using %s must not be used",
					     char_format_err('<')); 
				place_question <<
					fmt("in conjunction with optional dependencies using %s",
					     char_format_err('?')); 
				throw ERROR_LOGICAL;
			}
				
			for (auto &j:  ret) {
				j->add_flags(F_OPTIONAL); 
				j->set_place_optional(place_question); 
			}
		}
		return true;
	}

	/* '&' single_expression */ 
	if (is_operator('&')) {
		Place place_ampersand= (*iter)->get_place(); 
		++iter;
		if (! build_expression(ret, place_param_name_input, place_input, targets)) {
			if (iter == tokens.end()) {
				place_end << "expected a dependency";
				place_ampersand << fmt("after %s",
						       char_format_err('&')); 
				throw ERROR_LOGICAL;
			} else {
				(*iter)->get_place() << "expected a dependency";
				place_ampersand << fmt("after %s",
							char_format_err('&'));
				throw ERROR_LOGICAL;
			}
		}
		for (auto &j:  ret) {
			if (! option_nontrivial)
				j->add_flags(F_TRIVIAL); 
			j->set_place_trivial(place_ampersand); 
		}
		return true;
	}
		
	/* '$' ; variable dependency */ 
	shared_ptr <Dependency> dependency= 
		build_variable_dependency(place_param_name_input, place_input, targets);
	if (dependency != nullptr) {
		ret.push_back(dependency); 
		return true; 
	}

	shared_ptr <Dependency> r= 
		build_redirect_dependency(place_param_name_input, place_input, targets); 
	if (r != nullptr) {
		ret.push_back(r);
		return true; 
	}

	return false;
}

shared_ptr <Dependency> Build
::build_variable_dependency(Place_Param_Name &place_param_name_input, 
			    Place &place_input,
			    const vector <shared_ptr <Place_Param_Target> > &targets)
{
	(void) targets;
	
	bool has_input= false;

	shared_ptr <Dependency> ret;

	if (! is_operator('$')) 
		return nullptr;

	Place place_dollar= (*iter)->get_place();
	++iter;

	if (iter == tokens.end()) {
		place_end << fmt("expected %s", char_format_err('['));
		place_dollar << fmt("after %s", 
				     char_format_err('$')); 
		throw ERROR_LOGICAL;
	}
	
	if (! is_operator('[')) 
		return nullptr;

	++iter;

	Flags flags= F_VARIABLE;

	/* Flags */ 
	Place place_flag_last;
	char flag_last= 'E';
	while (is_operator('!') || is_operator('&') || is_operator('?')) {

		flag_last= is <Operator> ()->op; 
		if (is_operator('!')) {
			place_flag_last= (*iter)->get_place();
			flags |= F_EXISTENCE; 
		} else if (is_operator('?')) {
			if (! option_nonoptional) {
				(*iter)->get_place() << 
					fmt("optional dependency using %s must not appear",
					     char_format_err('?')); 
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
	if (! is <Name_Token> ()) {
		(*iter)->get_place() << "expected a filename";
		if (has_input)
			place_input << fmt("after %s", 
					    char_format_err('<')); 
		else if (! place_flag_last.empty()) 
			place_flag_last << fmt("after %s", 
					       char_format_err(flag_last)); 
		else
			place_dollar << frmt("after %s$[%s",
					     Color::beg_name_quoted, Color::end_name_quoted);

		throw ERROR_LOGICAL;
	}
	shared_ptr <Place_Param_Name> place_param_name= 
		is <Name_Token> ();
	++iter;

	if (has_input && ! place_param_name_input.empty()) {
		place_param_name->place << 
			fmt("duplicate input redirection %s<%s%s", 
			    Color::beg_name_bare, place_param_name->format_mid(), Color::end_name_bare);
		place_param_name_input.place << 
			fmt("shadows previous input redirection %s<%s%s", 
			    Color::beg_name_bare, place_param_name_input.format_mid(), Color::end_name_bare); 
		if (targets.size() == 1) {
			targets.front()->place <<
				fmt("for target %s", targets.front()->format_err()); 
		} else if (targets.size() > 1) {
			targets.front()->place <<
				fmt("for targets %s...", targets.front()->format_err()); 
		}
		throw ERROR_LOGICAL;
	}

	/* Check that the name does not contain '=' */ 
	for (auto &j:  place_param_name->get_texts()) {
		if (j.find('=') != string::npos) {
			place_param_name->place <<
				fmt("name of variable dependency must not contain %s",
				     char_format_err('=')); 
			throw ERROR_LOGICAL;
		}
	}


	if (iter == tokens.end()) {
		place_end << fmt("expected %s", char_format_err(']'));
		place_dollar << frmt("after opening %s$[%s",
				     Color::beg_name_quoted, Color::end_name_quoted);
		throw ERROR_LOGICAL;
	}

	string variable_name= "";
	
	/* Explicit variable name */ 
	if (is_operator('=')) {
		Place place_equal= (*iter)->get_place();
		++iter;
		if (iter == tokens.end()) {
			place_end << "expected a filename";
			place_equal << fmt("after %s in variable dependency",
					    char_format_err('=')); 
			throw ERROR_LOGICAL;
		}
		if (! is <Name_Token> ()) {
			(*iter)->get_place() << 
				fmt("expected a filename, not %s",
				    (*iter)->format_start_err());
			place_equal << fmt("after %s in variable dependency",
					    char_format_err('=')); 
			throw ERROR_LOGICAL;
		}

		if (place_param_name->get_n() != 0) {
			place_param_name->place << 
				fmt("variable name %s must be unparametrized", 
				    place_param_name->format_err());
			throw ERROR_LOGICAL; 
		}

		variable_name= place_param_name->unparametrized();

		place_param_name= is <Name_Token> ();
		++iter; 
	}

	/* Closing ']' */ 
	if (! is_operator(']')) {
		(*iter)->get_place() << fmt("expected %s", char_format_err(']'));
		place_dollar << frmt("after opening %s$[%s",
				     Color::beg_name_quoted, Color::end_name_quoted); 
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
	return make_shared <Direct_Dependency> 
		(flags, 
		 Place_Param_Target(Type::FILE, *place_param_name, place_dollar), 
		 variable_name);
}

shared_ptr <Dependency> Build::build_redirect_dependency
(Place_Param_Name &place_param_name_input,
 Place &place_input,
 const vector <shared_ptr <Place_Param_Target> > &targets)
{
	(void) targets;

	bool has_input= false;

	if (is_operator('<')) {
		place_input= (*iter)->get_place();

		++iter;
		has_input= true; 
	}

	bool has_transient= false;
	Place place_at; 
	if (is_operator('@')) {
		place_at= (*iter)->get_place();
		if (has_input) {
			place_at << "expected a filename";
			place_input << fmt("after %s",
					    char_format_err('<')); 
			throw ERROR_LOGICAL;
		}
		++ iter;
		has_transient= true; 
	}

	if (iter == tokens.end()) {
		if (has_input) {
			place_end << "expected a filename";
			place_input << fmt("after %s",
					    char_format_err('<')); 
			throw ERROR_LOGICAL;
		} else if (has_transient) {
			place_end << "expected the name of a transient target";
			place_at << fmt("after %s",
					 char_format_err('@')); 
			throw ERROR_LOGICAL; 
		} else {
			return nullptr;
		}
	}

	if (nullptr == is <Name_Token> ()) {
		if (has_input) {
			(*iter)->get_place() << "expected a filename";
			place_input << fmt("after %s",
					    char_format_err('<')); 
			throw ERROR_LOGICAL;
		} else if (has_transient) {
			(*iter)->get_place() << "expected the name of a transient target";
			place_at << fmt("after %s",
					 char_format_err('@')); 
			throw ERROR_LOGICAL;
		} else {
			return nullptr;
		}
	}

	shared_ptr <Name_Token> name_token= is <Name_Token> ();
	++iter; 

	if (has_input && ! place_param_name_input.empty()) {
		name_token->place << 
			fmt("duplicate input redirection %s<%s%s", 
			    Color::beg_name_bare, name_token->format_mid(), Color::end_name_bare);
		place_param_name_input.place << 
			fmt("shadows previous input redirection %s<%s%s", 
			    Color::beg_name_bare, place_param_name_input.format_mid(), Color::end_name_bare); 
		if (targets.size() == 1) {
			targets.front()->place <<
				fmt("for target %s", targets.front()->format_err()); 
		} else if (targets.size() > 1) {
			targets.front()->place <<
				fmt("for targets %s...", targets.front()->format_err()); 
		}
		throw ERROR_LOGICAL;
	}

	Flags flags= 0;
	if (has_input) {
		assert(place_param_name_input.empty()); 
		place_param_name_input= *name_token;
	}

	return make_shared <Direct_Dependency>
		(flags,
		 Place_Param_Target(has_transient ? Type::TRANSIENT : Type::FILE,
				    *name_token,
				    has_transient ? place_at : name_token->place)); 
}

void Build::append_copy(      Param_Name &to,
			const Param_Name &from) 
{
	/* Only append if TO ends in a slash */
	if (! (to.last_text().size() != 0 &&
	       to.last_text().back() == '/')) {
		return;
	}

	for (int i= from.get_n();  i >= 0;  --i) {
		for (int j= from.get_texts()[i].size() - 1;
		     j >= 0;  --j) {
			if (from.get_texts()[i][j] == '/') {

				/* Don't append the found slash, as TO
				 * already ends in a slash */ 
				to.append_text(from.get_texts()[i].substr(j + 1));

				for (unsigned k= i;  k < from.get_n();  ++k) {
					to.append_parameter(from.get_parameters()[k]);
					to.append_text(from.get_texts()[k + 1]);
				}
				return;
			}
		}
	} 

	/* FROM does not contain slashes;
	 * prepend the whole FROM to TO */
	to.append(from);
}

void Build::get_rule_list(vector <shared_ptr <Rule> > &rules,
			  vector <shared_ptr <Token> > &tokens,
			  const Place &place_end)
{
	auto iter= tokens.begin(); 

	Build build(tokens, iter, place_end);

	build.build_rule_list(rules); 

	if (iter != tokens.end()) {
		(*iter)->get_place_start() 
			<< fmt("expected a rule, not %s", 
			       (*iter)->format_start_err()); 
		throw ERROR_LOGICAL;
	}
}

void Build::get_expression_list(vector <shared_ptr <Dependency> > &dependencies,
				vector <shared_ptr <Token> > &tokens,
				const Place &place_end,
				Place_Param_Name &input,
				Place place_input)
{
	auto iter= tokens.begin(); 
	Build build(tokens, iter, place_end); 
	vector <shared_ptr <Place_Param_Target>> targets;
	build.build_expression_list(dependencies, input, place_input, targets); 
	if (iter != tokens.end()) {
		(*iter)->get_place_start() 
			<< fmt("expected a dependency, not %s",
			       (*iter)->format_start_err());
		throw ERROR_LOGICAL;
	}
}

shared_ptr <Dependency> Build::get_target_dependency(string text)
{
	Place place(Place::Type::ARGV, text);

	if (text.empty()) {
		place << "name must not be empty";
		throw ERROR_LOGICAL;
	}

	const char *begin= text.c_str();
	const char *p= text.c_str() + text.size();
	int closing= 0;
	while (p != begin && p[-1] == ']') {
		++closing;
		--p;
	}
	const char *end_name= p;

	const char *q= begin;
	while (q != end_name && 
	       (*q == '[' || *q == '!' || *q == '?')) {
		++q;
	}

	assert(q <= end_name); 

	Type type= Type::FILE;
	const char *begin_name= q;
	if (begin_name != end_name && *q == '@') {
		type= Type::TRANSIENT;
		++ begin_name;
	}

	if (begin_name == end_name) {
		place << "name must not be empty";
		throw ERROR_LOGICAL; 
	}

	shared_ptr <Dependency> ret= make_shared <Direct_Dependency> 
		(0, Place_Param_Target
		 (type, 
		  Place_Param_Name
		  (string(begin_name, end_name - begin_name), 
		   place)));

	while (q != begin) {
		if (q[-1] == '!') {
			ret->add_flags(F_EXISTENCE); 
			ret->set_place_existence(place);
		} else if (q[-1] == '?') {
			ret->add_flags(F_OPTIONAL); 
			ret->set_place_optional(place); 
		} else if (q[-1] == '[') {
			ret= make_shared <Dynamic_Dependency> (0, ret);
			-- closing;
		} else {
			assert(false); 
			/* Ignore character */ 
		}
		--q;
	}

	assert(q == begin);
	
	if (closing != 0) {
		place << frmt("unbalanced brackets %s[]%s", 
			      Color::beg_name_quoted, Color::end_name_quoted);
		throw ERROR_LOGICAL;
	}

	return ret; 
}

#endif /* ! BUILD_HH */
