#include "transient_executor.hh"

Transient_Executor::~Transient_Executor()
/* Objects of this type are never deleted */
{
	unreachable();
}

Proceed Transient_Executor::execute(shared_ptr <const Dep> dep_link)
{
	Debug debug(this);
	Proceed proceed= execute_phase_A(dep_link);
	assert(proceed);
	if (proceed & (P_WAIT | P_CALL_AGAIN)) {
		return proceed;
	}
	if (proceed & P_FINISHED) {
		is_finished= true;
	}

	return proceed;
}

bool Transient_Executor::finished() const
{
	return is_finished;
}

bool Transient_Executor::finished(Flags) const
{
	return is_finished;
}

Transient_Executor::Transient_Executor(shared_ptr <const Dep> dep_link,
				       Executor *parent,
				       shared_ptr <const Rule> rule_,
				       shared_ptr <const Rule> param_rule_,
				       std::map <string, string> &mapping_parameter_,
				       int &error_additional)
	:  Executor(param_rule_), rule(rule_), is_finished(false)
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
		is_finished= true;
		parents.erase(parent);
		raise(error_additional);
		return;
	}

	if (rule == nullptr) {
		/* There must be a rule for transient targets (as opposed to file
		 * targets), so this is an error. */
		is_finished= true;
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
			shared_ptr <Dep> depp_new= Dep::clone(depp);
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
