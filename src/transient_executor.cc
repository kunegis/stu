#include "transient_executor.hh"

Transient_Executor::~Transient_Executor()
/* Objects of this type are never deleted */
{
	unreachable();
}

Proceed Transient_Executor::execute(shared_ptr <const Dep> dep_link)
{
	TRACE_FUNCTION(show_trace(dep_link));
	TRACE("done= %s", done.show());
	Debug debug(this);

	Proceed proceed= execute_phase_A(dep_link);
	assert(proceed);
	if (proceed & P_ABORT) {
		TRACE("Phase A abort");
		assert(proceed & P_FINISHED);
		done |= Done::from_flags(dep_link->flags);
		return proceed;
	}
	if (proceed & (P_WAIT | P_CALL_AGAIN)) {
		TRACE("Phase A wait / call again");
		assert((proceed & P_FINISHED) == 0);
		return proceed;
	}

	assert(proceed == P_FINISHED);
	done |= Done::from_flags(dep_link->flags & (F_PERSISTENT | F_OPTIONAL));

	if (finished(dep_link->flags)) {
		done |= Done::from_flags(dep_link->flags);
		return proceed |= P_FINISHED;
	}

	Proceed proceed_B= execute_phase_B(dep_link);
	TRACE("proceed_B= %s", show(proceed_B));
	assert(proceed_B);
	if (proceed_B & P_ABORT) {
		TRACE("Phase B abort");
		assert(proceed_B & P_FINISHED);
		done |= Done::from_flags(dep_link->flags);
		return proceed_B;
	}
	if (proceed_B & (P_WAIT | P_CALL_AGAIN)) {
		TRACE("Phase B wait / call again");
		assert((proceed_B & P_FINISHED) == 0);
		return proceed_B;
	}

	assert(proceed_B == P_FINISHED);

	done |= Done::from_flags(dep_link->flags);
	return proceed_B |= P_FINISHED;
}

bool Transient_Executor::finished(Flags flags) const
{
	TRACE_FUNCTION();
	TRACE("flags= %s", show(flags));
	bool ret= done.is_done_from_flags(flags);
	TRACE("ret= %s", frmt("%d", ret));
	return ret;
}

Transient_Executor::Transient_Executor(
	shared_ptr <const Dep> dep_link,
	Executor *parent,
	shared_ptr <const Rule> rule_,
	shared_ptr <const Rule> param_rule_,
	std::map <string, string> &mapping_parameter_,
	int &error_additional)
	: Executor(param_rule_), rule(rule_)
{
	swap(mapping_parameter, mapping_parameter_);

	assert(to <Plain_Dep> (dep_link));
	shared_ptr <const Plain_Dep> plain_dep=
		to <Plain_Dep> (dep_link);

	Hash_Dep hash_dep= plain_dep->place_target.unparametrized();
	assert(hash_dep.is_transient());

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
		/* There must be a rule for transient targets (as opposed to file
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

	for (auto &dependency: rule->deps) {
		shared_ptr <const Dep> depp= dependency;
		if (dep_link->flags) {
			shared_ptr <Dep> depp_new= depp->clone();
			depp_new->flags |= dep_link->flags & (F_PLACED | F_ATTRIBUTE);
			depp_new->flags |= F_RESULT_COPY;
			for (unsigned i= 0; i < C_PLACED; ++i) {
				assert(!(dep_link->flags & (1 << i)) ==
				       dep_link->get_place_flag(i).empty());
				if (depp_new->get_place_flag(i).empty() && ! dep_link->get_place_flag(i).empty())
					depp_new->set_place_flag(i, dep_link->get_place_flag(i));
			}
			depp= depp_new;
		}
		push(depp);
	}

	parents.erase(parent);
	if (find_cycle(parent, this, dep_link)) {
		raise(ERROR_LOGICAL);
		error_additional |= ERROR_LOGICAL;
		return;
	}
	parents[parent]= dep_link;
}

void Transient_Executor::render(Parts &parts, Rendering rendering) const
{
	assert(hash_deps.size());
	return hash_deps.front().render(parts, rendering);
}

void Transient_Executor::notify_result(shared_ptr <const Dep> dep,
				       Executor *,
				       Flags flags,
				       shared_ptr <const Dep> dep_source)
{
	assert(flags == F_RESULT_COPY);
	assert(dep_source);
	dep= append_top(dep, dep_source);
	push_result(dep);
}

void Transient_Executor::notify_variable(
	const std::map <string, string> &result_variable_child)
{
	result_variable.insert(result_variable_child.begin(),
		result_variable_child.end());
}

bool Transient_Executor::optional_finished(shared_ptr <const Dep> )
{
	return false;
}
