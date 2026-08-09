#ifndef RULE_HH
#define RULE_HH

#include <memory>
#include <set>
#include <unordered_map>
#include <unordered_set>

#include "base.hh"
#include "dep.hh"
#include "place.hh"
#include "preset.hh"
#include "token.hh"

typedef unsigned Target_Index;
inline constexpr Target_Index TARGET_INDEX_NONE= std::numeric_limits <Target_Index> ::max();

class Rule
	: public std::enable_shared_from_this <Rule>
/*
 * The class Rule allows parameters; there is no "unparametrized rule" class.
 *
 * The following four fields contain names to which the base directory is eventually
 * prepended:
 *   - TARGETS:       In constructor
 *   - DEPS:          In instantiate()
 */
{
public:
	std::vector <shared_ptr <const Plain_Dep> > targets;
	/* The targets of the rule, in the order specified in the rule.  Contains at least
	 * one element.  Each element contains all parameters of the rule, and therefore
	 * should be used for iterating over all parameters.  The place in each target is
	 * used when referring to a target specifically.  The targets may or may not be
	 * canonicalized. */

	std::vector <shared_ptr <const Dep> > deps;
	/* The dependencies in order of declaration.  Dependencies are included multiple
	 * times if they appear multiple times in the source.  Any parameter occuring in
	 * a dependency must occur in every target. */

	const Place place;
	/* The place of the rule as a whole.  Taken from the place of the first target
	 * (but could be different, in principle). */

	const shared_ptr <const Command> command;
	/* The command.  Contains its own place, as it is a token.  Null when the rule
	 * does not have a command, i.e., ends in a semicolon ';'.  For content rules, the
	 * content of the file (not optional). */

	const string base_dir;
	/* The base directory is also contained in TARGETS and DEPS.  Empty when no cd
	 * needed. */
	
	const Placed_Name name_input;
	/* Unparametrized.  Not rebased.  When !is_copy: The name of the file from which
	 * input should be read; must be one of the file dependencies.  Empty for no input
	 * redirection.  When is_copy: the file from which to copy; never empty. */

	const Placed_Name name_output;
	/* Unpametrized.  Not rebased.  Empty for no output redirection. */

	const bool is_content;
	/* The rule is content rule; i.e., the command represents the content, not an
	 * actual command. */

	const Placed_Name copy_src, copy_dst;
	/* Empty if not a copy rule.  Not rebased. */

	Rule(
		std::vector <shared_ptr <const Plain_Dep> > &&targets_,
		const std::vector <shared_ptr <const Dep> > &deps_,
		shared_ptr <const Command> command_,
		string base_dir,
		bool is_content_,
		const Placed_Name &name_input_,
		const Placed_Name &name_output_);
	/* Regular rule:  all cases except copy rules */

	Rule(
		shared_ptr <const Plain_Dep> target_,
		shared_ptr <const Placed_Name> copy_src_,
		string base_dir,
		const Place &place_persistent,
		const Place &place_optional);
	/* A copy rule.  When the places are EMPTY, the corresponding flag is not used. */

	Rule(
		std::vector <shared_ptr <const Plain_Dep> > &&targets_,
		std::vector <shared_ptr <const Dep> > &&deps_,
		const Place &place_,
		const shared_ptr <const Command> &command_,
		string base_dir_,
		const Placed_Name &name_input_,
		const Placed_Name &name_output_,
		bool is_content_,
		const Placed_Name &copy_src_,
		const Placed_Name &copy_dst_);
	/* Direct constructor that specifies everything; no checks, initialization or
	 * canonicalization is performed. */

	bool is_parametrized() const;
	bool is_copy() const { return ! copy_src.empty(); }

	/* A rule in which the targets must exist */
	bool must_exist() const {
		return command == nullptr && ! is_content && ! is_copy();
	}

	void render(Parts &, Rendering= 0) const;
	/* Format the rule, as for the -P option */

	void check_unparametrized(
		shared_ptr <const Dep> dep,
		const std::set <string> &parameters) const;
	/* Print error message and throw a logical error when DEP contains parameters */

	void check_duplicate_target() const;
	const std::vector <string> &get_parameters() const;

	void canonicalize();
	/* In-place canonicalization of the rule.  This applies to the targets of the
	 * rule. */

	shared_ptr <const Rule> instantiate(const std::map <string, string> &mapping) const;
	shared_ptr <const Rule> rebase() const;

private:
	// TODO rename rebase
	shared_ptr <const Dep> base(shared_ptr <const Dep> d) const {
		return ::rebase(d, base_dir);
	}
};

void render(shared_ptr <const Rule>, Parts &, Rendering= 0);

class Rule_Set
/* A set of rules.  They can be both parametrized and unparametrized. */
{
public:
	void add(std::vector <shared_ptr <Rule> > &);
	/* Add rules to this rule set.  While adding rules, check for duplicates, and
	 * print and throw a logical error if there is.  If a given rule has duplicate
	 * targets, print and throw a logical error. */

	shared_ptr <const Rule> get(
		Hash_Dep hash_dep,
		shared_ptr <const Rule> &param_rule,
		std::map <string, string> &mapping_parameter,
		const Place &place,
		shared_ptr <const Plain_Dep> &target_plain_dep,
		Target_Index &target_index);
	/* Match HASH_DEP to a rule, and return the instantiated (non-parametrized)
	 * corresponding rule.  TARGET must be non-dynamic and not have flags (except
	 * F_TARGET_PHONY).  MAPPING_PARAMETER must be empty.  Return null when no
	 * match is found.  When a match is found, write the original (possibly
	 * parametrized) rule into PARAM_RULE and the matched parameters into
	 * MAPPING_PARAMETER.  Throws errors, in which case PARAM_RULE is never set.
	 * PLACE is the place of the dependency; used in error messages. */

	void print_for_option_P() const;
	void print_for_option_I() const;

private:
	std::unordered_map <Hash_Dep, std::pair <Target_Index, shared_ptr <const Rule> > >
		rules_unparam;
	/* All unparametrized rules by their targets.  Rules with multiple targets are
	 * included multiple times, for each * of their targets.  None of the targets has
	 * flags set (except * F_TARGET_TARNSIENT.)  The targets are canonicalized, both
	 * as keys in this map, * as well as in each Rule. */

	std::unordered_set <shared_ptr <const Rule> > rules_param;
	/* All parametrized rules.  Each parametrized rule is here, and in one more of the
	 * containers below.  This variable is only needed for printing the rule. */

	Preset <std::pair <Target_Index, shared_ptr <const Rule> > >
		rules_param_prefix, rules_param_suffix;
	/* All parametrized rules that have a target with a prefix/suffix, stored by each
	 * of their affixes.  In SUFFIX, everything is reversed, so access must use
	 * reversed strings. */

	std::vector <std::pair <Target_Index, shared_ptr <const Rule> > > rules_param_bare;
	/* All parametrized rules where at least one target is affixless, or in which
	 * there is an affix which, due to special canonicalization rules (see manpage),
	 * is not present in a matched string. */

	void add_unparametrized_rule(shared_ptr <Rule>);
	void add_parametrized_rule(shared_ptr <Rule>);
};

class Found_Rule
{
public:
	shared_ptr <const Rule> rule;
	std::map <string, string> mapping;
	std::vector <size_t> anchoring;
	int priority;
	shared_ptr <const Plain_Dep> target;
	Target_Index target_index;

	bool operator<(const Found_Rule &) const;
};

class Best_Rule_Finder
{
public:
	void check(const Hash_Dep &, shared_ptr <const Rule> , Target_Index);
	size_t count() const { return found_rules.size(); }

	/* Access the best rule.  The best rule must be unique. */
	const Found_Rule &best() {
		assert(found_rules.size() == 1);
		return * found_rules.begin();
	}

	const std::set <Found_Rule> &all_best() const { return found_rules; }

private:
	std::set <Found_Rule> found_rules;
};

#endif /* ! RULE_HH */
