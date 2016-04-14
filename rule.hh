#ifndef RULE_HH
#define RULE_HH

/* Data structures for representing rules. 
 */

#include <unordered_map>

#include "dependency.hh"

/* A rule.  The class Rule allows parameters; there is no "unparametrized rule" class.  
 */ 
class Rule
{
public:

	/* The target of the rule.  This contains all parameters of the rule
	 * and therefore should be used for iterating of all parameters.
	 * The place in this target is used when referring to the target
	 * specifically.  To refer to the rule as a whole, PLACE is used. 
	 */ 
	Place_Param_Target place_param_target; 

	/* The dependencies in order of declaration.  Dependencies are
	 * included multiple times if they appear multiple times in the
	 * source.  All parameters occuring in the dependencies also
	 * occur in the target. */ 
	vector <shared_ptr <Dependency> > dependencies;

	/* The place of the rule as a whole.  Taken from the place of the
	 * target (but could be different, in principle) */ 
	Place place;

	/* The command (optional).  Contains its own place, as it is a
	 * token. */ 
	shared_ptr <Command> command;

	/* Whether output is to be redirected.  Only TRUE if target is a file.
	 * The output redirection is always to the target file, so no need
	 * to save it here. 
	 */ 
	bool redirect_output; 

	/* The name of the file from which input should be read; must be one
	 * of the file dependencies.  Empty for no input redirection. 
	 */ 
	Param_Name filename_input; 

	Rule(shared_ptr <Place_Param_Target> place_param_target_,
	     const vector <shared_ptr <Dependency> > &dependencies_,
	     shared_ptr <Command> command_,
	     bool redirect_output_,
	     const Param_Name &filename_input_);

	/* Format the rule, as in the -p option */ 
	string format() const; 

	/* Return the same rule, but without parameters.  
	 * We pass THIS as PARAM_RULE explicitly in order for it to be
	 * returned itself. 
	 */ 
	static shared_ptr <Rule> instantiate(shared_ptr <Rule> param_rule,
					     const map <string, string> &mapping);
};

/* A set of parametrized rules
 */
class Rule_Set
{
private:

	/* All unparametrized general rules by their target. */ 
	unordered_map <Target, shared_ptr <Rule> > rules_unparametrized;

	/* All parametrized general rules. */ 
	vector <shared_ptr <Rule> > rules_parametrized;

public:

	/* Add rules to the rule set.  
	 * While adding rules, check for duplicates. 
	 */ 
	void add(vector <shared_ptr <Rule> > &rules_);

	/* Match TARGET to a rule, and return the instantiated
	 * (unparametrized) corresponding rule.  
	 * Return nullptr when no match was found. 
	 * The used original rule is written into ORIGINAL_RULE when a
	 * match is found.  The matched parameters are stored in
	 * MAPPING_OUT. 
	 */ 
	shared_ptr <Rule> get(Target target, 
			      shared_ptr <Rule> &rule_original,
			      map <string, string> &mapping_out);

	/* Print the rule set to standard output, as used in the -p option */  
	void print() const;
};

Rule::Rule(shared_ptr <Place_Param_Target> place_param_target_,
	   const vector <shared_ptr <Dependency> > &dependencies_,
	   shared_ptr <Command> command_,
	   bool redirect_output_,
	   const Param_Name &filename_input_)
	:  place_param_target(*place_param_target_), 
	   dependencies(dependencies_),
	   command(command_),
	   redirect_output(redirect_output_),
	   filename_input(filename_input_)
{ 
	/* The place of the rule as a whole is the same as the place of the
	 * target. */
	place= place_param_target.place; 

	/* Check that all dependencies only include
	 * parameters from the target */ 

	unordered_set <string> parameters;
	for (auto &i:  place_param_target.place_param_name.get_parameters()) {
		parameters.insert(i); 
	}

	/* Check that only valid parameters are used */ 
	for (auto &i:  dependencies) {

		shared_ptr <Dependency> dep= i;
		while (dynamic_pointer_cast <Dynamic_Dependency> (dep)) {
			dep= dynamic_pointer_cast <Dynamic_Dependency> (dep)->dependency;
		}

		if (dynamic_pointer_cast <Direct_Dependency> (dep)) {

			shared_ptr <Direct_Dependency> dependency= 
				dynamic_pointer_cast <Direct_Dependency> (dep); 

			for (unsigned jj= 0;  
				 jj < dependency->place_param_target.place_param_name.get_n();  ++jj) {
				string parameter= dependency->place_param_target
					.place_param_name.get_parameters()[jj]; 
				if (parameters.count(parameter) == 0) {
					dependency->place_param_target.place_param_name.get_places()[jj] <<
						fmt("parameter $%s is not used", parameter); 
					place_param_target.place <<
						fmt("in target %s", place_param_target.format());
					throw ERROR_LOGICAL; 
				}
			}
		} else {
			assert(0); 
		}
	}
}

shared_ptr <Rule> 
Rule::instantiate(shared_ptr <Rule> rule,
		  const map <string, string> &mapping) 
{
	if (rule->place_param_target.place_param_name.get_n() == 0) {
		return rule;
	}

	vector <shared_ptr <Dependency> > dependencies;

	for (auto &i:  rule->dependencies) {
		dependencies.push_back(i->instantiate(mapping));
	}

	shared_ptr <Rule> ret
		(new Rule(rule->place_param_target.instantiate(mapping),
			  dependencies,
			  rule->command,
			  rule->redirect_output,
			  rule->filename_input.instantiate(mapping)));

	return ret; 
}

string Rule::format() const
{
	string ret;

	string text_target= place_param_target.format(); 
	ret += fmt("Rule(%s:  ", text_target); 
	for (auto i= dependencies.begin();  i != dependencies.end();  ++i) {
		if (i != dependencies.begin())
			ret += ", ";
		ret += (*i)->format(); 
	}
	ret += ")";

	return ret; 
}

shared_ptr <Rule> Rule_Set::get(Target target, 
				shared_ptr <Rule> &rule_original,
				map <string, string> &mapping_out)
{
	assert(target.type == T_FILE || target.type == T_PHONY); 
	assert(mapping_out.size() == 0); 

	/* Check for an unparametrized rule.  Since we keep them in a
	 * map by filename, there can only be a single matching rule to
	 * begin with.  (I.e., if multiple unparametrized rules for the same
	 * filename exist, then that error is catched earlier when the
	 * File_Rule_Set is built.)
	 */ 
	if (rules_unparametrized.count(target)) {

		shared_ptr <Rule> rule= rules_unparametrized.at(target);
		assert(rule->place_param_target.place_param_name.get_n() == 0);
		assert(rule->place_param_target.place_param_name.unparametrized() 
		       == target.name); 

		rule_original= rule; 
		return rule;
	}

	/* Search the best parametrized rule.  Since this implementation
	 * does not have an index for parametrized rules, we simply
	 * check all rules, and choose the best-fitting one. 
	 */ 

	/* Element [0] corresponds to the best rule whose information is
	 * saved in the two previous variables.  Subsequent rules have
	 * the same number of parameters. */ 
	vector <shared_ptr <Rule> > rules_best;
	vector <map <string, string> > mappings_best; 
	vector <vector <int> > anchorings_best; 

	for (auto &i:  rules_parametrized) {

		assert(i->place_param_target.place_param_name.get_n() > 0);
		
		map <string, string> mapping;
		vector <int> anchoring;

		/* The parametrized rule is of another type */ 
		if (target.type != i->place_param_target.type)
			continue;

		/* The parametrized rule does not match */ 
		if (! i->place_param_target.place_param_name
			.match(target.name, mapping, anchoring))
			continue; 

		assert(anchoring.size() == 
			   (2 * i->place_param_target.place_param_name.get_n())); 

		size_t k= rules_best.size(); 
		
		assert(k == anchorings_best.size()); 
		assert(k == mappings_best.size()); 

		/* Check whether the rule is dominated by at least one other rule */
		for (int j= 0;  j < (ssize_t) k;  ++j) {
			if (Param_Name::anchoring_dominates(anchorings_best[j], anchoring)) 
				goto dont_add;
		}

		/* Check whether the rule dominates all other rules */ 
		{
			bool is_best= true;
			for (int j= 0;  is_best && j < (ssize_t) k;  ++j) {
				if (! Param_Name::anchoring_dominates(anchoring, anchorings_best[j]))
					is_best= false;
			}
			if (is_best) {
				k= 0;
			}
		} 
		rules_best.resize(k+1); 
		mappings_best.resize(k+1);
		anchorings_best.resize(k+1); 
		rules_best[k]= i;
		swap(mapping, mappings_best[k]);
		swap(anchoring, anchorings_best[k]); 
	dont_add:;
	}

	/* No rule matches */ 
	if (rules_best.size() == 0) {
		assert(rules_best.size() == 0); 
		return shared_ptr <Rule> ();
	}

	assert(rules_best.size() >= 1);

	/* More than one rule matches:  error */ 
	if (rules_best.size() > 1) {
		print_error(fmt("Multiple minimal rules for target %s", 
				target.format())); 
		for (auto &i:  rules_best) {
			i->place <<
				fmt("rule with target %s", 
				    i->place_param_target.format()); 
		}
		throw ERROR_LOGICAL; 
	}

	/* Instantiate */ 
	assert(rules_best.size() == 1); 
	shared_ptr <Rule> rule_best= rules_best.at(0);
	mapping_out= mappings_best.at(0); 

	rule_original= rule_best; 

	shared_ptr <Rule> ret
		(Rule::instantiate(rule_best, mapping_out));
		
	return ret;
}

void Rule_Set::add(vector <shared_ptr <Rule> > &rules_) 
{
	for (auto &i:  rules_) {

		shared_ptr <Rule> rule= i;

		if (rule->place_param_target.place_param_name.get_n() == 0) {
			Target target= rule->place_param_target.unparametrized(); 
			if (rules_unparametrized.count(target)) {
				rule->place <<
					fmt("duplicate rule for %s", target.format());
				rules_unparametrized.at(target)->place <<
					"previous definition";
				throw ERROR_LOGICAL; 
			}
			rules_unparametrized[target]= rule;
		} else {

			rules_parametrized.push_back(rule); 
		}
	}
}

void Rule_Set::print() const
{
	for (auto i:  rules_unparametrized)  {
		string text= i.second->format(); 
		puts(text.c_str()); 
	}

	for (auto i:  rules_parametrized)  {
		string text= i->format(); 
		puts(text.c_str()); 
	}
}

#endif /* ! RULE_HH */
