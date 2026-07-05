#include "rule.hh"

Rule::Rule(
	std::vector <shared_ptr <const Plain_Dep> > &&targets_,
	std::vector <shared_ptr <const Dep> > &&deps_,
	const Place &place_,
	const shared_ptr <const Command> &command_,
	const Placed_Name &placed_name_input_,
	bool is_content_,
	Target_Index output_target_index_,
	bool is_copy_)
	: targets(targets_),
	  deps(deps_),
	  place(place_),
	  command(command_),
	  placed_name_input(placed_name_input_),
	  output_target_index(output_target_index_),
	  is_content(is_content_),
	  is_copy(is_copy_)
{ }

Rule::Rule(
	std::vector <shared_ptr <const Plain_Dep> > &&targets_,
	const std::vector <shared_ptr <const Dep> > &deps_,
	shared_ptr <const Command> command_,
	bool is_content_,
	Target_Index output_target_index_,
	const Placed_Name &placed_name_input_)
	: targets(targets_),
	  deps(deps_),
	  place(targets_[0]->place),
	  command(command_),
	  placed_name_input(placed_name_input_),
	  output_target_index(output_target_index_),
	  is_content(is_content_),
	  is_copy(false)
{
	assert(targets.size() != 0);
	assert(output_target_index == TARGET_INDEX_NONE ||
		output_target_index < targets.size());
	if (output_target_index != TARGET_INDEX_NONE) {
		assert((targets[output_target_index]->flags.get_flags()
			& F_TARGET_PHONY) == 0);
	}

	/* Check that all dependencies only include
	 * parameters from the target */
	std::set <string> parameters;
	for (auto &parameter: get_parameters()) {
		parameters.insert(parameter);
	}

	/* Check that only valid parameters are used */
	for (const auto &d: deps) {
		d->check();
		check_unparametrized(d, parameters);
	}
}

Rule::Rule(
	shared_ptr <const Plain_Dep> target_,
	shared_ptr <const Placed_Name> placed_name_source_,
	const Place &place_persistent,
	const Place &place_optional)
	: targets{target_},
	  place(target_->place),
	  placed_name_input(*placed_name_source_),
	  output_target_index(TARGET_INDEX_NONE),
	  is_content(false),
	  is_copy(true)
{
	auto dep= std::make_shared <Plain_Dep>
		(Placed_Target(0, *placed_name_source_));

	if (! place_persistent.empty()) {
		dep->flags.add_placed_index(I_PERSISTENT, place_persistent);
	}
	if (! place_optional.empty()) {
		dep->flags.add_placed_index(I_OPTIONAL, place_optional);
	}

	deps.push_back(dep);
}

shared_ptr <const Rule> Rule::instantiate(
	shared_ptr <const Rule> rule,
	const std::map <string, string> &mapping)
{
	assert(rule->get_parameters().size() != 0);

	std::vector <shared_ptr <const Plain_Dep> >
		placed_targets(rule->targets.size());
	for (size_t i= 0; i < rule->targets.size(); ++i) {
		shared_ptr <const Plain_Dep> instantiated=
			to <Plain_Dep> (rule->targets[i]->instantiate(mapping));
		assert(instantiated);
		placed_targets[i]= instantiated;
	}

	std::vector <shared_ptr <const Dep> > deps;
	for (auto &dep: rule->deps)
		deps.push_back(dep->instantiate(mapping));

	shared_ptr <Placed_Name> placed_name_input=
		rule->placed_name_input.instantiate(mapping);

	return std::make_shared <Rule> (
		move(placed_targets),
		move(deps), rule->place, rule->command,
		*placed_name_input,
		rule->is_content, rule->output_target_index,
		rule->is_copy);
}

void Rule::render(Parts &parts, Rendering rendering) const
{
	parts.append_operator("Rule(");

	bool first= true;
	for (auto t: targets) {
		if (first)
			first= false;
		else
			parts.append_space();
		t->render(parts, rendering);
	}

	if (deps.size() != 0) {
		parts.append_operator(":");
		parts.append_space();
	}
	for (auto i= deps.begin(); i != deps.end(); ++i) {
		if (i != deps.begin()) {
			parts.append_space();
		}
		(*i)->render(parts, rendering);
	}

	parts.append_operator(")");
}

void Rule::check_unparametrized(
	shared_ptr <const Dep> dep,
	const std::set <string> &parameters) const
{
	assert(dep != nullptr);

	if (auto dynamic_dep= to <const Dynamic_Dep> (dep)) {
		check_unparametrized(dynamic_dep->dep, parameters);
	} else if (auto compound_dep= to <const Compound_Dep> (dep)) {
		for (const auto &d: compound_dep->deps) {
			check_unparametrized(d, parameters);
		}
	} else if (auto concat_dep= to <const Concat_Dep> (dep)) {
		for (const auto &d: concat_dep->deps) {
			check_unparametrized(d, parameters);
		}
	} else if (auto plain_dep= to <const Plain_Dep> (dep)) {
		for (size_t jj= 0; jj < plain_dep->placed_target.placed_name.get_n(); ++jj) {
			string parameter= plain_dep->placed_target.placed_name
				.get_parameters()[jj];
			if (parameters.count(parameter) != 0) continue;

			plain_dep->placed_target.placed_name.get_places()[jj] << fmt(
				"parameter %s cannot appear in dependency %s",
				show(Prefix_View("$", parameter)),
				show(plain_dep->placed_target));
			if (targets.size() == 1) {
				targets[0]->place <<
					fmt("because it does not appear in target %s",
						show(targets[0]));
			} else {
				place << fmt(
					"because it does not appear in any of the targets %s... of the rule",
					show(targets[0]));
			}
			throw ERR_LOGICAL;
		}
	} else {
		unreachable();
	}
}

void Rule::check_duplicate_target() const
{
	for (size_t i= 0; i < targets.size(); ++i) {
		for (size_t j= 0; j < i; ++j) {
			if (! targets[i]->placed_target
				.equals_same_length(targets[j]->placed_target))
				continue;
			targets[i]->place <<
				fmt("there cannot be a target %s",
					show(targets[i]));
			targets[j]->place <<
				fmt("shadowing target %s of the same rule",
					show(targets[j]));
			throw ERR_LOGICAL;
		}
	}
}

void Rule::canonicalize()
{
	for (size_t i= 0; i < targets.size(); ++i) {
		shared_ptr <Dep> d= targets[i]->clone();
		shared_ptr <Plain_Dep> e= to <Plain_Dep> (d);
		assert(e);
		e->placed_target.canonicalize();
		targets[i]= e;
	}
}

void render(shared_ptr <const Rule> rule, Parts &parts, Rendering rendering)
{
	rule->render(parts, rendering);
}

void Rule_Set::add(std::vector <shared_ptr <Rule> > &rules_)
{
	for (auto &rule: rules_) {
		rule->canonicalize();
		rule->check_duplicate_target();
		if (! rule->is_parametrized()) {
			add_unparametrized_rule(rule);
		} else {
			add_parametrized_rule(rule);
		}
	}
}

shared_ptr <const Rule> Rule_Set::get(
	Hash_Dep hash_dep,
	shared_ptr <const Rule> &param_rule,
	std::map <string, string> &mapping_parameter,
	const Place &place,
	shared_ptr <const Plain_Dep> &target_plain_dep,
	Target_Index &target_index)
{
	TRACE_FUNCTION();
	TRACE("hash_dep= %s", show(hash_dep));
	assert(hash_dep.is_file() || hash_dep.is_phony());
	assert((hash_dep.get_front_word() & ~F_TARGET_PHONY) == 0);
	assert(mapping_parameter.size() == 0);
	assert(!target_plain_dep);

	hash_dep.canonicalize_plain();

	/* Check for an unparametrized rule.  Since we keep them in a map by target
	 * filename(s), there can only be a single matching rule to begin with.  (I.e., if
	 * multiple unparametrized rules for the same filename exist, then that error is
	 * caught earlier when the Rule_Set is built.) */
	auto i= rules_unparam.find(hash_dep);
	if (i != rules_unparam.end()) {
		target_index= i->second.first;
		shared_ptr <const Rule> rule= i->second.second;
		assert(rule != nullptr);
		assert(rule->targets.front()->placed_target.placed_name.get_n() == 0);
#ifndef NDEBUG
		/* Check that the target is a target of the found rule */
		bool found= false;
		for (auto ta: rule->targets) {
			Hash_Dep t= ta->placed_target.unparametrized();
			t.canonicalize();
			if (t == hash_dep)
				found= true;
		}
		assert(found);
#endif /* ! NDEBUG */
		param_rule= rule;
		target_plain_dep= rule->targets[target_index];
		TRACE("target_plain_dep= %s", show_trace(target_plain_dep));
		return rule;
	}

	/*
	 * Parametrized rules
	 */
	Best_Rule_Finder best_rule_finder;

	/* Search the best parametrized rule, if there is an affix in the rule */
	for (auto it= rules_param_prefix.find(hash_dep.get_name_nondynamic());
	     it != rules_param_prefix.end(); ++it) {
		best_rule_finder.check(hash_dep, (*it).second, (*it).first);
	}
	string target_reversed= hash_dep.get_name_nondynamic();
	std::reverse(target_reversed.begin(), target_reversed.end());
	for (auto it= rules_param_suffix.find(target_reversed);
	     it != rules_param_suffix.end(); ++it) {
		best_rule_finder.check(hash_dep, (*it).second, (*it).first);
	}

	/* Search the best parametrized rule, if the rules are affixless */
	for (auto &ii: rules_param_bare)
		best_rule_finder.check(hash_dep, ii.second, ii.first);

	/* No rule matches */
	if (best_rule_finder.count() == 0)
		return nullptr;

	/* More than one rule matches:  error */
	if (best_rule_finder.count() != 1 ) {
		place << fmt("multiple minimal matching rules for target %s",
			     show(hash_dep));
		for (const Found_Rule &f: best_rule_finder.all_best()) {
			TRACE("f.target= %s", show(f.target));
			f.target->place << fmt("rule with target %s", show(f.target));
		}
		explain_minimal_matching_rule();
		throw ERR_LOGICAL;
	}

	/* Instantiate the rule */
	shared_ptr <const Rule> rule_best= best_rule_finder.best().rule;
	mapping_parameter= best_rule_finder.best().mapping;
	shared_ptr <const Rule> ret(Rule::instantiate(rule_best, mapping_parameter));
	param_rule= rule_best;
	target_plain_dep= best_rule_finder.best().target;
	target_index= best_rule_finder.best().target_index;
	TRACE("target_plain_dep= %s", show_trace(target_plain_dep));
	return ret;
}

void Rule_Set::print_for_option_P() const
{
	std::unordered_set <shared_ptr <const Rule> > seen;
	for (auto i: rules_unparam)  {
		if (seen.find(i.second.second) != seen.end()) continue;
		seen.insert(i.second.second);
		string text= show(i.second.second, S_OPTION_P);
		puts(text.c_str());
	}
	for (auto i: rules_param)  {
		string text= show(i, S_OPTION_P);
		puts(text.c_str());
	}
}

void Rule_Set::print_for_option_I() const
{
	std::set <string> filenames;
	for (auto i: rules_unparam)  {
		const Rule &rule= * i.second.second;
		if (rule.must_exist())
			continue;
		for (auto target: rule.targets) {
			if (target->flags.get_flags() & F_TARGET_PHONY)
				continue;
			filenames.insert(
				show(target->placed_target.placed_name, S_OPTION_I, R_GLOB));
		}
	}
	for (auto rule: rules_param)  {
		if (rule->must_exist())
			continue;
		for (auto target: rule->targets) {
			if (target->flags.get_flags() & F_TARGET_PHONY)
				continue;
			filenames.insert(
				show(target->placed_target.placed_name, S_OPTION_I, R_GLOB));
		}
	}
	for (const string &filename: filenames) {
		puts(filename.c_str());
	}
}

void Rule_Set::add_unparametrized_rule(shared_ptr <Rule> rule)
{
	for (size_t i= 0; i < rule->targets.size(); ++i) {
		auto &t= rule->targets[i];
		Hash_Dep hash_dep= t->placed_target.unparametrized();
		if (rules_unparam.count(hash_dep)) {
			t->place <<
				fmt("there cannot be a second rule for target %s",
					show(hash_dep));
			auto rule_2= rules_unparam.at(hash_dep);
			for (auto t2: rule_2.second->targets) {
				assert(t2->placed_target.placed_name.get_n() == 0);
				if (t2->placed_target.unparametrized() == hash_dep) {
					t2->place <<
						fmt("shadowing previous rule %s",
							show(hash_dep));
					break;
				}
			}
			throw ERR_LOGICAL;
		}
		rules_unparam[hash_dep]= {i, rule};
	}
}

void Rule_Set::add_parametrized_rule(shared_ptr <Rule> rule)
{
	TRACE_FUNCTION();
	TRACE("rule= %s", show(rule));
	rules_param.insert(rule);

	for (Target_Index ti= 0; ti < rule->targets.size(); ++ti) {
		auto target= rule->targets[ti];
		const Name &name= target->placed_target.placed_name;
		assert(name.is_parametrized());
		const string prefix= name.get_texts()[0];
		const string suffix= name.get_texts()[name.get_n()];
		if (prefix.empty() && suffix.empty()) {
			rules_param_bare.push_back({ti, rule});
		}
		if (!prefix.empty()) {
			rules_param_prefix.insert(prefix, {ti, rule});
		}
		if (!suffix.empty()) {
			string suffix_inv= suffix;
			std::reverse(suffix_inv.begin(), suffix_inv.end());
			rules_param_suffix.insert(suffix_inv, {ti, rule});
		}
		/* Special rule (a):  Target starts with './' follwed by a parameter */
		if (prefix == "./")
			rules_param_bare.push_back({ti, rule});
		/* Special rules (b) and (c):  Target starts with a parameter, followed by
		 * a slash */
		if (prefix.empty() && name.get_texts()[1][0] == '/')
			rules_param_bare.push_back({ti, rule});
	}
}

bool Found_Rule::operator<(const Found_Rule &that) const
{
	TRACE_FUNCTION();
	if (target->placed_target.placed_name < that.target->placed_target.placed_name)
		return true;
	if (target->placed_target.placed_name > that.target->placed_target.placed_name)
		return false;
	bool ret= rule.get() < that.rule.get();
	return ret;
}

void Best_Rule_Finder::check(
	const Hash_Dep &hash_dep,
	shared_ptr <const Rule> rule,
	Target_Index target_index)
{
	TRACE_FUNCTION();
	TRACE("hash_dep= %s", show(hash_dep));
	TRACE("rule= %s", show(rule));
	TRACE("target_index= %s", frmt("%u", target_index));
	shared_ptr <const Plain_Dep> t= rule->targets[target_index];

	assert(t->placed_target.placed_name.get_n() > 0);
	std::map <string, string> mapping;
	std::vector <size_t> anchoring;
	int priority;

	/* The parametrized rule is of another type */
	if (hash_dep.get_front_word() !=
		(t->flags.get_flags() & F_TARGET_PHONY))
		return;

	/* The parametrized rule does not match */
	if (! t->placed_target.placed_name.match(
			hash_dep.get_name_nondynamic(),
			mapping, anchoring, priority))
		return;

	assert(anchoring.size() == 2 * t->placed_target.placed_name.get_n());

	/* Check whether the rule is dominated by at least one other rule; also, avoid
	 * inserting the same rule twice (which happens if the rule was found from both a
	 * prefix and a suffix.)  But note that there can be two parametrized targets in
	 * one rule which both match the target, in which case we do want to throw a
	 * "duplicate rule" error (because Stu wouldn't know how to chose the parameter),
	 * and therefore we also need to compare anchorings. */
	for (const Found_Rule &f: found_rules) {
		if (rule == f.rule && target_index == f.target_index)
			return;
		if (Name::anchoring_dominates(
				f.anchoring, anchoring,
				f.priority, priority))
			return;
	}

	/* Check whether the rule dominates all other rules */
	bool is_best= true;
	for (const Found_Rule &f: found_rules) {
		if (! Name::anchoring_dominates(
				anchoring, f.anchoring,
				priority, f.priority))
			is_best= false;
	}
	if (is_best) found_rules.clear();

	found_rules.insert({rule, mapping, anchoring, priority, t, target_index});
}
