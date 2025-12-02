#include "transitive_executor.hh"

Transitive_Executor::Transitive_Executor(
	shared_ptr <const Dep> dep_link,
	Executor *parent,
	shared_ptr <const Rule> rule_,
	shared_ptr <const Rule> param_rule_,
	std::map <string, string> &mapping_parameter_,
	int &error_additional)
	: Executor(param_rule_), rule(rule_)
{
	TRACE_FUNCTION();
	swap(mapping_parameter, mapping_parameter_);

	assert(to <Plain_Dep> (dep_link));
	shared_ptr <const Plain_Dep> plain_dep= to <Plain_Dep> (dep_link);

	Hash_Dep hash_dep= plain_dep->place_target.unparametrized();
	assert(hash_dep.is_phony());

	if (rule == nullptr)
		hash_deps.push_back(dep_link->get_target());

	parents[parent]= dep_link;
	if (error_additional) {
		*this << "";
		done.set_all();
		parents.erase(parent);
		raise(error_additional);
		return;
	}

	if (rule == nullptr) {
		/* There must be a rule for phony targets (as opposed to file
		 * targets), so this is an error. */
		done.set_all();
		*this << fmt("no rule to build %s", show(hash_dep));
		parents.erase(parent);
		error_additional |= ERROR_BUILD;
		raise(ERROR_BUILD);
		return;
	}

	for (auto &place_param_target: rule->place_targets) {
		hash_deps.push_back(place_param_target->unparametrized());
	}
	assert(hash_deps.size());

	assert((param_rule == nullptr) == (rule == nullptr));

	/* Fill EXECUTORS_BY_TARGET with all targets from the rule, not just the one given
	 * in the dependency.  Also, add the flags. */
	for (Hash_Dep t: hash_deps) {
		t.get_front_word_nondynamic() |= (word_t)
			(dep_link->flags & (F_WORD & ~F_TARGET_DYNAMIC));
		executors_by_hash_dep[t]= this;
	}

	for (auto &dep: rule->deps) {
		push(prepare(dep, dep_link));
	}

	TRACE("Check cycle");
	parents.erase(parent);
	if (Cycle::find(parent, this, dep_link)) {
		TRACE("Rule-level but not file-level cycle found");
		raise(ERROR_LOGICAL);
		error_additional |= ERROR_LOGICAL;
		return;
	}
	parents[parent]= dep_link;
}

Transitive_Executor::~Transitive_Executor()
/* Objects of this type are never deleted */
{
	unreachable();
}

bool Transitive_Executor::want_delete() const
{
	return false;
}

Proceed Transitive_Executor::execute(shared_ptr <const Dep> dep_link)
/* This is an example of a "plain" execute() function, containing the minimal wrapper
 * around execute_phase_{A,B}() */
{
	TRACE_FUNCTION(show_trace(dep_link));
	TRACE("done= %s", done.show());

	Proceed proceed_A= execute_phase_A(dep_link);
	assert(is_valid(proceed_A));
	if (proceed_A) {
		TRACE("Phase A wait / call again");
		return proceed_A;
	}
	if (error) {
		TRACE("Phase A abort");
		done |= Done::from_flags(dep_link->flags);
		return P_NOTHING;
	}

	done |= Done::from_flags(dep_link->flags & (F_PERSISTENT | F_OPTIONAL));
	if (finished(dep_link->flags)) {
		done |= Done::from_flags(dep_link->flags);
		return P_NOTHING;
	}

	Proceed proceed_B= execute_phase_B(dep_link);
	TRACE("proceed_B= %s", show(proceed_B));
	assert(is_valid(proceed_B));
	if (proceed_B) {
		TRACE("Phase B wait / call again");
		return proceed_B;
	}

	done |= Done::from_flags(dep_link->flags);
	return P_NOTHING;
}

bool Transitive_Executor::finished(Flags flags) const
{
	TRACE_FUNCTION();
	TRACE("flags= %s", show(flags));
	bool ret= done.is_done_from_flags(flags);
	TRACE("ret= %s", frmt("%d", ret));
	return ret;
}

#ifndef NDEBUG
void Transitive_Executor::render(Parts &parts, Rendering rendering) const
{
	assert(hash_deps.size());
	return hash_deps.front().render(parts, rendering);
}
#endif /* ! NDEBUG */

void Transitive_Executor::notify_result(
	shared_ptr <const Dep> dep,
	Executor *,
	Flags flags,
	shared_ptr <const Dep> dep_source)
{
	assert(flags == F_RESULT_COPY);
	assert(dep_source);
	dep= append_top(dep, dep_source);
	push_result(dep);
}

void Transitive_Executor::notify_variable(
	const std::map <string, string> &result_variable_child)
{
	result_variable.insert(result_variable_child.begin(),
		result_variable_child.end());
}

bool Transitive_Executor::optional_finished(shared_ptr <const Dep> )
{
	return false;
}

shared_ptr <const Dep> Transitive_Executor::prepare(
	shared_ptr <const Dep> dep,
	shared_ptr <const Dep> dep_link)
{
	if (! dep_link->flags) return dep;
	shared_ptr <Dep> dep_new= dep->clone();
	dep_new->flags |= dep_link->flags & (F_PLACED | F_ATTRIBUTE);
	dep_new->flags |= F_RESULT_COPY;
	for (unsigned i= 0; i < C_PLACED; ++i) {
		assert(!(dep_link->flags & (1 << i)) == dep_link->get_place_flag(i).empty());
		if (dep_new->get_place_flag(i).empty() && ! dep_link->get_place_flag(i).empty())
			dep_new->set_place_flag(i, dep_link->get_place_flag(i));
	}
	return dep_new;
}
