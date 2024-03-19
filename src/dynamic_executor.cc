#include "dynamic_executor.hh"

Dynamic_Executor::Dynamic_Executor(shared_ptr <const Dynamic_Dep> dep_,
				   Executor *parent,
				   int &error_additional)
	:  dep(dep_)
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
		done.set_all();
		parents.erase(parent);
		raise(error_additional);
		return;
	}

	/* Find the rule of the inner dependency */
	shared_ptr <const Dep> inner_dep= Dep::strip_dynamic(dep);
	if (auto inner_plain_dep= to <const Plain_Dep> (inner_dep)) {
		Hash_Dep hash_dep_base(inner_plain_dep->place_target.flags,
				       inner_plain_dep->place_target.place_name.unparametrized());
		Hash_Dep hash_dep= dep->get_target();
		try {
			std::map <string, string> mapping_parameter;
			shared_ptr <const Rule> rule=
				rule_set.get(hash_dep_base, param_rule, mapping_parameter,
					     dep->get_place());
		} catch (int e) {
			assert(e);
			*this << "";
			error_additional |= e;
			raise(e);
			return;
		}
		executors_by_hash_dep[hash_dep]= this;
	}

	parents.erase(parent);
	if (find_cycle(parent, this, dep)) {
		parent->raise(ERROR_LOGICAL);
		error_additional |= ERROR_LOGICAL;
		return;
	}
	// TODO can this be removed? (same assignment as above)
	parents[parent]= dep;

	/* Push single initial dependency */
	shared_ptr <Dep> dep_child= Dep::clone(dep->dep);
	dep_child->flags |= F_RESULT_NOTIFY;
	push(dep_child);

	if (dep->flags & F_PHASE_B) bits |= B_NEED_BUILD;
}

Proceed Dynamic_Executor::execute(shared_ptr <const Dep> dep_link)
{
	TRACE_FUNCTION();
	TRACE("{%s}", show(dep_link, S_DEBUG, R_SHOW_FLAGS));
	Debug debug(this);

	Proceed proceed= execute_phase_A(dep_link);
	assert(proceed);
	if (proceed & P_ABORT) {
		assert(proceed & P_FINISHED);
		done |= Done::from_flags(dep_link->flags);
		return proceed;
	}
	if (proceed & (P_WAIT | P_CALL_AGAIN)) {
		assert((proceed & P_FINISHED) == 0);
		return proceed;
	}

	assert(proceed & P_FINISHED);
	proceed &= ~P_FINISHED;

	if (finished(dep_link->flags)) {
		assert(! (proceed & P_WAIT));
		return proceed | P_FINISHED;
	}

	if (! (bits & B_NEED_BUILD)) {
		done |= Done::from_flags(dep_link->flags);
		return proceed | P_FINISHED;
	}

	Proceed proceed_B= execute_phase_B(dep_link);
	assert(proceed_B);
	proceed |= proceed_B;
	if (proceed & (P_WAIT | P_CALL_AGAIN)) {
		assert((proceed & P_FINISHED) == 0);
		return proceed;
	}
	if (proceed & P_FINISHED) {
		done |= Done::from_flags(dep_link->flags);
		return proceed;
	}

	if (finished(dep_link->flags)) {
		assert(! (proceed & P_WAIT));
		return proceed | P_FINISHED;
	}

	assert(proceed);
	return proceed;
}

bool Dynamic_Executor::finished() const
{
	return done.is_all();
}

bool Dynamic_Executor::finished(Flags flags) const
{
	return done.is_done_from_flags(flags);
}

bool Dynamic_Executor::want_delete() const
{
	return to <Plain_Dep> (Dep::strip_dynamic(dep)) == nullptr;
}

void Dynamic_Executor::render(Parts &parts, Rendering rendering) const
{
	dep->render(parts, rendering);
}

void Dynamic_Executor::notify_result(shared_ptr <const Dep> d, Executor *source,
				     Flags flags, shared_ptr <const Dep> dep_source)
{
	assert(!(flags & ~(F_RESULT_NOTIFY | F_RESULT_COPY)));
	assert((flags & ~(F_RESULT_NOTIFY | F_RESULT_COPY))
	       != (F_RESULT_NOTIFY | F_RESULT_COPY));
	assert(dep_source);

	if (flags & F_RESULT_NOTIFY) {
		std::vector <shared_ptr <const Dep> > deps;
		source->read_dynamic(to <const Plain_Dep> (d), deps, dep, this);
		for (auto &j: deps) {
			shared_ptr <Dep> j_new= Dep::clone(j);
			/* Add -% flag */
			j_new->flags |= F_RESULT_COPY;
			/* Add flags from self */
			j_new->flags |= dep->flags & (F_TARGET_WORD & ~F_TARGET_DYNAMIC);
			for (unsigned i= 0; i < C_PLACED; ++i) {
				if (j_new->get_place_flag(i).empty() &&
				    ! dep->get_place_flag(i).empty())
					j_new->set_place_flag(i, dep->get_place_flag(i));
			}
			j= j_new;
			push(j);
		}
	} else if (flags & F_RESULT_COPY) {
		push_result(d);
	} else {
		unreachable();
	}
}
