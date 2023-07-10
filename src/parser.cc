#include "parser.hh"

#include "explain.hh"
#include "tokenizer.hh"

shared_ptr <Rule> Parser::parse_rule(shared_ptr <const Place_Target> &target_first)
{
	const auto iter_begin= iter;
	/* Used to check that when this function fails (i.e., returns
	 * null), is has not read any tokens.  */

	Place place_output;
	/* An empty place when output is not redirected */

	int redirect_index= -1;
	/* Index of the target that has the output, or -1 */

	std::vector <shared_ptr <const Place_Target> > place_targets;

	while (iter != tokens.end()) {
		Place place_output_new;
		/* Remains an empty place when '>' is not present */

		if (is_operator('>')) {
			place_output_new= (*iter)->get_place();
			++iter;
		}

		Flags flags_type= 0;
		/* F_TARGET_TRANSIENT is set when '@' is found */

		Place place_of_target;
		if (iter != tokens.end())
			place_of_target= (*iter)->get_place_start();

		if (is_operator('@')) {
			Place place_at= (*iter)->get_place();
			++iter;

			if (iter == tokens.end()) {
				place_end << "expected the name of transient target";
				place_at << fmt("after %s", show_operator('@'));
				throw ERROR_LOGICAL;
			}
			if (! is <Name_Token> ()) {
				(*iter)->get_place_start()
					<< fmt("expected the name of transient target, not %s",
					       show(*iter));
				place_at << fmt("after %s", show_operator('@'));
				throw ERROR_LOGICAL;
			}
			flags_type= F_TARGET_TRANSIENT;
		}

		if (! is <Name_Token> ()) {
			if (! place_output_new.empty()) {
				if (iter == tokens.end())
					place_end << "expected a filename";
				else
					(*iter)->get_place_start() <<
						fmt("expected a filename, not %s",
						    show((*iter)));
				place_output_new <<
					fmt("after output redirection using %s",
					    show_operator('>'));
				throw ERROR_LOGICAL;
			}
			break;
		}

		/* Target */
		shared_ptr <Name_Token> target_name= is <Name_Token> ();
		++iter;

		string param_1, param_2;
		if (! target_name->valid(param_1, param_2)) {
			place_of_target <<
				fmt("the two parameters %s and %s in the name %s "
				    "must be separated by at least one character",
				    show_prefix("$", param_1),
				    show_prefix("$", param_2),
				    show(target_name));
			explain_separated_parameters();
			throw ERROR_LOGICAL;
		}

		string parameter_duplicate;
		if ((parameter_duplicate= target_name->get_duplicate_parameter()) != "") {
			place_of_target <<
				fmt("target %s must not contain duplicate parameter %s",
				    show(target_name),
				    show_prefix("$", parameter_duplicate));
			throw ERROR_LOGICAL;
		}

		shared_ptr <const Place_Target> place_target=
			std::make_shared <Place_Target>
			(flags_type, *target_name, place_of_target);

		if (! place_output_new.empty()) {
			if (! place_output.empty()) {
				place_output_new <<
					fmt("there must not be a second output redirection %s",
					    show_prefix(">", *place_target));
				assert(place_targets[redirect_index]
				       ->place_name.get_n() == 0);
				assert((place_targets[redirect_index]->flags
					& F_TARGET_TRANSIENT) == 0);
				place_output <<
					fmt("shadowing previous output redirection %s",
					    show_prefix
					    (">", place_targets[redirect_index]
					     ->unparametrized().get_name_nondynamic()));
				throw ERROR_LOGICAL;
			}
			place_output= place_output_new;
			assert(! place_output.empty());
			redirect_index= place_targets.size();
		}

		if (flags_type == F_TARGET_TRANSIENT) {
			if (! place_output_new.empty()) {
				place_target->place <<
					fmt("transient target %s is invalid",
					    show(*place_target));
				place_output_new << fmt("after output redirection using %s",
							show_operator('>'));
				throw ERROR_LOGICAL;
			}
		}
		if (target_first == nullptr) {
			target_first= place_target;
		}

		place_targets.push_back(place_target);
	}

	if (place_targets.size() == 0) {
		assert(iter == iter_begin);
		return nullptr;
	}

	/* Check that all targets have the same set of parameters */
	std::set <string> parameters_0;
	for (const string &parameter: place_targets[0]->place_name.get_parameters()) {
		parameters_0.insert(parameter);
	}
	assert(place_targets.size() >= 1);
	for (size_t i= 1; i < place_targets.size(); ++i) {
		std::set <string> parameters_i;
		for (const string &parameter:
			     place_targets[i]->place_name.get_parameters()) {
			parameters_i.insert(parameter);
		}
		if (parameters_i != parameters_0) {
			place_targets[i]->place <<
				fmt("parameters of target %s differ",
				    show(*place_targets[i]));
			place_targets[0]->place <<
				fmt("from parameters of target %s in rule with multiple targets",
				    show(*place_targets[0]));
			throw ERROR_LOGICAL;
		}
	}

	if (iter == tokens.end()) {
		place_end <<
			fmt("expected a command, %s, %s, or %s",
			    show_operator(':'), show_operator(';'), show_operator('='));
		place_targets.back()->place
			<< fmt("after target %s",
			       show(*place_targets.back()));
		throw ERROR_LOGICAL;
	}

	std::vector <shared_ptr <const Dep> > deps;
	bool had_colon= false;

	/* Empty at first */
	Place_Name filename_input;
	Place place_input;

	if (is_operator(':')) {
		had_colon= true;
		++iter;
		parse_expression_list(deps, filename_input,
				      place_input, place_targets);
	}

	/* Command */
	if (iter == tokens.end()) {
		assert(had_colon);
		place_end << fmt("expected a dependency, a command, or %s",
				 show_operator(';'));
		place_targets[0]->place
			<< fmt("for target %s", show(*place_targets[0]));
		throw ERROR_LOGICAL;
	}

	shared_ptr <Command> command;
	/* Remains null when there is no command */

	bool is_hardcode;
	/* When command is not null, whether the command is a command or
	 * hardcoded content */

	Place place_nocommand; /* Place of ';' */
	Place place_equal;
	shared_ptr <Name_Token> name_copy; /* Name of the copy-from file */

	if ((command= is <Command> ())) {
		++iter;
		is_hardcode= false;
	} else if (! had_colon && is_operator('=')) {
		place_equal= (*iter)->get_place();
		++iter;

		if (iter == tokens.end()) {
			place_end << fmt("expected a filename, a flag, or %s",
					 show_operator('{'));
			place_equal << fmt("after %s",
					   show_operator('='));
			throw ERROR_LOGICAL;
		}

		if ((command= is <Command> ())) {
			/* Hardcoded content */
			++iter;
			assert(place_targets.size() != 0);
			if (place_targets.size() != 1) {
				place_equal <<
					fmt("there must not be assigned content using %s",
					    show_operator('='));
				place_targets[0]->place <<
					fmt("in rule for %s... with multiple targets",
					    show(*place_targets[0]));
				throw ERROR_LOGICAL;
			}
			if ((place_targets[0]->flags & F_TARGET_TRANSIENT)) {
				place_equal <<
					fmt("there must not be assigned content using %s",
					    show_operator('='));
				place_targets[0]->place <<
					fmt("for transient target %s",
					    show(*place_targets[0]));
				throw ERROR_LOGICAL;
			}
			/* No redirected output is checked later */
			is_hardcode= true;
		} else {
			Place place_flag_persistent;
			Place place_flag_optional;

			while (is <Flag_Token> ()) {
				shared_ptr <Flag_Token> flag= is <Flag_Token> ();
				if (flag->flag == 'p') {
					place_flag_persistent= flag->get_place();
					++iter;
				} else if (flag->flag == 'o') {
					if (! option_g)
						place_flag_optional= flag->get_place();
					++iter;
				} else {
					flag->get_place()
						<< fmt("flag %s must not be used",
						       show_prefix("-", frmt("%c", flag->flag)));
					place_equal <<
						fmt("in copy rule using %s for target %s",
						    show_operator('='),
						    show(*place_targets[0]));
					throw ERROR_LOGICAL;
				}
			}

			if (! is <Name_Token> ()) {
				if (iter == tokens.end()) {
					(*iter)->get_place_start() <<
						fmt("expected a filename, a flag, or %s",
						    show_operator('{'));
				} else {
					(*iter)->get_place_start() <<
						fmt("expected a filename, a flag, or %s, not %s",
						    show_operator('{'),
						    show(*iter));
				}
				place_equal << fmt("after %s", show_operator('='));
				throw ERROR_LOGICAL;
			}

			/* Copy rule */
			name_copy= is <Name_Token> ();
			++iter;

			/* Check that the source file contains
			 * only parameters that also appear in
			 * the target  */
			std::set <string> parameters;
			for (auto &parameter:
				     place_targets[0]->place_name.get_parameters()) {
				parameters.insert(parameter);
			}
			for (size_t jj= 0; jj < name_copy->get_n(); ++jj) {
				string parameter=
					name_copy->get_parameters()[jj];
				if (parameters.count(parameter) == 0) {
					name_copy->places[jj] <<
						fmt("parameter %s must not appear in copied file %s",
						    show_prefix("$", parameter),
						    show(name_copy));
					place_targets[0]->place <<
						fmt("because it does not appear in target %s",
						    show(*place_targets[0]));
					throw ERROR_LOGICAL;
				}
			}

			if (iter == tokens.end()) {
				place_end << fmt("expected %s", show_operator(';'));
				name_copy->get_place() <<
					fmt("after copy dependency %s",
					    show(name_copy));
				throw ERROR_LOGICAL;
			}
			if (! is_operator(';')) {
				(*iter)->get_place() <<
					fmt("expected %s", show_operator(';'));
				name_copy->place <<
					fmt("after copy dependency %s",
					    show(name_copy));
				throw ERROR_LOGICAL;
			}
			++iter;

			if (! place_output.empty()) {
				place_output <<
					fmt("output redirection using %s must not be used",
					    show_operator('>'));
				place_equal <<
					fmt("in copy rule using %s for target %s",
					    show_operator('='),
					    show(*place_targets[0]));
				throw ERROR_LOGICAL;
			}

			/* Check that there is just a single
			 * target */
			if (place_targets.size() != 1) {
				place_equal <<
					fmt("there must not be a copy rule using %s",
					    show_operator('='));
				place_targets[0]->place <<
					fmt("for multiple targets %s...",
					    show(*place_targets[0]));
				throw ERROR_LOGICAL;
			}

			/* Check that target is not transient */
			if (place_targets[0]->flags & F_TARGET_TRANSIENT) {
				assert(place_targets[0]->flags & F_TARGET_TRANSIENT);
				place_equal << fmt("copy rule using %s cannot be used",
						   show_operator('='));
				place_targets[0]->place
					<< fmt("with transient target %s",
					       show(*place_targets[0]));
				throw ERROR_LOGICAL;
			}

			assert(place_targets.size() == 1);

			/* Append target name when source ends
			 * in slash */
			append_copy(*name_copy, place_targets[0]->place_name);

			return std::make_shared <Rule> (place_targets[0], name_copy,
						   place_flag_persistent,
						   place_flag_optional);
		}
	} else if (is_operator(';')) {
		place_nocommand= (*iter)->get_place();
		++iter;
	} else {
		(*iter)->get_place() <<
			(had_colon
			 ? fmt("expected a dependency, a command, or %s, not %s",
			       show_operator(';'),
			       show(*iter))
			 : fmt("expected a command, %s, %s, or %s, not %s",
			       show_operator(':'),
			       show_operator(';'),
			       show_operator('='),
			       show(*iter)));
		place_targets[0]->place <<
			fmt("for target %s",
			    show(*place_targets[0]));
		throw ERROR_LOGICAL;
	}

	/* Cases where output redirection is not possible */
	if (! place_output.empty()) {
		/* Already checked before */
		assert((place_targets[redirect_index]->flags & F_TARGET_TRANSIENT) == 0);

		if (command == nullptr) {
			place_output <<
				fmt("output redirection using %s must not be used",
				    show_operator('>'));
			place_nocommand <<
				fmt("in rule for %s without a command",
				    show(*place_targets[0]));
			throw ERROR_LOGICAL;
		}

		if (command != nullptr && is_hardcode) {
			place_output <<
				fmt("output redirection using %s must not be used",
				    show_operator('>'));
			place_equal <<
				fmt("in rule for %s with assigned content using %s",
				    show(*place_targets[0]),
				    show_operator('='));
			throw ERROR_LOGICAL;
		}
	}

	/* Cases where input redirection is not possible */
	if (! filename_input.empty()) {
		if (command == nullptr) {
			place_input <<
				fmt("input redirection using %s must not be used",
				    show_operator('<'));
			place_nocommand <<
				fmt("in rule for %s without a command",
				    show(*place_targets[0]));
			throw ERROR_LOGICAL;
		} else {
			assert(! is_hardcode);
		}
	}

	return std::make_shared <Rule>
		(move(place_targets), deps, command, is_hardcode,
		 redirect_index, filename_input);
}

bool Parser::parse_expression_list
(std::vector <shared_ptr <const Dep> > &ret,
 Place_Name &place_name_input,
 Place &place_input,
 const std::vector <shared_ptr <const Place_Target> > &targets)
{
	assert(ret.size() == 0);

	while (iter != tokens.end()) {
		shared_ptr <const Dep> ret_new;
		bool r= parse_expression(ret_new, place_name_input,
					 place_input, targets);
		if (!r) {
			assert(ret_new == nullptr);
			return ! ret.empty();
		}
		assert(ret_new != nullptr);
		ret.push_back(ret_new);
	}

	return ! ret.empty();
}

bool Parser::parse_expression(shared_ptr <const Dep> &ret,
			      Place_Name &place_name_input,
			      Place &place_input,
			      const std::vector <shared_ptr <const Place_Target> > &targets)
{
	assert(ret == nullptr);

	/* '(' expression* ')' */
	if (is_operator('(')) {
		Place place_paren= (*iter)->get_place();
		++iter;
		std::vector <shared_ptr <const Dep> > r;
		if (parse_expression_list(r, place_name_input, place_input, targets)) {
			assert(r.size() >= 1);
			if (r.size() > 1) {
				ret= std::make_shared <Compound_Dep> (std::move(r), place_paren);
			} else {
				ret= std::move(r.at(0));
			}
			r.clear();
		}
		if (iter == tokens.end()) {
			place_end << fmt("expected %s", show_operator(')'));
			place_paren << fmt("after opening %s", show_operator('('));
			throw ERROR_LOGICAL;
		}
		if (! is_operator(')')) {
			(*iter)->get_place_start() <<
				fmt("expected %s, not %s",
				    show_operator(')'), show(*iter));
			place_paren << fmt("after opening %s", show_operator('('));
			throw ERROR_LOGICAL;
		}
		++iter;

		/* If RET is null, it means we had empty parentheses.
		 * Return an empty Compound_Dependency in that case  */
		if (ret == nullptr)
			ret= std::make_shared <Compound_Dep> (place_paren);

		if (next_concatenates()) {
			shared_ptr <const Dep> next;
			bool rr= parse_expression(next, place_name_input, place_input, targets);
			/* It can be that an empty list was parsed, in
			 * which case RR is true but the list is empty */
			if (rr && next != nullptr) {
				shared_ptr <Concat_Dep> ret_new= std::make_shared <Concat_Dep> ();
				ret_new->push_back(ret);
				ret_new->push_back(next);
				ret.reset();
				ret= move(ret_new);
			}
		}

		return true;
	}

	/* '[' expression* ']' */
	if (is_operator('[')) {
		Place place_bracket= (*iter)->get_place();
		++iter;
		std::vector <shared_ptr <const Dep> > r2;
		parse_expression_list(r2, place_name_input, place_input, targets);

		if (iter == tokens.end()) {
			place_end << fmt("expected %s", show_operator(']'));
			place_bracket << fmt("after opening %s", show_operator('['));
			throw ERROR_LOGICAL;
		}
		if (! is_operator(']')) {
			(*iter)->get_place_start() <<
				fmt("expected %s, not %s",
				    show_operator(']'), show(*iter));
			place_bracket << fmt("after opening %s", show_operator('['));
			throw ERROR_LOGICAL;
		}
		++iter;
		shared_ptr <Compound_Dep> ret_nondynamic=
			std::make_shared <Compound_Dep> (place_bracket);
		for (auto &j: r2) {
			/* Variable dependency cannot appear within
			 * dynamic dependency */
			if (j->flags & F_VARIABLE) {
				j->get_place() <<
					fmt("variable dependency %s must not appear",
					    show(j));
				place_bracket <<
					fmt("within dynamic dependency started by %s",
					    show_operator('['));
				throw ERROR_LOGICAL;
			}

			ret_nondynamic->push_back(j);
		}
		ret= std::make_shared <Dynamic_Dep> (0, ret_nondynamic);

		if (next_concatenates()) {
			shared_ptr <const Dep> next;
			bool rr= parse_expression(next, place_name_input, place_input, targets);
			/* It can be that an empty list was parsed, in
			 * which case RR is true but the list is empty */
			if (rr && next != nullptr) {
				shared_ptr <Concat_Dep> ret_new= std::make_shared <Concat_Dep> ();
				ret_new->push_back(ret);
				ret_new->push_back(next);
				ret.reset();
				ret= move(ret_new);
			}
		}

		/* If RET is null, it means we had empty parentheses.
		 * Return an empty Compound_Dependency in that case. */
		if (ret == nullptr)
			ret= std::make_shared <Compound_Dep> (place_bracket);

		return true;
	}

	/* flag expression */
	if (is <Flag_Token> ()) {
		const Flag_Token &flag_token= *is <Flag_Token> ();
		const Place place_flag= (*iter)->get_place();
		const unsigned i_flag= flag_get_index(flag_token.flag);
		++iter;

		if (! parse_expression(ret, place_name_input, place_input, targets)) {
			if (iter == tokens.end()) {
				place_end << "expected a dependency";
			} else {
				(*iter)->get_place_start() <<
					fmt("expected a dependency, not %s",
					    show(*iter));
			}
			place_flag << fmt("after flag %s",
					  show_prefix("-", frmt("%c", flag_token.flag)));
			throw ERROR_LOGICAL;
		}

		/* A dependency cannot be an input dependency and
		 * optional at the same time.  Note: Input redirection
		 * must not appear in dynamic dependencies, and
		 * therefore it is sufficient to check this here.  */
		if (! place_name_input.place.empty() && flag_token.flag == 'o') {
			place_input <<
				fmt("input redirection using %s must not be used",
				    show_operator('<'));
			place_flag <<
				fmt("in conjunction with optional dependency flag %s",
				    show_prefix("-", "o"));
			throw ERROR_LOGICAL;
		}

		/* Add the flag */
		if (! ((i_flag == I_OPTIONAL && option_g) ||
		       (i_flag == I_TRIVIAL  && option_a))) {
			shared_ptr <Dep> ret_new= Dep::clone(ret);
			ret_new->flags |= (1 << i_flag);
			assert(i_flag < C_WORD);
			if (i_flag < C_PLACED)
				ret_new->set_place_flag(i_flag, place_flag);
			ret= ret_new;
		}

		return true;
	}

	/* '$' ; variable dependency */
	shared_ptr <const Dep> dep=
		parse_variable_dep(place_name_input, place_input, targets);
	if (dep != nullptr) {
		ret= dep;
		return true;
	}

	/* Redirect dependency */
	dep= parse_redirect_dep(place_name_input, place_input, targets);
	if (dep != nullptr) {
		ret= dep;
		return true;
	}

	return false;
}

shared_ptr <const Dep> Parser
::parse_variable_dep(Place_Name &place_name_input,
		     Place &place_input,
		     const std::vector <shared_ptr <const Place_Target> > &targets)
{
	bool has_input= false;
	shared_ptr <const Dep> ret;
	if (! is_operator('$'))
		return nullptr;
	const Place place_dollar= (*iter)->get_place();
	++iter;

	assert(iter != tokens.end());
	if (! is_operator('[')) {
		/* The '$' and '[' operators are only generated when they both
		 * appear in conjunction.  */
		should_not_happen();
		return nullptr;
	}
	++iter;

	/* Flags */
	Flags flags= F_VARIABLE;
	Place places_flags[C_PLACED];
	for (unsigned i= 0; i < C_PLACED; ++i)
		places_flags[i].clear();
	Place place_flag_last;
	shared_ptr <Flag_Token> flag_token_last;
	while (is_flag('p') || is_flag('o') || is_flag('t')) {
		flag_token_last= is <Flag_Token> ();
		place_flag_last= (*iter)->get_place();
		if (is_flag('p')) {
			flags |= F_PERSISTENT;
			places_flags[I_PERSISTENT]= place_flag_last;
		} else if (is_flag('o')) {
			/* If the nonoptional (-g) option is set, ignore the -o flag */
			if (! option_g) {
				(*iter)->get_place() <<
					fmt("optional dependency using %s must not appear",
					    show_prefix("-", "o"));
				place_dollar << "within dynamic variable declaration";
				throw ERROR_LOGICAL;
			}
		} else if (is_flag('t')) {
			if (! option_a) {
				flags |= F_TRIVIAL;
				places_flags[I_TRIVIAL]= place_flag_last;
			}
		} else {
			unreachable();
		}
		++iter;
	}

	/* Input redirection using '<' */
	if (is_operator('<')) {
		has_input= true;
		place_input= (*iter)->get_place();
		flags |= F_INPUT;
		++iter;
	}

	/* Name of variable dependency */
	if (! is <Name_Token> ()) {
		if (iter == tokens.end())
			place_end << "expected a filename";
		else
			(*iter)->get_place_start() <<
				fmt("expected a filename, not %s", show(*iter));
		if (has_input) {
			place_input << fmt("after %s", show_operator('<'));
		} else if (! place_flag_last.empty()) {
			assert(flag_token_last);
			place_flag_last << fmt("after %s", show(flag_token_last));
		} else {
			place_dollar << fmt("after %s", show_operator("$["));
		}
		throw ERROR_LOGICAL;
	}
	shared_ptr <Place_Name> place_name= is <Name_Token> ();
	++iter;

	/* Check that the name does not contain '=' */
	for (auto &j: place_name->get_texts()) {
		if (j.find('=') != string::npos) {
			place_name->place <<
				fmt("name of variable dependency %s must not contain %s",
				    show(*place_name), show_operator('='));
			explain_variable_equal();
			throw ERROR_LOGICAL;
		}
	}

	/* Explicit variable name */
	string variable_name= "";
	if (is_operator('=')) {
		Place place_equal= (*iter)->get_place();
		++iter;
		if (iter == tokens.end()) {
			place_end << "expected a filename";
			place_equal <<
				fmt("after %s in variable dependency %s",
				    show_operator('='), show(*place_name));
			throw ERROR_LOGICAL;
		}
		if (! is <Name_Token> ()) {
			(*iter)->get_place_start() <<
				fmt("expected a filename, not %s",
				    show(*iter));
			place_equal << fmt("after %s in variable dependency %s",
					   show_operator('='), show(*place_name));
			throw ERROR_LOGICAL;
		}

		if (place_name->get_n() != 0) {
			place_name->place <<
				fmt("variable name %s must be unparametrized",
				    show(*place_name));
			throw ERROR_LOGICAL;
		}

		variable_name= place_name->unparametrized();
		place_name= is <Name_Token> ();
		++iter;
	}

	/* Closing ']' */
	if (! is_operator(']')) {
		if (iter == tokens.end()) {
			place_end << fmt("expected %s", show_operator(']'));
			place_dollar << fmt("after opening %s", show_operator("$["));
		} else {
			(*iter)->get_place_start() <<
				fmt("expected %s, not %s",
				    show_operator(']'), show(*iter));
			place_dollar << fmt("after opening %s", show_operator("$["));
		}
		throw ERROR_LOGICAL;
	}
	++iter;

	/* The place of the variable dependency as a whole is set on the
	 * name contained in it.  It would be conceivable to also set it
	 * on the dollar sign.  */
	ret= std::make_shared <Plain_Dep>
		(flags, places_flags,
		 Place_Target(0, *place_name, place_name->place),
		 variable_name);

	if (has_input && ! place_name_input.empty()) {
		place_name->place <<
			fmt("there must not be a second input redirection %s",
			    show(ret, S_DEFAULT, R_SHOW_INPUT));
		place_name_input.place <<
			fmt("shadowing previous input redirection %s",
			    show_prefix("<", place_name_input));
		if (targets.size() == 1) {
			targets.front()->place <<
				fmt("for target %s", show(*targets.front()));
		} else if (targets.size() > 1) {
			targets.front()->place <<
				fmt("for targets %s...", show(*targets.front()));
		}
		throw ERROR_LOGICAL;
	}
	if (has_input)
		place_name_input= *place_name;
	return ret;
}

shared_ptr <const Dep> Parser::parse_redirect_dep
(Place_Name &place_name_input, Place &place_input,
 const std::vector <shared_ptr <const Place_Target> > &targets)
{
	(void) targets;
	bool has_input= false;

	if (is_operator('<')) {
		has_input= true;
		place_input= (*iter)->get_place();
		assert(! place_input.empty());
		++iter;
	}

	bool has_transient= false;
	Place place_at;
	if (is_operator('@')) {
		place_at= (*iter)->get_place();
		if (has_input) {
			place_at << fmt("expected a filename, not %s", show_operator('@'));
			place_input << fmt("after input redirection using %s",
					   show_operator('<'));
			throw ERROR_LOGICAL;
		}
		++iter;
		has_transient= true;
	}

	if (iter == tokens.end()) {
		if (has_input) {
			place_end << "expected a filename";
			place_input << fmt("after input redirection using %s",
					   show_operator('<'));
			throw ERROR_LOGICAL;
		} else if (has_transient) {
			place_end << "expected the name of a transient target";
			place_at << fmt("after %s", show_operator('@'));
			throw ERROR_LOGICAL;
		} else {
			return nullptr;
		}
	}

	if (! is <Name_Token> ()) {
		if (has_input) {
			(*iter)->get_place_start() <<
				fmt("expected a filename, not %s",
				    show(*iter));
			place_input << fmt("after input redirection using %s",
					   show_operator('<'));
			throw ERROR_LOGICAL;
		} else if (has_transient) {
			(*iter)->get_place_start()
				<< fmt("expected the name of a transient target, not %s",
				       show(*iter));
			place_at << fmt("after %s",
					show_operator('@'));
			throw ERROR_LOGICAL;
		} else {
			return nullptr;
		}
	}

	shared_ptr <Name_Token> name_token= is <Name_Token> ();
	++iter;

	Flags flags= 0;
	if (has_input) {
		flags |= F_INPUT;
	}

	if (! place_name_input.empty())
		assert(! place_input.empty());

	Flags transient_bit= has_transient ? F_TARGET_TRANSIENT : 0;
	shared_ptr <const Dep> ret= std::make_shared <Plain_Dep>
		(flags | transient_bit,
		 Place_Target(transient_bit, *name_token,
			      has_transient ? place_at : name_token->place));

	if (has_input && ! place_name_input.empty()) {
		name_token->place <<
			fmt("there must not be a second input redirection %s",
			    show(ret, S_DEFAULT, R_SHOW_INPUT));
		place_name_input.place <<
			fmt("shadowing previous input redirection %s",
			    show_prefix("<", place_name_input));
		if (targets.size() == 1) {
			targets.front()->place <<
				fmt("for target %s", show(*targets.front()));
		} else if (targets.size() > 1) {
			targets.front()->place <<
				fmt("for targets %s...", show(*targets.front()));
		}
		throw ERROR_LOGICAL;
	}

	if (has_input)
		place_name_input= *name_token;

	if (next_concatenates()) {
		shared_ptr <const Dep> next;
		bool rr= parse_expression(next, place_name_input, place_input, targets);
		/* It can be that an empty list was parsed, in
		 * which case RR is true but the list is empty */
		if (rr && next != nullptr) {
			shared_ptr <Concat_Dep> ret_new= std::make_shared <Concat_Dep> ();
			ret_new->push_back(ret);
			ret_new->push_back(next);
			ret.reset();
			ret= move(ret_new);
		}
	}

	return ret;
}

void Parser::append_copy(      Name &to,
			 const Name &from)
{
	/* Only append if TO ends in a slash */
	if (! (to.last_text().size() != 0 &&
	       to.last_text().back() == '/')) {
		return;
	}

	for (ssize_t i= from.get_n(); i >= 0; --i) {
		for (ssize_t j= from.get_texts()[i].size() - 1; j >= 0; --j) {
			if (from.get_texts()[i][j] == '/') {
				/* Don't append the found slash, as TO already
				 * ends in a slash  */
				to.append_text(from.get_texts()[i].substr(j + 1));

				for (size_t k= i; k < from.get_n(); ++k) {
					to.append_parameter(from.get_parameters()[k]);
					to.append_text(from.get_texts()[k + 1]);
				}
				return;
			}
		}
	}

	/* FROM does not contain slashes; prepend the whole FROM to TO */
	to.append(from);
}

void Parser::get_rule_list(std::vector <shared_ptr <Rule> > &rules,
			   std::vector <shared_ptr <Token> > &tokens,
			   const Place &place_end,
			   shared_ptr <const Place_Target> &target_first)
{
	auto iter= tokens.begin();
	Parser parser(tokens, iter, place_end);
	parser.parse_rule_list(rules, target_first);

	if (iter != tokens.end()) {
		(*iter)->get_place_start()
			<< fmt("expected a rule, not %s", show(*iter));
		throw ERROR_LOGICAL;
	}
}

void Parser::get_expression_list(std::vector <shared_ptr <const Dep> > &deps,
				 std::vector <shared_ptr <Token> > &tokens,
				 const Place &place_end,
				 Place_Name &input,
				 Place &place_input)
{
	auto iter= tokens.begin();
	Parser parser(tokens, iter, place_end);
	std::vector <shared_ptr <const Place_Target>> targets;
	parser.parse_expression_list(deps, input, place_input, targets);
	if (iter != tokens.end()) {
		(*iter)->get_place_start()
			<< fmt("expected a dependency, not %s",
			       show(*iter));
		throw ERROR_LOGICAL;
	}
}

void Parser::get_expression_list_delim(std::vector <shared_ptr <const Dep> > &deps,
				       const char *filename,
				       char c, char c_printed,
				       const Printer &printer)
/* We use getdelim() for parsing.  A more optimized way would be via
 * mmap()+strchr().  */
{
	FILE *file= fopen(filename, "r");
	if (file == nullptr) {
		print_errno(show(filename));
		throw ERROR_BUILD;
	}

	Place place(Place::Type::INPUT_FILE, filename, 0, 0);

	char *lineptr= nullptr;
	size_t n= 0;
	ssize_t len;
	while ((len= getdelim(&lineptr, &n, c, file)) >= 0) {
		++place.line;
		/* LEN is at least one by the specification of
		 * getdelim().  */
		assert(len >= 1);
		assert(lineptr[len] == '\0');

		/* There may or may not be a terminating \n or \0.
		 * getdelim(3) will include it if it is present, but the
		 * file may not have one for the last entry.  */

		if (lineptr[len - 1] == c)
			--len;

		/* An empty line: This corresponds to an empty filename,
		 * and thus we treat is as a syntax error, because
		 * filenames can never be empty.  */
		if (len == 0) {
			free(lineptr);
			fclose(file);
			place << "filename must not be empty";
			printer <<
				fmt("in %s-separated dynamic dependency %s "
				    "declared with flag %s",
				    c == '\0' ? "zero" : "newline",
				    show(filename),
				    show_prefix("-", frmt("%c", c_printed)));
			throw ERROR_LOGICAL;
		}

		string filename_dep= string(lineptr, len);

		if (c != '\0' && filename_dep.find('\0') != string::npos) {
			free(lineptr);
			fclose(file);
			place << fmt("filename %s must not contain %s",
				     show(filename_dep),
				     show_text(string(1, '\0')));
			printer <<
				fmt("in %s-separated dynamic dependency %s "
				    "declared with flag %s",
				    c == '\0' ? "zero" : "newline",
				    show(filename),
				    show_prefix("-", frmt("%c", c_printed)));
			throw ERROR_LOGICAL;
		}
		if (c == '\0') {
			assert(filename_dep.find('\0') == string::npos);
		}

		deps.push_back(std::make_shared <Plain_Dep>
			       (0, Place_Target
				(0, Place_Name(filename_dep, place))));
	}
	free(lineptr);
	if (fclose(file)) {
		print_errno(show(filename));
		throw ERROR_BUILD;
	}
}

void Parser::get_target_arg(std::vector <shared_ptr <const Dep> > &deps,
			    int argc, const char *const *argv)
/*
 *    - Recognize only the special characters "-@[]".  And "-@" only at the
 *      beginning of arguments, or after [ or ], etc.
 *    - Treat whitespace within arguments as part of the name, and only
 *      consider the separation between arguments to be whitespace
 *    - Don't support '$' or other syntax
 *    - Don't need space after flags
 *    - Recognize '[' and ']' in the middle of the string, to denote
 *      concatenation
 */
{
	/* All tokens get the same place, because we don't distinguish
	 * the location within command line arguments  */
	Place place(Place::Type::ARGUMENT);
	std::vector <shared_ptr <Token> > tokens;

	for (int j= 0; j < argc; ++j) {
		const char *p= argv[j];

		bool allow_dash= true;
		bool allow_at= true;

		Environment environment= E_WHITESPACE;
		/* Whether the token is preceded by whitespace */

		while (*p) {
			if (allow_at && *p == '@') {
				tokens.push_back(std::make_shared <Operator>
						 (*p, place, environment));
				++p;
				allow_dash= false;
				allow_at= false;
				environment= 0;
			} else if (*p == '[') {
				tokens.push_back(std::make_shared <Operator>
						 (*p, place, environment));
				++p;
				allow_dash= true;
				allow_at= true;
				environment= 0;
			} else if (*p == ']') {
				tokens.push_back(std::make_shared <Operator>
						 (*p, place, environment));
				++p;
				allow_dash= false;
				allow_at= false;
				environment= 0;
			} else if (allow_dash && *p == '-') {
				++p;
				if (*p == '\0') {
					place << fmt("expected a flag character after %s",
						     show_operator('-'));
					throw ERROR_LOGICAL;
				}
				if (! Tokenizer::is_flag_char(*p)) {
					if (isalnum(*p)) {
						place << fmt("invalid flag %s",
							     show_prefix("-", frmt("%c", *p)));
					} else {
						place << fmt("expected a flag character after dash %s, not %s",
							     show_operator('-'),
							     show_text(string(1, *p)));
					}
					throw ERROR_LOGICAL;
				}
				tokens.push_back(std::make_shared <Flag_Token>
						 (*p, place, environment));
				++p;
				allow_dash= true;
				allow_at= true;
				environment= 0;
			} else {
				const char *q= p;
				while (*p
				       && (*p != '@' || ! allow_at)
				       && (*p != '-' || ! allow_dash)
				       && *p != '[' && *p != ']') {
					++p;
				}
				assert(p > q);
				Place_Name place_name(string(q, p-q), place);
				tokens.push_back(std::make_shared <Name_Token>
						 (place_name, environment));
				allow_dash= false;
				allow_at= false;
				environment= 0;
			}
		}
	}

	Place_Name input;  /* Remains empty */
	Place place_input;  /* Remains empty */
	std::vector <shared_ptr <const Dep> > deps_new;
	get_expression_list(deps_new, tokens, place, input, place_input);
	for (const auto &i: deps_new)
		deps.push_back(i);
}

bool Parser::next_concatenates() const
{
	if (iter == tokens.end())
		return false;

	if ((*iter)->environment & E_WHITESPACE)
		return false;

	if (is <Name_Token> ())
		return true;

	if (! is <Operator> ())
		return false;

	char op= is <Operator> ()->op;
	return op == '(' || op == '[';
}

void Parser::get_file(string filename,
		      int file_fd,
		      Rule_Set &rule_set,
		      shared_ptr <const Place_Target> &target_first,
		      Place &place_first)
{
	assert(file_fd == -1 || file_fd > 1);

	Place place_diagnostic= filename.empty()
		? Place() : Place(Place::Type::OPTION, 'f');
	if (filename.empty())
		filename= FILENAME_INPUT_DEFAULT;
	string filename_passed= filename;
	if (filename_passed == "-")
		filename_passed= "";

	/* Tokenize */
	std::vector <shared_ptr <Token> > tokens;
	Place place_end;
	Tokenizer::parse_tokens_file
		(tokens, Tokenizer::SOURCE,
		 place_end, filename_passed,
		 place_diagnostic, file_fd);

	/* Build rules */
	std::vector <shared_ptr <Rule> > rules;
	Parser::get_rule_list(rules, tokens, place_end, target_first);

	/* Add to set */
	rule_set.add(rules);

	if (rules.empty() && place_first.empty()) {
		place_first= place_end;
	}
}

void Parser::get_string(const char *s,
			Rule_Set &rule_set,
			shared_ptr <const Place_Target> &target_first)
{
	std::vector <shared_ptr <Token> > tokens;
	Place place_end;
	Tokenizer::parse_tokens_string
		(tokens,
		 Tokenizer::OPTION_F,
		 place_end, s,
		 Place(Place::Type::OPTION, 'F'));

	std::vector <shared_ptr <Rule> > rules;
	Parser::get_rule_list(rules, tokens, place_end, target_first);

	rule_set.add(rules);
}

void Parser::add_deps_option_C(std::vector <shared_ptr <const Dep> > &deps,
			       const char *string_)
{
	std::vector <shared_ptr <Token> > tokens;
	Place place_end;
	Tokenizer::parse_tokens_string(tokens,
				       Tokenizer::OPTION_C,
				       place_end, string_,
				       Place(Place::Type::OPTION, 'C'));

	std::vector <shared_ptr <const Dep> > deps_option;
	Place_Name input; /* remains empty */
	Place place_input; /* remains empty */

	Parser::get_expression_list(deps_option, tokens,
				    place_end, input, place_input);

	for (auto &j: deps_option)
		deps.push_back(j);
}

void Parser::parse_rule_list(std::vector <shared_ptr <Rule> > &ret,
			     shared_ptr <const Place_Target> &target_first)
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
