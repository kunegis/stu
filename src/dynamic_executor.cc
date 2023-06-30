#include "dynamic_executor.hh"

Dynamic_Executor::Dynamic_Executor(shared_ptr <const Dynamic_Dep> dep_,
				   Executor *parent,
				   int &error_additional)
	:  dep(dep_),
	   is_finished(false)
{
	assert(dep_);
	assert(dep_->is_normalized());
	assert(parent);
	dep->check();

	/* Set the rule here, so cycles in the dependency graph can be
	 * detected.  Note however that the rule of dynamic executors
	 * is otherwise not used.  */

	parents[parent]= dep;
	if (error_additional) {
		*this << "";
		is_finished= true;
		parents.erase(parent);
		raise(error_additional);
		return;
	}

	/* Find the rule of the inner dependency */
	shared_ptr <const Dep> inner_dep= Dep::strip_dynamic(dep);
	if (auto inner_plain_dep= to <const Plain_Dep> (inner_dep)) {
		Target target_base(inner_plain_dep->place_param_target.flags,
				   inner_plain_dep->place_param_target.place_name.unparametrized());
		Target target= dep->get_target();
		try {
			map <string, string> mapping_parameter;
			shared_ptr <const Rule> rule=
				rule_set.get(target_base, param_rule, mapping_parameter,
					     dep->get_place());
		} catch (int e) {
			assert(e);
			*this << "";
			error_additional |= e;
			raise(e);
			return;
		}
		executors_by_target[target]= this;
	}

	parents.erase(parent);
	if (find_cycle(parent, this, dep)) {
		parent->raise(ERROR_LOGICAL);
		error_additional |= ERROR_LOGICAL;
		return;
	}
	parents[parent]= dep;

	/* Push single initial dependency */
	shared_ptr <Dep> dep_child= Dep::clone(dep->dep);
	dep_child->flags |= F_RESULT_NOTIFY;
	push(dep_child);
}

Proceed Dynamic_Executor::execute(shared_ptr <const Dep> dep_this)
{
	Proceed proceed= execute_base_A(dep_this);
	assert(proceed);
	if (proceed & (P_WAIT | P_PENDING)) {
		assert((proceed & P_FINISHED) == 0);
		return proceed;
	}
	if (proceed & P_FINISHED) {
		is_finished= true;
		return proceed;
	}

	proceed |= execute_base_B(dep_this);
	if (proceed & (P_WAIT | P_PENDING)) {
		assert((proceed & P_FINISHED) == 0);
		return proceed;
	}
	if (proceed & P_FINISHED) {
		is_finished= true;
	}

	return proceed;
}

bool Dynamic_Executor::finished() const
{
	return is_finished;
}

bool Dynamic_Executor::finished(Flags) const
{
	return is_finished;
}

bool Dynamic_Executor::want_delete() const
{
	return to <Plain_Dep> (Dep::strip_dynamic(dep)) == nullptr;
}

void Dynamic_Executor::render(Parts &parts, Rendering rendering) const
{
	dep->render(parts, rendering);
}

void Dynamic_Executor::notify_result(shared_ptr <const Dep> d,
				     Executor *source,
				     Flags flags,
				     shared_ptr <const Dep> dep_source)
{
	assert(!(flags & ~(F_RESULT_NOTIFY | F_RESULT_COPY)));
	assert((flags & ~(F_RESULT_NOTIFY | F_RESULT_COPY)) != (F_RESULT_NOTIFY | F_RESULT_COPY));
	assert(dep_source);

	if (flags & F_RESULT_NOTIFY) {
		vector <shared_ptr <const Dep> > deps;
		source->read_dynamic(to <const Plain_Dep> (d), deps, dep, this);
		for (auto &j: deps) {
			shared_ptr <Dep> j_new= Dep::clone(j);
			/* Add -% flag */
			j_new->flags |= F_RESULT_COPY;
			/* Add flags from self */
			j_new->flags |= dep->flags & (F_TARGET_BYTE & ~F_TARGET_DYNAMIC);
			for (unsigned i= 0; i < C_PLACED; ++i) {
				if (j_new->get_place_flag(i).empty() &&
				    ! dep->get_place_flag(i).empty())
					j_new->set_place_flag(i, dep->get_place_flag(i));
			}
			j= j_new;
			push(j);
		}
	} else {
		assert(flags & F_RESULT_COPY);
		push_result(d);
	}
}
