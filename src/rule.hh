#ifndef RULE_HH
#define RULE_HH

#include <set>
#include <unordered_map>
#include <unordered_set>

#include "preset.hh"
#include "token.hh"

class Rule
/* A rule.  The class Rule allows parameters; there is no "unparametrized rule"
 * class.  */
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
	 * followed by a filename.  */

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
/* A set of rules.  They can be both parametrized and unparametrized. */
{
public:
	void add(vector <shared_ptr <Rule> > &);
	/* Add rules to this rule set.  While adding rules, check for
	 * duplicates, and print and throw a logical error if there is.  If a
	 * given rule has duplicate targets, print and throw a logical error.  */

	shared_ptr <const Rule> get(Target target,
				    shared_ptr <const Rule> &param_rule,
				    map <string, string> &mapping_parameter,
				    const Place &place);
	/* Match TARGET to a rule, and return the instantiated
	 * (non-parametrized) corresponding rule.  TARGET must be non-dynamic
	 * and not have flags (except F_TARGET_TRANSIENT).  MAPPING_PARAMETER
	 * must be empty.  Return null when no match is found.  When a match is
	 * found, write the original (possibly parametrized) rule into
	 * PARAM_RULE and the matched parameters into MAPPING_PARAMETER.  Throws
	 * errors, in which case PARAM_RULE is never set.  PLACE is the place of
	 * the dependency; used in error messages.  */

	void print() const;
	/* Print the rule set to standard output, as used by the -P and
	 * -d options.  */

private:
	unordered_map <Target, shared_ptr <const Rule> > rules_unparam;
	/* All unparametrized rules by their target.  Rules with multiple
	 * targets are included multiple times, for each of their targets.  None
	 * of the targets has flags set (except F_TARGET_TARNSIENT.)  The
	 * targets are canonicalized, both as keys in this map, as well as in
	 * each Rule.  */

	unordered_set <shared_ptr <const Rule> > rules_param;
	/* All parametrized rules.  Each parametrized rule is here, and in one
	 * more of the containers below.  */

	Preset <shared_ptr <const Rule> > rules_param_prefix, rules_param_suffix;
	/* All parametrized rules that have a target with a prefix/suffix,
	 * stored by each of their affixes.  In SUFFIX, everything is reversed,
	 * so access must use reversed strings.  */

	vector <shared_ptr <const Rule> > rules_param_bare;
	/* All parametrized rules where at least one target is affixless, or in
	 * which there is an affix which, due to special canonicalization rules
	 * (see manpage), is not present in a matched string.  */
};

class Best_Rule_Finder
{
public:
	void add(const Target &, shared_ptr <const Rule> );
	size_t count() const {
		assert(rules_best.size() == place_param_targets_best.size());
		return rules_best.size();
	}

	/* Return all equally best targets.  They are returned as a map by
	 * place, so iterating over them will give a consistent order, for
	 * consistent error messages.  */
	const map <Place, shared_ptr <const Place_Param_Target> > &targets_best() const;

	/* Access the best rule.  The best rule must be unique.  */
	const shared_ptr <const Rule> &rule_best() const {
		assert(rules_best.size() == 1);
		return rules_best[0];
	}
	map <string, string> &mapping_best() {
		assert(rules_best.size() == 1);
		return mappings_best[0];
	}

private:
	/* Element [0] corresponds to the best rule. */
	vector <shared_ptr <const Rule> > rules_best;
	vector <map <string, string> > mappings_best;
	vector <vector <size_t> > anchorings_best;
	vector <int> priorities_best;
	vector <shared_ptr <const Place_Param_Target> > place_param_targets_best;

	/* The same as PLACE_PARAM_TARGETS_BEST, but sorted by place.  Filled on
	 * demand.  */
	mutable map <Place, shared_ptr <const Place_Param_Target> > best_sorted;
};

#endif /* ! RULE_HH */
