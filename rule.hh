#ifndef RULE_HH
#define RULE_HH

/* 
 * Data structures for representing rules. 
 */

#include <unordered_map>

#include "token.hh"
#include "explain.hh"

class Rule
/* A rule.  The class Rule allows parameters; there is no
 * "unparametrized rule" class.  */ 
{
public:
	vector <shared_ptr <const Place_Param_Target> > place_param_targets; 
	/* The targets of the rule, in the order specified in the rule.
	 * Contains at least one element.  Each element contains all
	 * parameters of the rule, and therefore should be used for
	 * iterating over all parameters.  The place in each target is
	 * used when referring to a target specifically.  The targets
	 * may or may not be canonicalized.  */

	vector <shared_ptr <const Dep> > deps;
	/* The dependencies in order of declaration.  Dependencies are
	 * included multiple times if they appear multiple times in the
	 * source.  Any parameter occuring in any dependency also occurs
	 * in every target.  */ 

	const Place place;
	/* The place of the rule as a whole.  Taken from the place of
	 * the first target (but could be different, in principle)  */   

	const shared_ptr <const Command> command;
	/* The command (optional).  Contains its own place, as it is a
	 * token.  Null when the rule does not have a command, i.e.,
	 * ends in a semicolon ';'.  For hardcoded rules, the content of
	 * the file (not optional).  */  

	const Name filename; 
	/* When !is_copy:  The name of the file from which
	 *   input should be read; must be one of the file dependencies.
	 *   Empty for no input redirection.   
	 * When is_copy: the file from which to copy; never empty.  */ 

	const int redirect_index; 
	/* Index within PLACE_PARAM_TARGETS of the target to which
	 * output redirection is applied. -1 if no output redirection is
	 * used. The target with that index is a file target.  */

	const bool is_hardcode;
	/* Whether the command is a command or hardcoded content */ 

	const bool is_copy;
	/* Whether the rule is a copy rule, i.e., declared with '='
	 * followed by a filename. */ 

	Rule(vector <shared_ptr <const Place_Param_Target> > &&place_param_targets,
	     vector <shared_ptr <const Dep> > &&deps_,
	     const Place &place_,
	     const shared_ptr <const Command> &command_,
	     Name &&filename_,
	     bool is_hardcode_,
	     int redirect_index_,
	     bool is_copy_); 
	/* Direct constructor that specifies everything; no checks,
	 * initialization or canonicalization is performed.  */

	Rule(vector <shared_ptr <const Place_Param_Target> > &&place_param_targets_,
	     const vector <shared_ptr <const Dep> > &deps_,
	     shared_ptr <const Command> command_,
	     bool is_hardcode_,
	     int redirect_index_,
	     const Name &filename_input_);
	/* Regular rule:  all cases except copy rules */

	Rule(shared_ptr <const Place_Param_Target> place_param_target_,
	     shared_ptr <const Place_Name> place_name_source_,
	     const Place &place_persistent,
	     const Place &place_optional); 
	/* A copy rule.  When the places are EMPTY, the corresponding
	 * flag is not used. */

	/* Whether the rule is parametrized */ 
	bool is_parametrized() const {
		return place_param_targets.front()->place_name.get_n() != 0; 
	}

	string format_out() const; 
	/* Format the rule, as for the -P or -d options */ 

	void check_unparametrized(shared_ptr <const Dep> dep,
				  const set <string> &parameters);
	/* Print error message and throw a logical error when DEP
	 * contains parameters  */

	const vector <string> &get_parameters() const
	{
		assert(place_param_targets.size() != 0); 
		return place_param_targets.front()->place_name.get_parameters(); 
	}

	static shared_ptr <const Rule> instantiate(shared_ptr <const Rule> rule,
						   const map <string, string> &mapping);
	/* Return the same rule as RULE, but with parameters having been
	 * replaced by the given MAPPING.  
	 * We pass THIS as PARAM_RULE explicitly so we can return it
	 * itself when it is unparametrized.  */ 

	void canonicalize(); 
	/* In-place canonicalization of the rule.  This applies to the
	 * targets of the rule.  Called by Rule_Set::add(). */
};

class Rule_Set
/* A set of parametrized rules */
{
private:
	unordered_map <Target, shared_ptr <const Rule> > rules_unparametrized;
	/* All unparametrized rules by their target.  Rules
	 * with multiple targets are included multiple times, for each
	 * of their targets.  None of the targets has flags set (except
	 * F_TARGET_TARNSIENT of course.)  The targets are
	 * canonicalized, both as keys in this map, as well as in each
	 * Rule.  */ 

	vector <shared_ptr <const Rule> > rules_parametrized;
	/* All parametrized rules. */ 

public:
	void add(vector <shared_ptr <Rule> > &rules_);
	/* Add rules to this rule set.  While adding rules, check for
	 * duplicates, and print and throw a logical error if there is.
	 * If the given rule has duplicate targets, print and throw a
	 * logical error.  */ 

	shared_ptr <const Rule> get(Target target, 
				    shared_ptr <const Rule> &param_rule,
				    map <string, string> &mapping_parameter,
				    const Place &place);
	/* Match TARGET to a rule, and return the instantiated
	 * (non-parametrized) corresponding rule.  TARGET must be
	 * non-dynamic and not have flags (except F_TARGET_TRANSIENT).
	 * MAPPING_PARAMETER must be empty.  Return null when no match
	 * is found.  When a match is found, write the original
	 * (possibly parametrized) rule into PARAM_RULE and the matched
	 * parameters into MAPPING_PARAMETER.  Throws errors, in which
	 * case PARAM_RULE is never set.  PLACE is the place of the
	 * dependency; used in error messages.  */ 

	void print() const;
	/* Print the rule set to standard output, as used by the -P and
	 * -d options */   
};

Rule::Rule(vector <shared_ptr <const Place_Param_Target> > &&place_param_targets_,
	   vector <shared_ptr <const Dep> > &&deps_,
	   const Place &place_,
	   const shared_ptr <const Command> &command_,
	   Name &&filename_,
	   bool is_hardcode_,
	   int redirect_index_,
	   bool is_copy_)
	:  place_param_targets(place_param_targets_),
	   deps(deps_),
	   place(place_),
	   command(command_),
	   filename(filename_),
	   redirect_index(redirect_index_),
	   is_hardcode(is_hardcode_),
	   is_copy(is_copy_)
{  }

Rule::Rule(vector <shared_ptr <const Place_Param_Target> > &&place_param_targets_,
	   const vector <shared_ptr <const Dep> > &deps_,
	   shared_ptr <const Command> command_,
	   bool is_hardcode_,
	   int redirect_index_,
	   const Name &filename_)
	:  place_param_targets(place_param_targets_), 
	   deps(deps_),
  	   place(place_param_targets_[0]->place),
	   command(command_),
	   filename(filename_),
	   redirect_index(redirect_index_),
	   is_hardcode(is_hardcode_),
	   is_copy(false)
{ 
	assert(place_param_targets.size() != 0); 
	assert(redirect_index>= -1);
	assert(redirect_index < (ssize_t) place_param_targets.size());
	if (redirect_index >= 0) {
		assert((place_param_targets[redirect_index]->flags & F_TARGET_TRANSIENT) == 0); 
	}

	/* Check that all dependencies only include
	 * parameters from the target */ 
	set <string> parameters;
	for (auto &parameter:  get_parameters()) {
		parameters.insert(parameter); 
	}

	/* Check that only valid parameters are used */ 
	for (const auto &d:  deps) {
		d->check(); 
		check_unparametrized(d, parameters);
	}
}

Rule::Rule(shared_ptr <const Place_Param_Target> place_param_target_,
	   shared_ptr <const Place_Name> place_name_source_,
	   const Place &place_persistent,
	   const Place &place_optional)
	:  place_param_targets{place_param_target_},
	   place(place_param_target_->place),
	   filename(*place_name_source_),
	   redirect_index(-1),
	   is_hardcode(false),
	   is_copy(true)
{
	auto dep= make_shared <Plain_Dep> 
		(Place_Param_Target(0, *place_name_source_));

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
		  const map <string, string> &mapping) 
{
	/* The rule is unparametrized -- return it */ 
	if (rule->get_parameters().size() == 0) {
		return rule;
	}

	vector <shared_ptr <const Place_Param_Target> > place_param_targets(rule->place_param_targets.size());
	for (size_t i= 0;  i < rule->place_param_targets.size();  ++i) 
		place_param_targets[i]= rule->place_param_targets[i]->instantiate(mapping);

	vector <shared_ptr <const Dep> > deps;
	for (auto &dep:  rule->deps) {
		deps.push_back(dep->instantiate(mapping));
	}

	return make_shared <Rule> 
		(move(place_param_targets),
		 move(deps),
		 rule->place,
		 rule->command,
		 move(rule->filename.instantiate(mapping)),
		 rule->is_hardcode,
		 rule->redirect_index,
		 rule->is_copy); 
}

string Rule::format_out() const
{
	string ret;

	ret += "Rule(";
	
	bool first= true;
	for (auto place_param_target:  place_param_targets) {
		if (first)
			first= false;
		else
			ret += ' '; 
		ret += place_param_target->format_out(); 
	}

	if (deps.size() != 0)
		ret += ": ";
	for (auto i= deps.begin();  i != deps.end();  ++i) {
		if (i != deps.begin())
			ret += ", ";
		ret += (*i)->format_out(); 
	}

	ret += ")";

	return ret; 
}

void Rule::check_unparametrized(shared_ptr <const Dep> dep,
				const set <string> &parameters)
{
	assert(dep != nullptr); 

	if (auto dynamic_dep= to <const Dynamic_Dep> (dep)) {
		check_unparametrized(dynamic_dep->dep, parameters); 
	} else if (auto compound_dep= to <const Compound_Dep> (dep)) {
		for (const auto &d:  compound_dep->deps) {
			check_unparametrized(d, parameters); 
		}
	} else if (auto concat_dep= to <const Concat_Dep> (dep)) {
		for (const auto &d:  concat_dep->deps) {
			check_unparametrized(d, parameters); 
		}
	} else if (auto plain_dep= to <const Plain_Dep> (dep)) {
		for (size_t jj= 0;  jj < plain_dep->place_param_target.place_name.get_n();  ++jj) {
			string parameter= plain_dep->place_param_target.place_name.get_parameters()[jj]; 
			if (parameters.count(parameter) == 0) {
				plain_dep->place_param_target
					.place_name.get_places()[jj] <<
					fmt("parameter %s must not appear in dependency %s", 
					    prefix_format_err(parameter, "$"),
					    plain_dep->place_param_target.format_err());
				if (place_param_targets.size() == 1) {
					place_param_targets[0]->place <<
						fmt("because it does not appear in target %s",
						    place_param_targets[0]->format_err());
				} else {
					place << fmt("because it does not appear in any of the targets %s... of the rule",
						     place_param_targets[0]->format_err()); 
				}
				throw ERROR_LOGICAL; 
			}
		}
	} else {
		assert(false); 
	}
}

void Rule::canonicalize()
{
	for (size_t i= 0;  i < place_param_targets.size();  ++i) {
		place_param_targets[i]= ::canonicalize(place_param_targets[i]); 
	}
}

void Rule_Set::add(vector <shared_ptr <Rule> > &rules_) 
{
	for (auto &rule:  rules_) {

		rule->canonicalize(); 
		
		/* Check that the rule doesn't have a duplicate target */ 
		for (size_t i= 0;  i < rule->place_param_targets.size();  ++i) {
			for (size_t j= 0;  j < i;  ++j) {
				if (*rule->place_param_targets[i] ==
				    *rule->place_param_targets[j]) {
					rule->place_param_targets[i]->place << 
						fmt("there must not be a target %s",
						    rule->place_param_targets[i]->format_err()); 
					rule->place_param_targets[j]->place << 
						fmt("shadowing target %s of the same rule",
						    rule->place_param_targets[j]->format_err()); 
					throw ERROR_LOGICAL; 
				}
			}
		}

		/* Add the rule */ 
		if (! rule->is_parametrized()) {
			for (auto place_param_target:  rule->place_param_targets) {
				Target target= place_param_target->unparametrized(); 

				if (rules_unparametrized.count(target)) {
					place_param_target->place <<
						fmt("there must not be a second rule for target %s", 
						    target.format_err());
					auto rule_2= rules_unparametrized.at(target); 
					for (auto place_param_target_2: rule_2->place_param_targets) {
						assert(place_param_target_2->place_name.get_n() == 0);
						if (place_param_target_2->unparametrized() == target) {
							place_param_target_2->place << 
								fmt("shadowing previous rule %s", 
								    target.format_err());  
							break;
						}
					}
					throw ERROR_LOGICAL; 
				}
				rules_unparametrized[target]= rule;
			}
		} else {
			rule->canonicalize(); 
			rules_parametrized.push_back(rule); 
		}
	}
}

shared_ptr <const Rule> Rule_Set::get(Target target, 
				      shared_ptr <const Rule> &param_rule,
				      map <string, string> &mapping_parameter,
				      const Place &place)
{
	assert(target.is_file() || target.is_transient()); 
	assert((target.get_front_word() & ~F_TARGET_TRANSIENT) == 0); 
	assert(mapping_parameter.size() == 0); 

	target.canonicalize(); 

	/* Check for an unparametrized rule.  Since we keep them in a
	 * map by target filename(s), there can only be a single matching rule to
	 * begin with.  (I.e., if multiple unparametrized rules for the same
	 * filename exist, then that error is caught earlier when the
	 * Rule_Set is built.)  */ 
	auto i= rules_unparametrized.find(target);
	if (i != rules_unparametrized.end()) {
		shared_ptr <const Rule> rule= i->second;
		assert(rule != nullptr); 
		assert(rule->place_param_targets.front()->place_name.get_n() == 0);
#ifndef NDEBUG		
		/* Check that the target is a target of the found
		 * rule, as it should be */
		bool found= false;
		for (auto place_param_target:  rule->place_param_targets) {
			Target t= place_param_target->unparametrized();
			t.canonicalize(); 
			if (t == target)
				found= true;
		}
		assert(found); 
#endif 

		param_rule= rule; 
		return rule;
	}

	/* Search the best parametrized rule.  Since this implementation
	 * does not have an index for parametrized rules, we simply
	 * check all rules, and choose the best-fitting one.  This can
	 * be optimized, but the optimization is not trivial.  */ 

	/* Element [0] corresponds to the best rule. */ 
	vector <shared_ptr <const Rule> > rules_best;
	vector <map <string, string> > mappings_best; 
	vector <vector <size_t> > anchorings_best;
	vector <int> priorities_best;
	vector <shared_ptr <const Place_Param_Target> > place_param_targets_best; 

	for (auto &rule:  rules_parametrized) {

		for (auto &place_param_target:  rule->place_param_targets) {

			assert(place_param_target->place_name.get_n() > 0);
		
			map <string, string> mapping;
			vector <size_t> anchoring;
			int priority;

			/* The parametrized rule is of another type */ 
			if (target.get_front_word() !=
			    (place_param_target->flags & F_TARGET_TRANSIENT))
				continue;

			/* The parametrized rule does not match */ 
			if (! place_param_target->place_name.match(target.get_name_nondynamic(),
								   mapping, anchoring, priority))
				continue; 

			assert(anchoring.size() == 
			       (2 * place_param_target->place_name.get_n())); 

			size_t k= rules_best.size(); 
			assert(k == anchorings_best.size()); 
			assert(k == priorities_best.size());
			assert(k == mappings_best.size());

			/* Check whether the rule is dominated by at least one other rule */
			for (size_t j= 0;  j < k;  ++j) {
				if (Name::anchoring_dominates
				    (anchorings_best[j], anchoring,
				     priorities_best[j], priority)) {
					goto dont_add;
				}
			}

			/* Check whether the rule dominates all other rules */ 
			{
				bool is_best= true;
				for (ssize_t j= 0;  is_best && j < (ssize_t) k;  ++j) {
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
			place_param_targets_best.resize(k+1); 
			rules_best[k]= rule;
			swap(mapping, mappings_best[k]);
			swap(anchoring, anchorings_best[k]);
			priorities_best[k]= priority; 
			place_param_targets_best[k]= place_param_target;
		dont_add:;
		}
	}

	/* No rule matches */ 
	if (rules_best.size() == 0) {
		return nullptr; 
	}

	/* More than one rule matches:  error */ 
	if (rules_best.size() > 1) {
		place << fmt("multiple minimal matching rules for target %s", target.format_err());
		for (auto &place_param_target:  place_param_targets_best) {
			place_param_target->place <<
				fmt("rule with target %s", 
				    place_param_target->format_err()); 
		}
		explain_minimal_matching_rule(); 
		throw ERROR_LOGICAL; 
	}
	assert(rules_best.size() == 1); 

	/* Instantiate the rule */ 
	shared_ptr <const Rule> rule_best= rules_best[0];
	swap(mapping_parameter, mappings_best[0]); 
	shared_ptr <const Rule> ret(Rule::instantiate(rule_best, mapping_parameter));
	param_rule= rule_best; 
	return ret;
}

void Rule_Set::print() const
{
	for (auto i:  rules_unparametrized)  {
		string text= i.second->format_out(); 
		puts(text.c_str()); 
	}

	for (auto i:  rules_parametrized)  {
		string text= i->format_out(); 
		puts(text.c_str()); 
	}
}

#endif /* ! RULE_HH */
