#include "rule.hh"

Rule::Rule(std::vector <shared_ptr <const Place_Target> > &&place_targets_,
	   std::vector <shared_ptr <const Dep> > &&deps_,
	   const Place &place_,
	   const shared_ptr <const Command> &command_,
	   Name &&filename_,
	   bool is_hardcode_,
	   int redirect_index_,
	   bool is_copy_)
	:  place_targets(place_targets_),
	   deps(deps_),
	   place(place_),
	   command(command_),
	   filename(filename_),
	   redirect_index(redirect_index_),
	   is_hardcode(is_hardcode_),
	   is_copy(is_copy_)
{  }

Rule::Rule(std::vector <shared_ptr <const Place_Target> > &&place_targets_,
	   const std::vector <shared_ptr <const Dep> > &deps_,
	   shared_ptr <const Command> command_,
	   bool is_hardcode_,
	   int redirect_index_,
	   const Name &filename_)
	:  place_targets(place_targets_),
	   deps(deps_),
	   place(place_targets_[0]->place),
	   command(command_),
	   filename(filename_),
	   redirect_index(redirect_index_),
	   is_hardcode(is_hardcode_),
	   is_copy(false)
{
	assert(place_targets.size() != 0);
	assert(redirect_index>= -1);
	assert(redirect_index < (ssize_t) place_targets.size());
	if (redirect_index >= 0) {
		assert((place_targets[redirect_index]->flags & F_TARGET_TRANSIENT) == 0);
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

Rule::Rule(shared_ptr <const Place_Target> place_target_,
	   shared_ptr <const Place_Name> place_name_source_,
	   const Place &place_persistent,
	   const Place &place_optional)
	:  place_targets{place_target_},
	   place(place_target_->place),
	   filename(*place_name_source_),
	   redirect_index(-1),
	   is_hardcode(false),
	   is_copy(true)
{
	auto dep= std::make_shared <Plain_Dep>
		(Place_Target(0, *place_name_source_));

	if (! place_persistent.empty()) {
		dep->flags |= F_PERSISTENT;
		dep->places[I_PERSISTENT]= place_persistent;
	}
	if (! place_optional.empty()) {
		dep->flags |= F_OPTIONAL;
		dep->places[I_OPTIONAL]= place_optional;
	}

	deps.push_back(dep);
}

shared_ptr <const Rule>
Rule::instantiate(shared_ptr <const Rule> rule,
		  const std::map <string, string> &mapping)
{
	if (rule->get_parameters().size() == 0)
		return rule;

	std::vector <shared_ptr <const Place_Target> >
		place_targets(rule->place_targets.size());
	for (size_t i= 0; i < rule->place_targets.size(); ++i)
		place_targets[i]= rule->place_targets[i]->instantiate(mapping);

	std::vector <shared_ptr <const Dep> > deps;
	for (auto &dep: rule->deps)
		deps.push_back(dep->instantiate(mapping));

	return std::make_shared <Rule>
		(move(place_targets),
		 move(deps), rule->place, rule->command,
		 move(rule->filename.instantiate(mapping)),
		 rule->is_hardcode, rule->redirect_index,
		 rule->is_copy);
}

void Rule::render(Parts &parts, Rendering rendering) const
{
	parts.append_operator("Rule(");

	bool first= true;
	for (auto place_param_target: place_targets) {
		if (first)
			first= false;
		else
			parts.append_space();
		place_param_target->render(parts, rendering);
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

void Rule::check_unparametrized(shared_ptr <const Dep> dep,
				const std::set <string> &parameters)
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
		for (size_t jj= 0;
		     jj < plain_dep->place_target.place_name.get_n(); ++jj) {
			string parameter= plain_dep->place_target.place_name.get_parameters()[jj];
			if (parameters.count(parameter) == 0) {
				plain_dep->place_target
					.place_name.get_places()[jj] <<
					fmt("parameter %s must not appear in dependency %s",
					    show_prefix("$", parameter),
					    show(plain_dep->place_target));
				if (place_targets.size() == 1) {
					place_targets[0]->place <<
						fmt("because it does not appear in target %s",
						    show(*place_targets[0]));
				} else {
					place << fmt("because it does not appear in any of the targets %s... of the rule",
						     show(*place_targets[0]));
				}
				throw ERROR_LOGICAL;
			}
		}
	} else {
		unreachable();
	}
}

void Rule::canonicalize()
{
	for (size_t i= 0; i < place_targets.size(); ++i) {
		place_targets[i]= ::canonicalize(place_targets[i]);
	}
}

void Rule_Set::add(std::vector <shared_ptr <Rule> > &rules_)
{
	for (auto &rule: rules_) {
		rule->canonicalize();

		/* Check that the rule doesn't have a duplicate target */
		for (size_t i= 0; i < rule->place_targets.size(); ++i) {
			for (size_t j= 0; j < i; ++j) {
				if (*rule->place_targets[i] ==
				    *rule->place_targets[j]) {
					rule->place_targets[i]->place <<
						fmt("there must not be a target %s",
						    show(*rule->place_targets[i]));
					rule->place_targets[j]->place <<
						fmt("shadowing target %s of the same rule",
						    show(*rule->place_targets[j]));
					throw ERROR_LOGICAL;
				}
			}
		}

		/* Add the rule */
		if (! rule->is_parametrized()) {
			for (auto place_param_target: rule->place_targets) {
				Hash_Dep hash_dep= place_param_target->unparametrized();

				if (rules_unparam.count(hash_dep)) {
					place_param_target->place <<
						fmt("there must not be a second rule for target %s",
						    show(hash_dep));
					auto rule_2= rules_unparam.at(hash_dep);
					for (auto place_param_target_2: rule_2->place_targets) {
						assert(place_param_target_2->place_name.get_n() == 0);
						if (place_param_target_2->unparametrized() == hash_dep) {
							place_param_target_2->place <<
								fmt("shadowing previous rule %s",
								    show(hash_dep));
							break;
						}
					}
					throw ERROR_LOGICAL;
				}
				rules_unparam[hash_dep]= rule;
			}
		} else {
			rules_param.insert(rule);

			bool found_bare= false;
			for (auto target: rule->place_targets) {
				const Name &name= target->place_name;
				assert(name.is_parametrized());
				const string prefix= name.get_texts()[0];
				const string suffix= name.get_texts()[name.get_n()];
				if (prefix.empty() && suffix.empty()) {
					found_bare= true;
				}
				if (!prefix.empty()) {
					rules_param_prefix.insert(prefix, rule);
				}
				if (!suffix.empty()) {
					string suffix_inv= suffix;
					reverse_string(suffix_inv);
					rules_param_suffix.insert(suffix_inv, rule);
				}

				/* Set FOUND_BARE when the target starts/end with a
				 * special canonicalization string */

				/* Special rule (a):  Target starts with './' follwed by
				 * a parameter */
				if (prefix == "./")
					found_bare= true;
				/* Special rules (b) and (c):  Target starts
				 * with a parameter, followed by a slash */
				if (prefix.empty() && name.get_texts()[1][0] == '/')
					found_bare= true;
			}

			if (found_bare) {
				rules_param_bare.push_back(rule);
			}
		}
	}
}

shared_ptr <const Rule> Rule_Set::get(Hash_Dep hash_dep,
				      shared_ptr <const Rule> &param_rule,
				      std::map <string, string> &mapping_parameter,
				      const Place &place)
{
	assert(hash_dep.is_file() || hash_dep.is_transient());
	assert((hash_dep.get_front_word() & ~F_TARGET_TRANSIENT) == 0);
	assert(mapping_parameter.size() == 0);

	hash_dep.canonicalize();

	/* Check for an unparametrized rule.  Since we keep them in a
	 * map by target filename(s), there can only be a single matching rule to
	 * begin with.  (I.e., if multiple unparametrized rules for the same
	 * filename exist, then that error is caught earlier when the
	 * Rule_Set is built.)  */
	auto i= rules_unparam.find(hash_dep);
	if (i != rules_unparam.end()) {
		shared_ptr <const Rule> rule= i->second;
		assert(rule != nullptr);
		assert(rule->place_targets.front()->place_name.get_n() == 0);
#ifndef NDEBUG
		/* Check that the target is a target of the found rule */
		bool found= false;
		for (auto place_param_target: rule->place_targets) {
			Hash_Dep t= place_param_target->unparametrized();
			t.canonicalize();
			if (t == hash_dep)
				found= true;
		}
		assert(found);
#endif
		param_rule= rule;
		return rule;
	}

	/*
	 * Parametrized rules
	 */
	Best_Rule_Finder best_rule_finder;

	/* Search the best parametrized rule, if there is an affix in the rule */
	for (auto it= rules_param_prefix.find(hash_dep.get_name_nondynamic());
	     it != rules_param_prefix.end(); ++it) {
		best_rule_finder.add(hash_dep, *it);
	}
	string target_reversed= hash_dep.get_name_nondynamic();
	reverse_string(target_reversed);
	for (auto it= rules_param_suffix.find(target_reversed);
	     it != rules_param_suffix.end(); ++it) {
		best_rule_finder.add(hash_dep, *it);
	}

	/* Search the best parametrized rule, if the rules are affixless */
	for (auto &rule: rules_param_bare) {
		best_rule_finder.add(hash_dep, rule);
	}

	/* No rule matches */
	if (best_rule_finder.count() == 0)
		return nullptr;

	/* More than one rule matches:  error */
	if (best_rule_finder.count() != 1 ) {
		place << fmt("multiple minimal matching rules for target %s",
			     show(hash_dep));
		for (auto &place_param_target:
			     best_rule_finder.targets_best()) {
			place_param_target.second->place <<
				fmt("rule with target %s",
				    show(*place_param_target.second));
		}
		explain_minimal_matching_rule();
		throw ERROR_LOGICAL;
	}

	/* Instantiate the rule */
	shared_ptr <const Rule> rule_best=
		best_rule_finder.rule_best();
	swap(mapping_parameter,
	     best_rule_finder.mapping_best());
	shared_ptr <const Rule> ret
		(Rule::instantiate(best_rule_finder.rule_best(), mapping_parameter));
	param_rule= rule_best;
	return ret;
}

void Rule_Set::print_for_option_dP() const
{
	for (auto i: rules_unparam)  {
		string text= show(i.second, CH_OUT);
		puts(text.c_str());
	}
	for (auto i: rules_param)  {
		string text= show(i, CH_OUT);
		puts(text.c_str());
	}
}

void Rule_Set::print_for_option_I() const
{
	std::set <string> filenames;
	for (auto i: rules_unparam)  {
		const Rule &rule= *i.second;
		if (rule.must_exist())
			continue;
		for (auto target: rule.place_targets) {
			if (target->flags & F_TARGET_TRANSIENT)
				continue;
			filenames.insert(show(target->place_name, S_OPTION_I, R_GLOB));
		}
	}
	for (auto rule: rules_param)  {
		if (rule->must_exist())
			continue;
		for (auto target: rule->place_targets) {
			if (target->flags & F_TARGET_TRANSIENT)
				continue;
			filenames.insert(show(target->place_name, S_OPTION_I, R_GLOB));
		}
	}
	for (const string &filename: filenames) {
		puts(filename.c_str());
	}
}

void Best_Rule_Finder::add(const Hash_Dep &hash_dep, shared_ptr <const Rule> rule)
{
	best_sorted.clear();

	for (auto &place_param_target: rule->place_targets) {
		assert(place_param_target->place_name.get_n() > 0);
		std::map <string, string> mapping;
		std::vector <size_t> anchoring;
		int priority;

		/* The parametrized rule is of another type */
		if (hash_dep.get_front_word() !=
		    (place_param_target->flags & F_TARGET_TRANSIENT))
			continue;

		/* The parametrized rule does not match */
		if (! place_param_target->place_name.match(hash_dep.get_name_nondynamic(),
							   mapping, anchoring, priority))
			continue;

		assert(anchoring.size() == 2 * place_param_target->place_name.get_n());

		size_t k= rules_best.size();
		assert(k == anchorings_best.size());
		assert(k == priorities_best.size());
		assert(k == mappings_best.size());

		/* Check whether the rule is dominated by at least one other
		 * rule; also, avoid inserting the same rule twice (which
		 * happens if the rule was found from both a prefix and a
		 * suffix.)  But note that there can be two parametrized
		 * targets in one rule which both match the target, in which
		 * case we do want to throw a "duplicate rule" error (because
		 * Stu wouldn't know how to chose the parameter), and therefore
		 * we also need to compare anchorings.  */
		for (size_t j= 0; j < k; ++j) {
			if (rule == rules_best[j]
			    && anchoring == anchorings_best[j])
				return;
			if (Name::anchoring_dominates
			    (anchorings_best[j], anchoring,
			     priorities_best[j], priority))
				return;
		}

		/* Check whether the rule dominates all other rules */
		{
			bool is_best= true;
			for (ssize_t j= 0; is_best && j < (ssize_t) k; ++j) {
				if (! Name::anchoring_dominates
				    (anchoring, anchorings_best[j],
				     priority, priorities_best[j]))
					is_best= false;
			}
			if (is_best) {
				k= 0;
			}
		}
		rules_best.resize(k+1);
		mappings_best.resize(k+1);
		anchorings_best.resize(k+1);
		priorities_best.resize(k+1);
		place_targets_best.resize(k+1);
		rules_best[k]= rule;
		swap(mapping, mappings_best[k]);
		swap(anchoring, anchorings_best[k]);
		priorities_best[k]= priority;
		place_targets_best[k]= place_param_target;
	}
}

const std::map <Place, shared_ptr <const Place_Target> > &Best_Rule_Finder::targets_best() const
{
	assert(! place_targets_best.empty());

	if (best_sorted.empty()) {
		for (auto &place_param_target:
			     place_targets_best) {
			best_sorted[place_param_target->place]= place_param_target;
		}
		assert(place_targets_best.size() == best_sorted.size());
	}

	return best_sorted;
}
