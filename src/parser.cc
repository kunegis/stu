#include "parser.hh"

#include "explain.hh"
#include "tokenizer.hh"
#include "flags.hh"

shared_ptr <Rule> Parser::parse_rule(
	shared_ptr <const Plain_Dep> &target_first)
{
	TRACE_FUNCTION();
	const auto iter_begin= iter;
	Place place_output;
	int redirect_index= -1;
	std::vector <shared_ptr <const Plain_Dep> > targets;

	while (iter != tokens.end()) {
		bool r= parse_target(
			place_output, targets, redirect_index, target_first);
		if (!r) break;
	}
	if (targets.size() == 0) {
		assert(iter == iter_begin);
		return nullptr;
	}

	/* Check that all targets have the same set of parameters */
	std::set <string> parameters_0;
	for (const string &parameter: targets[0]->place_target
		     .place_name.get_parameters()) {
		parameters_0.insert(parameter);
	}
	TRACE("parameters_0.size= %s", frmt("%zu", parameters_0.size()));
	TRACE("targets.size= %s", frmt("%zu", targets.size()));
	assert(targets.size() >= 1);
	for (size_t i= 1; i < targets.size(); ++i) {
		TRACE("i= %s", frmt("%zu", i));
		std::set <string> parameters_i;
		for (const string &parameter:
			     targets[i]->place_target.place_name.get_parameters()) {
			parameters_i.insert(parameter);
		}
		TRACE("parameters_i.size= %s", frmt("%zu", parameters_i.size()));
		if (parameters_i != parameters_0) {
			targets[i]->place <<
				fmt("parameters of target %s differ",
					show(targets[i]));
			targets[0]->place << fmt(
				"from parameters of target %s in rule with multiple targets",
				    show(targets[0]));
			throw ERROR_LOGICAL;
		}
	}

	if (iter == tokens.end()) {
		place_end <<
			fmt("expected a command, %s, %s, or %s",
				show_operator(':'), show_operator(';'),
				show_operator('='));
		targets.back()->place <<
			fmt("after target %s", show(targets.back()));
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
		parse_expression_list(deps, filename_input, place_input, targets);
	}

	/* Command */
	if (iter == tokens.end()) {
		assert(had_colon);
		place_end << fmt("expected a dependency, a command, or %s",
			show_operator(';'));
		targets[0]->place
			<< fmt("for target %s", show(targets[0]));
		throw ERROR_LOGICAL;
	}

	shared_ptr <Command> command;
	bool is_content= false;
	Place place_nocommand_semicolon;
	Place place_equal;

	if ((command= is <Command> ())) {
		++iter;
	} else if (! had_colon && is_operator('=')) {
		place_equal= (*iter)->get_place();
		++iter;

		if (iter == tokens.end()) {
			place_end <<
				fmt("expected a filename, a flag, or %s",
					show_operator('{'));
			place_equal << fmt("after %s", show_operator('='));
			throw ERROR_LOGICAL;
		}

		if ((command= is <Command> ())) {
			++iter;
			assert(targets.size() != 0);
			if (targets.size() != 1) {
				place_equal <<
					fmt("content rule using %s cannot be used",
						show_operator('='));
				targets[0]->place <<
					fmt("with multiple targets %s...",
					    show(targets[0]));
				throw ERROR_LOGICAL;
			}
			if ((targets[0]->flags.get_flags() & F_TARGET_PHONY)) {
				place_equal <<
					fmt("content rule using %s cannot be used",
						show_operator('='));
				targets[0]->place <<
					fmt("with phony target %s",
					    show(targets[0]));
				throw ERROR_LOGICAL;
			}
			/* "No redirected output" is checked later */
			is_content= true;
		} else {
			return parse_remainder_copy_rule(
				place_equal, place_output, targets);
		}
	} else if (is_operator(';')) {
		place_nocommand_semicolon= (*iter)->get_place();
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
		targets[0]->place <<
			fmt("for target %s", show(targets[0]));
		throw ERROR_LOGICAL;
	}

	/* Cases where output redirection is not possible */
	if (! place_output.empty()) {
		/* Already checked before */
		assert((targets[redirect_index]->flags.get_flags() & F_TARGET_PHONY)
			== 0);

		if (command == nullptr) {
			place_output <<
				fmt("output redirection using %s cannot be used",
				    show_operator('>'));
			place_nocommand_semicolon <<
				fmt("in rule for %s without a command",
				    show(targets[0]));
			throw ERROR_LOGICAL;
		}

		if (command != nullptr && is_content) {
			place_output <<
				fmt("output redirection using %s cannot be used",
					show_operator('>'));
			place_equal <<
				fmt("in content rule for %s using %s",
				    show(targets[0]),
				    show_operator('='));
			throw ERROR_LOGICAL;
		}
	}

	/* Cases where input redirection is not possible */
	if (! filename_input.empty()) {
		if (command == nullptr) {
			place_input <<
				fmt("input redirection using %s cannot be used",
					show_operator('<'));
			place_nocommand_semicolon <<
				fmt("in rule for %s without a command",
				    show(targets[0]));
			throw ERROR_LOGICAL;
		} else {
			assert(! is_content);
		}
	}

	if (is_content && targets[0]->flags.get_flags() & F_NO_FOLLOW) {
		command->place << fmt("content rule using %s", show_operator('='));
		targets[0]->flags.place_by_index(I_NO_FOLLOW) <<
			fmt("must not have target flag %s (no-follow)",
				show_operator(frmt("-%c", flags_chars[I_NO_FOLLOW])));
		throw ERROR_LOGICAL;
	}

	return std::make_shared <Rule>
		(move(targets), deps, command, is_content, redirect_index, filename_input);
}

shared_ptr <Rule> Parser::parse_remainder_copy_rule(
	const Place &place_equal,
	const Place &place_output,
	std::vector <shared_ptr <const Plain_Dep> > targets)
{
	Place place_flag_persistent;
	Place place_flag_optional;
	shared_ptr <Name_Token> name_copy_src;

	while (is <Flag_Token> ()) {
		shared_ptr <Flag_Token> flag_token= is <Flag_Token> ();
		if (flag_token->flag == flags_chars[I_PERSISTENT]) {
			place_flag_persistent= flag_token->get_place();
			++iter;
		} else if (flag_token->flag == flags_chars[I_OPTIONAL]) {
			if (! option_g)
				place_flag_optional= flag_token->get_place();
			++iter;
		} else {
			flag_token->get_place()
				<< fmt("flag %s cannot be used",
					show(flag_token));
			place_equal <<
				fmt("in copy rule using %s for target %s",
					show_operator('='),
					show(targets[0]));
			throw ERROR_LOGICAL;
		}
	}

	if (! is <Name_Token> ()) {
		if (iter == tokens.end()) {
			place_end <<
				fmt("expected a filename or %s",
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
	name_copy_src= is <Name_Token> ();
	++iter;

	/* Check that the source file contains only parameters that also
	 * appear in the target */
	std::set <string> parameters;
	for (auto &parameter: targets[0]->place_target
		     .place_name.get_parameters()) {
		parameters.insert(parameter);
	}
	for (size_t jj= 0; jj < name_copy_src->get_n(); ++jj) {
		string parameter=
			name_copy_src->get_parameters()[jj];
		if (parameters.count(parameter) == 0) {
			name_copy_src->places[jj] <<
				fmt("parameter %s cannot appear in copied file %s",
					show_prefix("$", parameter),
					show(name_copy_src));
			targets[0]->place <<
				fmt("because it does not appear in target %s",
					show(targets[0]));
			throw ERROR_LOGICAL;
		}
	}

	if (iter == tokens.end()) {
		place_end << fmt("expected %s",
			show_operator(';'));
		name_copy_src->get_place() <<
			fmt("after copy dependency %s",
				show(name_copy_src));
		throw ERROR_LOGICAL;
	}
	if (! is_operator(';')) {
		(*iter)->get_place() <<
			fmt("expected %s", show_operator(';'));
		name_copy_src->place <<
			fmt("after copy dependency %s",
				show(name_copy_src));
		throw ERROR_LOGICAL;
	}
	++iter;

	if (! place_output.empty()) {
		place_output <<
			fmt("output redirection using %s cannot be used",
				show_operator('>'));
		place_equal <<
			fmt("in copy rule using %s for target %s",
				show_operator('='),
				show(targets[0]));
		throw ERROR_LOGICAL;
	}

	/* Check that there is just a single target */
	if (targets.size() != 1) {
		place_equal <<
			fmt("there cannot be a copy rule using %s",
				show_operator('='));
		targets[0]->place <<
			fmt("for multiple targets %s...",
				show(targets[0]));
		throw ERROR_LOGICAL;
	}

	if (targets[0]->flags.get_flags() & F_TARGET_PHONY) {
		place_equal << fmt("copy rule using %s cannot be used",
			show_operator('='));
		targets[0]->place
			<< fmt("with phony target %s",
				show(targets[0]));
		throw ERROR_LOGICAL;
	}

	if (targets[0]->flags.get_flags() & F_NO_FOLLOW) {
		place_equal << fmt("copy rule using %s cannot be used",
			show_operator('='));
		targets[0]->flags.place_by_index(I_NO_FOLLOW) <<
			fmt("with target having flag %s (no-follow)",
				show_operator(frmt("-%c", flags_chars[I_NO_FOLLOW])));
		throw ERROR_LOGICAL;
	}

	assert(targets.size() == 1);

	/* Append target name when source ends in slash */
	append_copy(*name_copy_src, targets[0]->place_target.place_name);

	return std::make_shared <Rule> (
		targets[0], name_copy_src,
		place_flag_persistent, place_flag_optional);
}

bool Parser::parse_target(
	Place &place_output,
	std::vector <shared_ptr <const Plain_Dep> > &place_targets,
	int &redirect_index,
	shared_ptr <const Plain_Dep> &target_first)
{
	Place place_output_new;
	Place_Flags place_flags;
	shared_ptr <Flag_Token> flag_token;

	while (is <Flag_Token> ()) {
		flag_token= is <Flag_Token> ();
		++iter;
		Index flag_index= flag_get_index(flag_token->flag);
		if (((1 << flag_index) & F_PLACED_TARGET) == 0) {
			string possible;
			for (Flags f= F_PLACED_TARGET, i= 0; f; f >>= 1, ++i)
			{
				if (!(f & 1)) continue;
				if (! possible.empty()) possible += "/";
				possible += show(Flag_View(flags_chars[i]));
			}
			flag_token->place << fmt(
				"flag %s is invalid before target (only %s are possible)",
				show(flag_token), possible);
			explain_target_flags();
			throw ERROR_LOGICAL;
		}
		place_flags.add_placed_index(flag_index, flag_token->place);
	}

	if (is_operator('>')) {
		place_output_new= (*iter)->get_place();
		++iter;
	}

	Place place_of_target;
	if (iter != tokens.end())
		place_of_target= (*iter)->get_place_start();

	if (is_operator('@')) {
		Place place_at= (*iter)->get_place();
		++iter;

		if (iter == tokens.end()) {
			place_end << "expected the name of phony target";
			place_at << fmt("after %s", show_operator('@'));
			throw ERROR_LOGICAL;
		}
		if (! is <Name_Token> ()) {
			(*iter)->get_place_start()
				<< fmt("expected the name of phony target, not %s",
					show(*iter));
			place_at << fmt("after %s", show_operator('@'));
			throw ERROR_LOGICAL;
		}
		place_flags.add_unplaced_flags(F_TARGET_PHONY);
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
		if (flag_token) {
			if (iter == tokens.end())
				place_end << "expected a filename";
			else
				(*iter)->get_place_start() <<
					fmt("expected a filename, not %s",
						show((*iter)));
			flag_token->place <<
				fmt("after flag %s", show(flag_token));
			throw ERROR_LOGICAL;
		}
		return false;
	}

	/* Target */
	shared_ptr <Name_Token> target_name= is <Name_Token> ();
	++iter;

	assert(! target_name->empty());
	string param_1, param_2;
	if (! target_name->find_duplicate_parameters(param_1, param_2)) {
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

	shared_ptr <const Plain_Dep> target= std::make_shared <Plain_Dep>
		(place_flags, Place_Target(place_flags.get_flags() & F_TARGET_PHONY,
			*target_name, place_of_target));

	if (place_flags.get_flags() & F_TARGET_PHONY && flag_token) {
		flag_token->place << fmt("flag %s cannot be used", show(flag_token));
		place_of_target << fmt("before phony target %s", show(target));
		explain_target_flags();
		throw ERROR_LOGICAL;
	}

	if (! place_output_new.empty() && place_flags.get_flags() & F_NO_FOLLOW) {
		place_output_new << fmt("output redirection using %s cannot be used",
			show_operator('>'));
		place_flags.place_by_index(I_NO_FOLLOW) <<
			fmt("before target with flag %s (no-follow)",
				show_operator(frmt("-%c", flags_chars[I_NO_FOLLOW])));
		throw ERROR_LOGICAL;
	}

	if (! place_output_new.empty()) {
		if (! place_output.empty()) {
			place_output_new <<
				fmt("there cannot be a second output redirection %s",
					show_prefix(">", target));
			assert(place_targets[redirect_index]
				->place_target.place_name.get_n() == 0);
			assert((place_targets[redirect_index]->flags.get_flags()
					& F_TARGET_PHONY) == 0);
			place_output <<
				fmt("shadowing previous output redirection %s",
					show_prefix
					(">", place_targets[redirect_index]->place_target
						.unparametrized().get_name_nondynamic()));
			throw ERROR_LOGICAL;
		}
		place_output= place_output_new;
		assert(! place_output.empty());
		redirect_index= place_targets.size();
	}

	if (place_flags.get_flags() & F_TARGET_PHONY && ! place_output_new.empty()) {
		target->place <<
			fmt("phony target %s is invalid", show(target));
		place_output_new <<
			fmt("after output redirection using %s",
				show_operator('>'));
		throw ERROR_LOGICAL;
	}
	if (target_first == nullptr)
		target_first= target;

	place_targets.push_back(target);
	return true;
}

bool Parser::parse_expression_list(
	std::vector <shared_ptr <const Dep> > &ret,
	Place_Name &place_name_input,
	Place &place_input,
	const std::vector <shared_ptr <const Plain_Dep> > &targets)
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

bool Parser::parse_expression(
	shared_ptr <const Dep> &ret,
	Place_Name &place_name_input,
	Place &place_input,
	const std::vector <shared_ptr <const Plain_Dep> > &targets)
{
	TRACE_FUNCTION();
	assert(ret == nullptr);

	/* '(' expression* ')' */
	if (ret= parse_compound_dep(place_name_input, place_input, targets))
		return true;

	/* '[' expression* ']' */
	if (ret= parse_dynamic_dep(place_name_input, place_input, targets))
		return true;

	/* flag expression */
	if (is <Flag_Token> ()) {
		const Flag_Token &flag_token= *is <Flag_Token> ();
		const Place place_flag= (*iter)->get_place();
		const Index i_flag= flag_get_index(flag_token.flag);

		assert((1 << i_flag) & F_PLACED);
		if (! ((1 << i_flag) & F_PLACED_DEPENDENCY)) {
			string possible;
			for (Flags f= F_PLACED_DEPENDENCY, i= 0; f; f >>= 1, ++i)
			{
				if (!(f & 1)) continue;
				if (! possible.empty()) possible += "/";
				possible += show(Flag_View(flags_chars[i]));
			}
			place_flag << fmt(
				"flag %s is invalid before dependency (only %s are possible)",
				show(Flag_View(flags_chars[i_flag])), possible);
			throw ERROR_LOGICAL;
		}

		++iter;

		if (! parse_expression(ret, place_name_input, place_input, targets)) {
			if (iter == tokens.end()) {
				place_end << "expected a dependency";
			} else {
				(*iter)->get_place_start() <<
					fmt("expected a dependency, not %s",
					    show(*iter));
			}
			place_flag << fmt("after flag %s", show(flag_token));
			throw ERROR_LOGICAL;
		}

		/* A dependency must not be an input dependency and optional at the same
		 * time.  Note: Input redirection must not appear in dynamic dependencies,
		 * and therefore it is sufficient to check this here. */
		if (! place_name_input.place.empty() &&
			flag_token.flag == flags_chars[I_OPTIONAL])
		{
			place_input <<
				fmt("input redirection using %s cannot be used",
					show_operator('<'));
			place_flag <<
				fmt("in conjunction with optional dependency flag %s",
					show(Flag_View(flags_chars[I_OPTIONAL])));
			throw ERROR_LOGICAL;
		}

		/* Add the flag */
		if (! ((i_flag == I_OPTIONAL && option_g) ||
		       (i_flag == I_TRIVIAL  && option_a)))
		{
			shared_ptr <Dep> ret_new= ret->clone();
			ret_new->flags.add_placed_index(i_flag,
				place_flag);
			ret= ret_new;
		}

		return true;
	}

	/* '$' ; variable dependency */
	if (ret= parse_variable_dep(place_name_input, place_input, targets))
		return true;

	/* Redirect dependency */
	if (ret= parse_redirect_dep(place_name_input, place_input, targets))
		return true;

	return false;
}

shared_ptr <const Dep> Parser::parse_compound_dep(
	Place_Name &place_name_input,
	Place &place_input,
	const std::vector <shared_ptr <const Plain_Dep> > &targets)
{
	if (! is_operator('('))
		return nullptr;

	shared_ptr <const Dep> ret;

	Place place_paren= (*iter)->get_place();
	++iter;
	std::vector <shared_ptr <const Dep> > r;
	if (parse_expression_list(r, place_name_input, place_input, targets)) {
		assert(r.size() >= 1);
		if (r.size() > 1) {
			ret= std::make_shared <Compound_Dep> (move(r), place_paren);
		} else {
			ret= move(r.at(0));
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

	/* If RET is null, it means we had empty parentheses.  Return an empty
	 * Compound_Dependency in that case. */
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

	return ret;
}

shared_ptr <const Dep> Parser::parse_dynamic_dep(
	Place_Name &place_name_input,
	Place &place_input,
	const std::vector <shared_ptr <const Plain_Dep> > &targets)
{
	if (! is_operator('['))
		return nullptr;
	shared_ptr <const Dep> ret;
	Place place_bracket= (*iter)->get_place();
	++iter;
	std::vector <shared_ptr <const Dep> > content;
	parse_expression_list(content, place_name_input, place_input, targets);

	if (iter == tokens.end()) {
		place_end << fmt("expected a dependency or %s", show_operator(']'));
		place_bracket << fmt("after opening %s", show_operator('['));
		throw ERROR_LOGICAL;
	}
	if (! is_operator(']')) {
		(*iter)->get_place_start() <<
			fmt("expected a dependency or %s, not %s",
				show_operator(']'), show(*iter));
		place_bracket << fmt("after opening %s", show_operator('['));
		throw ERROR_LOGICAL;
	}
	++iter;
	shared_ptr <Compound_Dep> ret_nondynamic=
		std::make_shared <Compound_Dep> (place_bracket);
	for (auto &j: content) {
		if (j->flags.get_flags() & F_VARIABLE) {
			j->get_place() <<
				fmt("variable dependency %s cannot appear",
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

	assert(ret);
	return ret;
}

shared_ptr <const Dep> Parser::parse_variable_dep(
	Place_Name &place_name_input,
	Place &place_input,
	const std::vector <shared_ptr <const Plain_Dep> > &targets)
{
	bool has_input= false;
	shared_ptr <const Dep> ret;
	if (! is_operator('$'))
		return nullptr;
	const Place place_dollar= (*iter)->get_place();
	++iter;

	assert(iter != tokens.end());
	if (! is_operator('[')) {
		/* The '$' and '[' operators are only generated when they both appear in
		 * conjunction. */
		should_not_happen();
		return nullptr;
	}
	++iter;

	/* Flags */
	Place_Flags place_flags;
	place_flags.add_unplaced_index(I_VARIABLE);
	Place place_flag_last;
	shared_ptr <Flag_Token> flag_token_last;
	while (is_flag(flags_chars[I_PERSISTENT]) ||
		is_flag(flags_chars[I_OPTIONAL]) ||
		is_flag(flags_chars[I_TRIVIAL]))
	{
		flag_token_last= is <Flag_Token> ();
		place_flag_last= (*iter)->get_place();
		if (is_flag(flags_chars[I_PERSISTENT])) {
			place_flags.add_placed_index(I_PERSISTENT,
				place_flag_last);
		} else if (is_flag(flags_chars[I_OPTIONAL])) {
			if (! option_g) {
				(*iter)->get_place() <<
					fmt("optional dependency using %s cannot appear",
						show(Flag_View('o')));
				place_dollar << "within dynamic variable declaration";
				throw ERROR_LOGICAL;
			}
		} else if (is_flag(flags_chars[I_TRIVIAL])) {
			if (! option_a) {
				place_flags.add_placed_index(I_TRIVIAL, place_flag_last);
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
		place_flags.add_unplaced_index(I_INPUT);
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

	/* The place of the variable dependency as a whole is set on the name contained in
	 * it.  It would be conceivable to also set it on the dollar sign. */
	ret= std::make_shared <Plain_Dep> (place_flags,
		Place_Target(0, *place_name, place_name->place), variable_name);

	if (has_input && ! place_name_input.empty()) {
		place_name->place <<
			fmt("there cannot be a second input redirection %s",
			    show(ret, S_DEFAULT, R_SHOW_INPUT));
		place_name_input.place <<
			fmt("shadowing previous input redirection %s",
			    show_prefix("<", place_name_input));
		if (targets.size() == 1) {
			targets.front()->place <<
				fmt("for target %s", show(targets.front()));
		} else if (targets.size() > 1) {
			targets.front()->place <<
				fmt("for targets %s...", show(targets.front()));
		}
		throw ERROR_LOGICAL;
	}
	if (has_input)
		place_name_input= *place_name;
	return ret;
}

shared_ptr <const Dep> Parser::parse_redirect_dep(
	Place_Name &place_name_input,
	Place &place_input,
	const std::vector <shared_ptr <const Plain_Dep> > &targets)
{
	bool has_input= false;

	if (is_operator('<')) {
		has_input= true;
		place_input= (*iter)->get_place();
		assert(! place_input.empty());
		++iter;
	}

	bool has_phony= false;
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
		has_phony= true;
	}

	if (iter == tokens.end()) {
		if (has_input) {
			place_end << "expected a filename";
			place_input << fmt("after input redirection using %s",
					   show_operator('<'));
			throw ERROR_LOGICAL;
		} else if (has_phony) {
			place_end << "expected the name of a phony target";
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
		} else if (has_phony) {
			(*iter)->get_place_start()
				<< fmt("expected the name of a phony target, not %s",
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

	Place_Flags place_flags;
	if (has_input) {
		place_flags.add_unplaced_index(I_INPUT);
	}

	if (! place_name_input.empty())
		assert(! place_input.empty());

	if (has_phony) {
		place_flags.add_unplaced_index(I_TARGET_PHONY);
	}
	Flags phony_bit= has_phony ? F_TARGET_PHONY : 0;
	shared_ptr <const Dep> ret= std::make_shared <Plain_Dep>
		(place_flags,
			Place_Target(phony_bit, *name_token,
				has_phony ? place_at : name_token->place));

	if (has_input && ! place_name_input.empty()) {
		name_token->place <<
			fmt("there cannot be a second input redirection %s",
			    show(ret, S_DEFAULT, R_SHOW_INPUT));
		place_name_input.place <<
			fmt("shadowing previous input redirection %s",
			    show_prefix("<", place_name_input));
		if (targets.size() == 1) {
			targets.front()->place <<
				fmt("for target %s", show(targets.front()));
		} else if (targets.size() > 1) {
			targets.front()->place <<
				fmt("for targets %s...", show(targets.front()));
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
				/* Don't append the found slash, as TO already ends in a
				 * slash */
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

void Parser::get_rule_list(
	std::vector <shared_ptr <Rule> > &rules,
	std::vector <shared_ptr <Token> > &tokens,
	const Place &place_end,
	shared_ptr <const Plain_Dep> &target_first)
{
	TRACE_FUNCTION();
	auto iter= tokens.begin();
	Parser parser(tokens, iter, place_end);
	parser.parse_rule_list(rules, target_first);

	if (iter != tokens.end()) {
		(*iter)->get_place_start()
			<< fmt("expected a rule, not %s", show(*iter));
		throw ERROR_LOGICAL;
	}
}

void Parser::get_expression_list(
	std::vector <shared_ptr <const Dep> > &deps,
	std::vector <shared_ptr <Token> > &tokens,
	const Place &place_end,
	Place_Name &input,
	Place &place_input)
{
	auto iter= tokens.begin();
	Parser parser(tokens, iter, place_end);
	std::vector <shared_ptr <const Plain_Dep>> targets;
	parser.parse_expression_list(deps, input, place_input, targets);
	if (iter != tokens.end()) {
		(*iter)->get_place_start()
			<< fmt("expected a dependency, not %s", show(*iter));
		throw ERROR_LOGICAL;
	}
}

void Parser::get_expression_list_delim(
	std::vector <shared_ptr <const Dep> > &deps,
	const char *filename,
	const Place &place_filename,
	char c, char c_printed,
	const Printer &printer,
	bool allow_enoent)
/* We use getdelim() for parsing.  A more optimized way would be via mmap()+strchr(). */
{
	TRACE_FUNCTION();
	TRACE("filename= %s", filename);
	FILE *file= fopen(filename, "r");
	if (file == nullptr) {
		if (allow_enoent && errno == ENOENT)
			return;
		place_filename << format_errno("fopen", filename);
		throw ERROR_BUILD;
	}

	Place place(Place::Type::INPUT_FILE, filename, 0, 0);

	char *lineptr= nullptr;
	size_t n= 0;
	ssize_t len;
	while ((len= getdelim(&lineptr, &n, c, file)) >= 0) {
		++place.line;
		/* LEN is at least one by the specification of getdelim(). */
		assert(len >= 1);
		assert(lineptr[len] == '\0');

		/* There may or may not be a terminating \n or \0.  getdelim(3) will
		 * include it if it is present, but the file may not have one for the last
		 * entry. */

		if (lineptr[len - 1] == c)
			--len;

		/* An empty line: This corresponds to an empty filename, and thus we treat
		 * is as a syntax error, because filenames can never be empty. */
		if (len == 0) {
			free(lineptr);
			fclose(file);
			place << "filename must not be empty";
			printer << fmt(
				"in %s-separated dynamic dependency %s declared with flag %s",
					c == '\0' ? "zero" : "newline",
					show(filename),
					show(Flag_View(c_printed)));
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
				fmt("in %s-separated dynamic dependency %s declared with flag %s",
					c == '\0' ? "zero" : "newline",
					show(filename),
					show(Flag_View(c_printed)));
			throw ERROR_LOGICAL;
		}
		if (c == '\0') {
			assert(filename_dep.find('\0') == string::npos);
		}

		deps.push_back(std::make_shared <Plain_Dep> (
			Place_Target(0, Place_Name(filename_dep, place))));
	}
	free(lineptr);
	if (fclose(file)) {
		place_filename << format_errno("fclose", filename);
		throw ERROR_BUILD;
	}
}

void Parser::get_target_arg(
	std::vector <shared_ptr <const Dep> > &deps,
	int argc,
	const char *const *argv)
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
	/* All tokens get the same place, because we don't distinguish the location within
	 * command line arguments */
	Place place(Place::Type::ARGUMENT);
	std::vector <shared_ptr <Token> > tokens;

	for (int j= 0; j < argc; ++j)
		Tokenizer::parse_tokens_arg(tokens, argv[j], place);

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

void Parser::get_file(
	const char *filename,
	int file_fd,
	Rule_Set &rule_set,
	shared_ptr <const Plain_Dep> &target_first,
	Place &place_first)
{
	assert(file_fd == -1 || file_fd > 1);
	assert(filename);

	Place place_diagnostic=
		!filename[0] ? Place() : Place(Place::Type::OPTION, 'f');
	if (!filename[0])
		filename= FILENAME_INPUT_DEFAULT;
	const char *filename_passed= filename;
	if (!strcmp(filename_passed, "-"))
		filename_passed= "";

	/* Tokenize */
	std::vector <shared_ptr <Token> > tokens;
	Place place_end;
	Tokenizer::parse_tokens_file(
		tokens, Tokenizer::SOURCE, place_end, filename_passed,
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

void Parser::get_string(
	const char *s,
	Rule_Set &rule_set,
	shared_ptr <const Plain_Dep> &target_first)
{
	std::vector <shared_ptr <Token> > tokens;
	Place place_end;
	Tokenizer::parse_tokens_string(
		tokens, Tokenizer::OPTION_F, place_end, s, Place(Place::Type::OPTION, 'F'));

	std::vector <shared_ptr <Rule> > rules;
	Parser::get_rule_list(rules, tokens, place_end, target_first);

	rule_set.add(rules);
}

void Parser::add_deps_option_C(
	std::vector <shared_ptr <const Dep> > &deps,
	const char *string_)
{
	std::vector <shared_ptr <Token> > tokens;
	Place place_end;
	Tokenizer::parse_tokens_string(
		tokens, Tokenizer::OPTION_C, place_end, string_,
		Place(Place::Type::OPTION, 'C'));

	std::vector <shared_ptr <const Dep> > deps_option;
	Place_Name input; /* remains empty */
	Place place_input; /* remains empty */

	Parser::get_expression_list(deps_option, tokens, place_end, input, place_input);

	for (auto &j: deps_option)
		deps.push_back(j);
}

void Parser::parse_rule_list(
	std::vector <shared_ptr <Rule> > &ret,
	shared_ptr <const Plain_Dep> &target_first)
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
